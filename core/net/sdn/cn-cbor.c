#ifndef CN_CBOR_C
#define CN_CBOR_C


#ifdef CONTIKI
#define CBOR_NO_FLOAT 1
#include <uip.h>
#define ntohl(x) uip_ntohl(x)
#define ntohs(x) uip_ntohs(x)
#else
#include <arpa/inet.h>
#endif

#include "cn-cbor.h"

#ifdef CONTIKI
#include "memb.h"

#ifndef CBOR_MAX_ELEM
#define CBOR_MAX_ELEM 50 //Era 64 (For slicing it must be at least 48)
#endif

MEMB(cbor_pool, cn_cbor, CBOR_MAX_ELEM);

static inline
cn_cbor *cn_cbor_calloc() {
  cn_cbor *cb = (cn_cbor *)memb_alloc(&cbor_pool);
  if (cb) {
    memset(cb, 0, sizeof(cn_cbor));
  }
  return cb;
}

#define CN_CBOR_CALLOC() cn_cbor_calloc()
#define CN_CBOR_FREE(cb) memb_free(&cbor_pool, (cb))
#else
// can be redefined, e.g. for pool allocation
#ifndef CN_CBOR_CALLOC
#define CN_CBOR_CALLOC() calloc(1, sizeof(cn_cbor))
#define CN_CBOR_FREE(cb) free((void*)(cb))
#endif
#endif

#define CN_CBOR_FAIL(code) do { pb->err = code;  goto fail; } while(0)

void cn_cbor_free(const cn_cbor* cb) {
  cn_cbor* p = (cn_cbor*) cb;
  while (p) {
    cn_cbor* p1;
    while ((p1 = p->first_child)) { /* go down */
      p = p1;
    }
    if (!(p1 = p->next)) {   /* go up next */
      if ((p1 = p->parent))
        p1->first_child = 0;
    }
    CN_CBOR_FREE(p);
    p = p1;
  }
}

#ifndef CBOR_NO_FLOAT
static double decode_half(int half) {
  int exp = (half >> 10) & 0x1f;
  int mant = half & 0x3ff;
  double val;
  if (exp == 0) val = ldexp(mant, -24);
  else if (exp != 31) val = ldexp(mant + 1024, exp - 25);
  else val = mant == 0 ? INFINITY : NAN;
  return half & 0x8000 ? -val : val;
}
#endif

/* Fix these if you can't do non-aligned reads */
#define ntoh8p(p) (*(unsigned char*)(p))
#define ntoh16p(p) (ntohs(*(unsigned short*)(p)))
#define ntoh32p(p) (ntohl(*(unsigned long*)(p)))
static uint64_t ntoh64p(unsigned char *p) {
  uint64_t ret = ntoh32p(p);
  ret <<= 32;
  ret += ntoh32p(p+4);
  return ret;
}

static cn_cbor_type mt_trans[] = {
  CN_CBOR_UINT,    CN_CBOR_INT,
  CN_CBOR_BYTES,   CN_CBOR_TEXT,
  CN_CBOR_ARRAY,   CN_CBOR_MAP,
  CN_CBOR_TAG,     CN_CBOR_SIMPLE,
};

struct parse_buf {
  unsigned char *buf;
  unsigned char *ebuf;
  cn_cbor_error err;
};

#define TAKE(pos, ebuf, n, stmt)                \
  if (n > (ebuf - pos))                         \
    CN_CBOR_FAIL(CN_CBOR_ERR_OUT_OF_DATA);                \
  stmt;                                         \
  pos += n;

