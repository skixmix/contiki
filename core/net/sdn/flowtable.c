/* 
 * File:   flowtable.c
 * Author: Giulio Micheloni
 *
 * Created on 10 novembre 2016, 15.21
 */


#include "flowtable.h"

#define DEBUG 1
#if DEBUG && (!SINK || DEBUG_SINK)
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

LIST(flowtable);
MEMB(entries_memb, entry_t, MAX_NUM_ENTRIES);
MEMB(actions_memb, action_t, MAX_NUM_ACTIONS);
MEMB(rules_memb, rule_t, MAX_NUM_RULES);
MEMB(bytes2_memb, bytes2_t, NUM_BYTES_2_BLOCKS);
MEMB(bytes4_memb, bytes4_t, NUM_BYTES_4_BLOCKS);
MEMB(bytes8_memb, bytes8_t, NUM_BYTES_8_BLOCKS);
MEMB(bytes16_memb, bytes16_t, NUM_BYTES_16_BLOCKS);
uint8_t status_register[STATUS_REGISTER_SIZE];

uint8_t clean_up_oldest_entry(){
    //TODO: find the oldest flow table entry and clean up its space
}

void flowtable_init(){
    list_init(flowtable);
    memb_init(&entries_memb);
    memb_init(&rules_memb);
    memb_init(&actions_memb);
    memb_init(&bytes2_memb);
    memb_init(&bytes4_memb);
    memb_init(&bytes8_memb);
    memb_init(&bytes16_memb);
}

void entry_init(entry_t *e){
    memset(e, 0, sizeof(*e));
    e->stats.ttl = 0;
    e->stats.count = 0; 
    LIST_STRUCT_INIT(e, rules);
    LIST_STRUCT_INIT(e, actions);
    e->priority = 0;
}

entry_t* allocate_entry(){
    entry_t *e;

    e = memb_alloc(&entries_memb);
    if(e == NULL) {
        clean_up_oldest_entry();
        e = memb_alloc(&entries_memb);
        if(e == NULL) {
            PRINTF("[FLT]: Failed to allocate an entry\n");
            return NULL;
        }
    }
    
    entry_init(e);
    return e;
}
action_t* allocate_action(){
    action_t *a;
    a = memb_alloc(&actions_memb);
    if(a == NULL) {
        PRINTF("[FLT]: Failed to allocate an action\n");
        return NULL;
    }
    memset(a, 0, sizeof(*a));
    return a;
}
rule_t* allocate_rule(){
    rule_t *r;
    r = memb_alloc(&actions_memb);
    if(r == NULL) {
        PRINTF("[FLT]: Failed to allocate a rule\n");
        return NULL;
    }
    memset(r, 0, sizeof(*r));
    return r;
}

action_t* create_action(action_type_t type, field_t field, uint8_t offset, uint8_t size, uint8_t* value){
    action_t* a;
    if(size < 0 || size > 128 || offset < 0 || offset > 128){
        PRINTF("[FLT]: Failed to create an action bacause of invalid parameters\n");
        return NULL;
    }
       
    a = allocate_action();
    if(a == NULL)
        return NULL;
    a->type = type;
    a->field = field;
    a->offset = offset;
    a->size = size;
    if(size <= 8){
        a->value.byte = *value;
    }
    else if(size > 8 && size <= 16){
        a->value.bytes = memb_alloc(&bytes2_memb);
        memset(a->value.bytes, 0, sizeof(*a->value.bytes));
        memcpy(a->value.bytes, value, 2);
    }
    else if(size > 16 && size <= 32){
        a->value.bytes = memb_alloc(&bytes4_memb);
        memset(a->value.bytes, 0, sizeof(*a->value.bytes));
        memcpy(a->value.bytes, value, 4);
    }
    else if(size > 32 && size <= 64){
        a->value.bytes = memb_alloc(&bytes8_memb);
        memset(a->value.bytes, 0, sizeof(*a->value.bytes));
        memcpy(a->value.bytes, value, 8);
    }
    else if(size > 64 && size <= 128){
        a->value.bytes = memb_alloc(&bytes16_memb);
        memset(a->value.bytes, 0, sizeof(*a->value.bytes));
        memcpy(a->value.bytes, value, 16);
    }
    return a;
}