static cn_cbor *decode_item (struct parse_buf *pb, cn_cbor* top_parent) {
  unsigned char *pos = pb->buf;
  unsigned char *ebuf = pb->ebuf;
  cn_cbor* parent = top_parent;
again:
  TAKE(pos, ebuf, 1, int ib = ntoh8p(pos) );
  if (ib == IB_BREAK) {
    if (!(parent->flags & CN_CBOR_FL_INDEF))
      CN_CBOR_FAIL(CN_CBOR_ERR_BREAK_OUTSIDE_INDEF);
    switch (parent->type) {
    case CN_CBOR_BYTES: case CN_CBOR_TEXT:
      parent->type += 2;            /* CN_CBOR_* -> CN_CBOR_*_CHUNKED */
      break;
    case CN_CBOR_MAP:
      if (parent->length & 1)
        CN_CBOR_FAIL(CN_CBOR_ERR_ODD_SIZE_INDEF_MAP);
    default:;
    }
    goto complete;
  }
  unsigned int mt = ib >> 5;
  int ai = ib & 0x1f;
  uint64_t val = ai;

  cn_cbor* cb = CN_CBOR_CALLOC();
  if (!cb)
    CN_CBOR_FAIL(CN_CBOR_ERR_OUT_OF_MEMORY);

  cb->type = mt_trans[mt];

  cb->parent = parent;
  if (parent->last_child) {
    parent->last_child->next = cb;
  } else {
    parent->first_child = cb;
  }
  parent->last_child = cb;
  parent->length++;

  cn_cbor *it;
  cn_cbor *it2;
  uint64_t i;

  switch (ai) {
  case AI_1: TAKE(pos, ebuf, 1, val = ntoh8p(pos)) ; break;
  case AI_2: TAKE(pos, ebuf, 2, val = ntoh16p(pos)) ; break;
  case AI_4: TAKE(pos, ebuf, 4, val = ntoh32p(pos)) ; break;
  case AI_8: TAKE(pos, ebuf, 8, val = ntoh64p(pos)) ; break;
  case 28: case 29: case 30: CN_CBOR_FAIL(CN_CBOR_ERR_RESERVED_AI);
  case AI_INDEF:
    if ((mt - MT_BYTES) <= MT_MAP) {
      cb->flags |= CN_CBOR_FL_INDEF;
      goto push;
    } else {
      CN_CBOR_FAIL(CN_CBOR_ERR_MT_UNDEF_FOR_INDEF);
    }
  }
  // process content
  switch (mt) {
  case MT_UNSIGNED:
    cb->v.uint = val;           /* to do: Overflow check */
    break;
  case MT_NEGATIVE:
    cb->v.sint = ~val;          /* to do: Overflow check */
    break;
  case MT_BYTES: case MT_TEXT:
    cb->v.str = (char *) pos;
    cb->length = val;
    TAKE(pos, ebuf, val, );
    break;
  case MT_MAP:
    val <<= 1;
    /* fall through */
  case MT_ARRAY:
    if ((cb->v.count = val)) {
      cb->flags |= CN_CBOR_FL_COUNT;
      goto push;
    }
    break;
  case MT_TAG:
    cb->v.uint = val;
    goto push;
  case MT_PRIM:
    switch (ai) {
    case VAL_NIL: cb->type = CN_CBOR_NULL; break;
    case VAL_FALSE: cb->type = CN_CBOR_FALSE; break;
    case VAL_TRUE: cb->type = CN_CBOR_TRUE; break;
    case AI_2: 
#ifdef CBOR_NO_FLOAT
      CN_CBOR_FAIL(CN_CBOR_ERR_FLOAT_NOT_SUPPORTED);
#else
      cb->type = CN_CBOR_DOUBLE; cb->v.dbl = decode_half(val); 
#endif
 break;
    case AI_4:
#ifdef CBOR_NO_FLOAT
      CN_CBOR_FAIL(CN_CBOR_ERR_FLOAT_NOT_SUPPORTED);
#else
      cb->type = CN_CBOR_DOUBLE;
      union {
        float f;
        uint32_t u;
      } u32;
      u32.u = val;
      cb->v.dbl = u32.f;
#endif
      break;
    case AI_8:
#ifdef CBOR_NO_FLOAT
      CN_CBOR_FAIL(CN_CBOR_ERR_FLOAT_NOT_SUPPORTED);
#else
      cb->type = CN_CBOR_DOUBLE;
      union {
        double d;
        uint64_t u;
      } u64;
      u64.u = val;
      cb->v.dbl = u64.d;
#endif
      break;
    default: cb->v.uint = val;
    }
  }
fill:                           /* emulate loops */
  if (parent->flags & CN_CBOR_FL_INDEF) {
    if (parent->type == CN_CBOR_BYTES || parent->type == CN_CBOR_TEXT)
      if (cb->type != parent->type)
          CN_CBOR_FAIL(CN_CBOR_ERR_WRONG_NESTING_IN_INDEF_STRING);
    goto again;
  }
  if (parent->flags & CN_CBOR_FL_COUNT) {
    if (--parent->v.count)
      goto again;
  }
  /* so we are done filling parent. */
complete:                       /* emulate return from call */
  if (parent == top_parent) {
    if (pos != ebuf)            /* XXX do this outside */
      CN_CBOR_FAIL(CN_CBOR_ERR_NOT_ALL_DATA_CONSUMED);
    pb->buf = pos;
    return cb;
  }
  cb = parent;
  parent = parent->parent;
  goto fill;
push:                           /* emulate recursive call */
  parent = cb;
  goto again;
fail:
  pb->buf = pos;
  return 0;
}

const cn_cbor* cn_cbor_decode(const char* buf, size_t len, cn_cbor_errback *errp) {
  cn_cbor catcher = {CN_CBOR_INVALID};
  struct parse_buf pb = {(unsigned char *)buf, (unsigned char *)buf+len};
  cn_cbor* ret = decode_item(&pb, &catcher);
  if (ret) {
    ret->parent = 0;            /* mark as top node */
  } else {
    if (catcher.first_child) {
      catcher.first_child->parent = 0;
      cn_cbor_free(catcher.first_child);
    }
  fail:
    if (errp) {
      errp->err = pb.err;
      errp->pos = pb.buf - (unsigned char *)buf;
    }
    return 0;
  }
  return ret;
}