rule_t* create_rule(field_t field, uint8_t offset, uint8_t size, operator_t operator, uint8_t* value){
    rule_t *r;
    if(size < 0 || size > 128 || offset < 0 || offset > 128){
        PRINTF("[FLT]: Failed to create a rule bacause of invalid parameters\n");
        return NULL;
    }
    
    r = allocate_rule();
    if(r == NULL)
        return NULL;
    
    r->field = field;
    r->offset = offset;
    r->size = size;
    r->operator = operator;
    
    if(size <= 8){
        r->value.byte = *value; 
    }
    else if(size > 8 && size <= 16){
        r->value.bytes = memb_alloc(&bytes2_memb);
        memset(r->value.bytes, 0, sizeof(*r->value.bytes));
        memcpy(r->value.bytes, value, 2);
    }
    else if(size > 16 && size <= 32){
        r->value.bytes = memb_alloc(&bytes4_memb);
        memset(r->value.bytes, 0, sizeof(*r->value.bytes));
        memcpy(r->value.bytes, value, 4);
    }
    else if(size > 32 && size <= 64){
        r->value.bytes = memb_alloc(&bytes8_memb);
        memset(r->value.bytes, 0, sizeof(*r->value.bytes));
        memcpy(r->value.bytes, value, 8);
    }
    else if(size > 64 && size <= 128){
        r->value.bytes = memb_alloc(&bytes16_memb);
        memset(r->value.bytes, 0, sizeof(*r->value.bytes));
        memcpy(r->value.bytes, value, 16);
    }
    
    return r;
}

uint8_t add_entry(uint16_t priority, rule_t* rule, action_t* action){
    entry_t* entry;
    entry_t* tmp;
    entry_t* help = NULL;
    if(rule == NULL){
        PRINTF("[FLT]: Failed to add a new entry bacause of invalid parameters\n");
        return -1;
    }
    
    entry = allocate_entry();
    entry->priority = priority;
    list_add(entry->rules, rule);
    list_add(entry->actions, action);
    
    //Sorted insert into the flow table list  
    for(tmp = list_head(flowtable); tmp != NULL; tmp = tmp->next){
        if(tmp->priority > priority)
            break;
        help = tmp;
    }
    //Last element of the list
    if(tmp == NULL && help != NULL)
        list_add(flowtable, entry);
    //First element of the list
    else if(help == NULL)
        list_push(flowtable, entry);
    //Insert the element in the middle of the list
    else{
        entry->next = help->next;
        help->next = entry;
    }
    return 1;
}

void print_action(action_t* a){
    switch(a->type){
        case FORWARD: PRINTF("FORWARD "); break;
        case DROP: PRINTF("DROP "); break;
        case MODIFY: PRINTF("MODIFY "); break;
        case DECREMENT: PRINTF("DECREMENT "); break;
        case INCREMENT: PRINTF("INCREMENT "); break;
        case CONTINUE: PRINTF("CONTINUE "); break;
        case TO_UPPER_L: PRINTF("TO_UPPER_L "); break;
        default: PRINTF("NO_ACTION ");
            
    }
    switch(a->field){
        case LINK_SRC_ADDR: PRINTF("LINK_SRC_ADDR "); break;
        case LINK_DST_ADDR: PRINTF("LINK_DST_ADDR "); break;
        case MH_SRC_ADDR: PRINTF("MH_SRC_ADDR "); break;
        case MH_DST_ADDR: PRINTF("MH_DST_ADDR "); break;
        case MH_HL: PRINTF("MH_HL "); break;
        case SICSLO_DISPATCH: PRINTF("SICSLO_DISPATCH "); break;
        case SICSLO_BRDCST_HDR: PRINTF("SICSLO_BRDCST_HDR "); break;
        case SICSLO_FRAG1_HDR: PRINTF("SICSLO_FRAG1_HDR "); break;
        case SICSLO_FRAGN_HDR: PRINTF("SICSLO_FRAGN_HDR "); break;
        case SICSLO_IPHC: PRINTF("SICSLO_IPHC "); break;
        case SICSLO_NHC: PRINTF("SICSLO_NHC "); break;
        case IP_SRC_ADDR: PRINTF("IP_SRC_ADDR "); break;
        case IP_DST_ADDR: PRINTF("IP_DST_ADDR "); break;
        case IP_HL: PRINTF("IP_HL "); break;
        case IP_TC: PRINTF("IP_TC "); break;
        case IP_FL: PRINTF("IP_FL "); break;
        case IP_NH: PRINTF("IP_NH "); break;
        case IP_PAYLOAD: PRINTF("IP_PAYLOAD "); break;
        case NODE_STATE: PRINTF("NODE_STATE "); break;
        default: PRINTF("NO_FIELD ");
    }
    

    PRINTF("OFFSET: %u ", a->offset);
    PRINTF("SIZE: %u ", a->size);
    if(a->size <= 8)
        PRINTF("VALUE: %c ", (char)a->value.byte);
    else
        PRINTF("VALUE: %s ", a->value.bytes);
}