const cn_cbor* cn_cbor_mapget_uint(const cn_cbor* cb, unsigned long key) {
  if (!cb || cb->type != CN_CBOR_MAP) {
    return NULL;
  }

  cb = cb->first_child;
  while (cb && ((cb->type != CN_CBOR_UINT) || (cb->v.uint != key))) {
    if (!(cb = cb->next)) {
      return NULL;
    }
    cb = cb->next;
  }

  return cb ? cb->next : NULL;
}

uint8_t cn_cbor_encode_uint(char* buf, void* value, uint8_t size){    //Size mesured in byte (so uint8_t = 1)
    uint8_t app = 0;
    uint8_t add_info;
    if(buf == NULL || value == NULL || size == 0)
        return 0;
    app = MT_UNSIGNED << 5;
    if(size == 1 && *(uint8_t*)value < 24){
        app |= *(uint8_t*)value;
        *buf = app;
        return 1;
    }
    switch(size){
        case 1:     //uint8_t
            app |= 24;
            *buf = app;
            *(buf+1) = *(uint8_t*)value;
            break;
        case 2:     //uing16_t
            app |= 25;
            *buf = app;
            *(buf+1) = *(uint16_t*)value >> 8;
            *(buf+2) = *(uint16_t*)value & ((1 << 8) - 1);         
            break;
        case 4:     //uint32_t
            app |= 26;
            *buf = app;
            *(buf+1) = *(uint32_t*)value >> 24;
            *(buf+2) = *(uint32_t*)value >> 16;
            *(buf+3) = *(uint32_t*)value >> 8;
            *(buf+4) = *(uint32_t*)value & ((1 << 8) - 1);
            break;
        case 8:     //uing64_t
            app |= 27;
            *buf = app;
            *(buf+1) = *(uint64_t*)value >> 56;
            *(buf+2) = *(uint64_t*)value >> 48;
            *(buf+3) = *(uint64_t*)value >> 40;
            *(buf+4) = *(uint64_t*)value >> 32;
            *(buf+5) = *(uint64_t*)value >> 24;
            *(buf+6) = *(uint64_t*)value >> 16;
            *(buf+7) = *(uint64_t*)value >> 8;
            *(buf+8) = *(uint64_t*)value & ((1 << 8) - 1);
            break;
        default:
            return 0;
            
    }
    return size + 1;
}

uint8_t cn_cbor_encode_int(char* buf, void* value, uint8_t size, uint8_t sign){    //Size mesured in byte (so uint8_t = 1)
    uint8_t app = 0;
    uint8_t add_info;
    if(buf == NULL || value == NULL || size == 0)
        return 0;
    if(sign == 0)
        app = MT_UNSIGNED << 5;
    else
        app = MT_NEGATIVE << 5;
    if(size == 1 && *(uint8_t*)value < 24){
        if(sign == 1)
            *(uint8_t*)value = -1 - *(uint8_t*)value;
        app |= *(uint8_t*)value;
        *buf = app;
        return 1;
    }
    switch(size){
        case 1:     //uint8_t
            app |= 24;
            *buf = app;
            if(sign == 1)
                *(uint8_t*)value = -1 - *(uint8_t*)value;
            *(buf+1) = *(uint8_t*)value;
            break;
        case 2:     //uing16_t
            app |= 25;
            *buf = app;
            if(sign == 1)
                *(uint16_t*)value = -1 - *(uint16_t*)value;
            *(buf+1) = *(uint16_t*)value >> 8;
            *(buf+2) = *(uint16_t*)value & ((1 << 8) - 1);         
            break;
        case 4:     //uint32_t
            app |= 26;
            *buf = app;
            if(sign == 1)
                *(uint32_t*)value = -1 - *(uint32_t*)value;
            *(buf+1) = *(uint32_t*)value >> 24;
            *(buf+2) = *(uint32_t*)value >> 16;
            *(buf+3) = *(uint32_t*)value >> 8;
            *(buf+4) = *(uint32_t*)value & ((1 << 8) - 1);
            break;
        case 8:     //uing64_t
            app |= 27;
            *buf = app;
            if(sign == 1)
                *(uint64_t*)value = -1 - *(uint8_t*)value;
            *(buf+1) = *(uint64_t*)value >> 56;
            *(buf+2) = *(uint64_t*)value >> 48;
            *(buf+3) = *(uint64_t*)value >> 40;
            *(buf+4) = *(uint64_t*)value >> 32;
            *(buf+5) = *(uint64_t*)value >> 24;
            *(buf+6) = *(uint64_t*)value >> 16;
            *(buf+7) = *(uint64_t*)value >> 8;
            *(buf+8) = *(uint64_t*)value & ((1 << 8) - 1);
            break;
        default:
            return 0;
            
    }
    return size + 1;
}


#ifdef  __cplusplus
}
#endif

#endif  /* CN_CBOR_C */