void print_rule(rule_t* r){
    
    switch(r->field){
        case LINK_SRC_ADDR: PRINTF("LINK_SRC_ADDR "); break;
        case LINK_DST_ADDR: PRINTF("LINK_DST_ADDR "); break;
        case MH_SRC_ADDR: PRINTF("MH_SRC_ADDR "); break;
        case MH_DST_ADDR: PRINTF("MH_DST_ADDR "); break;
        case MH_HL: PRINTF("MH_HL "); break;
        case SICSLO_DISPATCH: PRINTF("SICSLO_DISPATCH "); break;
        case SICSLO_BRDCST_HDR: PRINTF("SICSLO_BRDCST_HDR "); break;
        case SICSLO_FRAG1_HDR: PRINTF("SICSLO_FRAG1_HDR "); break;
        case SICSLO_FRAGN_HDR: PRINTF("SICSLO_FRAGN_HDR "); break;
        case SICSLO_IPHC: PRINTF("SICSLO_IPHC "); break;
        case SICSLO_NHC: PRINTF("SICSLO_NHC "); break;
        case IP_SRC_ADDR: PRINTF("IP_SRC_ADDR "); break;
        case IP_DST_ADDR: PRINTF("IP_DST_ADDR "); break;
        case IP_HL: PRINTF("IP_HL "); break;
        case IP_TC: PRINTF("IP_TC "); break;
        case IP_FL: PRINTF("IP_FL "); break;
        case IP_NH: PRINTF("IP_NH "); break;
        case IP_PAYLOAD: PRINTF("IP_PAYLOAD "); break;
        case NODE_STATE: PRINTF("NODE_STATE "); break;
        default: PRINTF("NO_FIELD ");
    }
    
    switch(r->operator){
        case EQUAL: PRINTF("EQUAL "); break;
        case NOT_EQUAL: PRINTF("NOT_EQUAL "); break;
        case GREATER: PRINTF("GREATER "); break;
        case LESS: PRINTF("LESS "); break;
        case GREATER_OR_EQUAL: PRINTF("GREATER_OR_EQUAL "); break;
        case LESS_OR_EQUAL: PRINTF("LESS_OR_EQUAL "); break;
    }

    PRINTF("OFFSET: %u ", r->offset);
    PRINTF("SIZE: %u ", r->size);
    if(r->size <= 8)
        PRINTF("VALUE: %c ", (char)r->value.byte);
    else
        PRINTF("VALUE: %s ", r->value.bytes);
}

void print_entry(entry_t* e) {
    rule_t *r;
    action_t *a;
    PRINTF("Priority %u: IF (", e->priority);
    for(r = list_head(e->rules); r != NULL; r = r->next) {
        print_rule(r);
        PRINTF(";");
    }
    PRINTF("){");
    for(a = list_head(e->actions); a != NULL; a = a->next) {
        print_action(a);
        PRINTF(";");
    } 
    PRINTF("}[%d %d]", e->stats.ttl, e->stats.count);   
}

void print_flowtable() {
    entry_t *e;
    for(e = list_head(flowtable); e != NULL; e = e->next) {
        PRINTF("[FLT]: ");
        print_entry(e);
        PRINTF("\n");  
    }
}

void flowtable_test(){
    flowtable_init();
    rule_t* rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, "feff001");
    action_t* action= create_action(FORWARD, NO_FIELD, 0, 64, "feff002");
    add_entry(2, rule, action);
    rule = create_rule(SICSLO_DISPATCH, 0, 8, EQUAL, "c");
    action= create_action(MODIFY, SICSLO_DISPATCH, 0, 8, "b");
    add_entry(5, rule, action);
    rule = create_rule(SICSLO_IPHC, 0, 16, EQUAL, "ca");
    action= create_action(MODIFY, SICSLO_IPHC, 3, 32, "34ca");
    add_entry(1, rule, action);
    print_flowtable();
}