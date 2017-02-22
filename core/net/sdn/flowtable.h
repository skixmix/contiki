/* 
 * File:   flowtable.h
 * Author: Giulio Micheloni
 *
 * Created on 10 novembre 2016, 15.21
 */

#ifndef FLOWTABLE_H
#define	FLOWTABLE_H

#include "lib/memb.h"
#include "lib/list.h"
#include "net/linkaddr.h"
#include <string.h>
#include "net/sdn/datapath.h"
#include "net/sdn/cn-cbor.h"


/*-------------------------Constants definition-------------------------------*/

#define MAX_NUM_ENTRIES         10
#define MAX_NUM_RULES           10
#define MAX_NUM_ACTIONS         10
#define NUM_BYTES_2_BLOCKS      5
#define NUM_BYTES_4_BLOCKS      0
#define NUM_BYTES_8_BLOCKS      20
#define NUM_BYTES_16_BLOCKS     0

/*---------------------Flow Table data structures-----------------------------*/

typedef struct bytes2{
    uint8_t mem_space[2];
} bytes2_t;

typedef struct bytes4{
    uint8_t mem_space[4];
} bytes4_t;

typedef struct bytes8{
    uint8_t mem_space[8];
} bytes8_t;

typedef struct bytes16{
    uint8_t mem_space[16];
} bytes16_t;

typedef enum {
    FORWARD = 1,
    DROP = 2,
    MODIFY = 3,
    DECREMENT = 4,
    INCREMENT = 5,
    CONTINUE = 6,
    TO_UPPER_L = 7
} action_type_t;

typedef enum {
    NO_FIELD = 1,
    LINK_SRC_ADDR = 2,          //802.15.4 source address
    LINK_DST_ADDR = 3,          //802.15.4 destination address
    MH_SRC_ADDR = 4,            //Mesh Header Originator Address field
    MH_DST_ADDR = 5,            //Mesh Header Final Address field
    MH_HL = 6,                  //Mesh Header Hop Limit field
    SICSLO_DISPATCH = 7,        //6LoWPAN Dispatch Type field(s)
    SICSLO_BRDCST_HDR = 8,      //6LoWPAN Broadcast Header
    SICSLO_FRAG1_HDR = 9,       //6LoWPAN Fragmentation Header (first fragment)
    SICSLO_FRAGN_HDR = 10,       //6LoWPAN Fragmentation Header (subsequent fragments)
    SICSLO_IPHC = 11,            //6LoWPAN IPv6 Header Compression
    SICSLO_NHC = 12,             //6LoWPAN IPv6 Next Header Compression
    SICSLO_IPV6 = 13,            //6LoWPAN IPv6 Uncompressed IPv6 addresses
    IP_SRC_ADDR = 14,            //IPv6 packet Source Address field
    IP_DST_ADDR = 15,            //IPv6 packet Destination Address field
    IP_HL = 16,                  //IPv6 packet Hop Limit field
    IP_TC = 17,                  //IPv6 packet Traffic Class field
    IP_FL = 18,                  //IPv6 packet Flow Label field
    IP_NH = 19,                  //IPv6 packet Next Header field
    IP_PAYLOAD = 20,             //Payload of the IPv6 packet
    NODE_STATE = 21              //State array
} field_t;

typedef enum {
    EQUAL = 1,
    NOT_EQUAL = 2,
    GREATER = 3,
    LESS = 4,
    GREATER_OR_EQUAL = 5,
    LESS_OR_EQUAL = 6
} operator_t;

typedef union {
    uint8_t byte;
    uint8_t* bytes;
} value_t;

typedef struct rule{
    struct rule* next;
    field_t field;
    uint8_t offset;
    uint8_t size;
    operator_t operator;
    value_t value;
} rule_t;

typedef struct action{
    struct action* next;
    action_type_t type;
    field_t field;
    uint8_t offset;
    uint8_t size;
    value_t value;
} action_t;

typedef struct stats{
    uint16_t ttl;
    uint16_t count;
} stats_t;

typedef struct entry{
    struct entry *next;
    uint16_t priority;      //The value 0 means that the priority field is not meaningful
    LIST_STRUCT(rules);
    LIST_STRUCT(actions);
    stats_t stats;
} entry_t;



/*---------------------------Flow Table API-----------------------------------*/
void flowtable_init();
entry_t* allocate_entry();
action_t* allocate_action();
rule_t* allocate_rule();
void print_flowtable();
void print_action(action_t* a);
void print_entry(entry_t* e);
void print_rule(rule_t* r);
entry_t* create_entry(uint16_t priority);
uint8_t add_rule_to_entry(entry_t* entry, rule_t* rule);
uint8_t add_action_to_entry(entry_t* entry, action_t* action);
uint8_t add_entry_to_ft(entry_t* entry);
rule_t* create_rule(field_t field, uint8_t offset, uint8_t size, operator_t operator, uint8_t* value);
action_t* create_action(action_type_t type, field_t field, uint8_t offset, uint8_t size, uint8_t* value);
entry_t* getFlowTableHead();
entry_t* find_entry(entry_t* entry_to_match);
uint8_t remove_entry(entry_t* entry_to_match);
void deallocate_entry(entry_t* entry);
void flowtable_test();
uint8_t* install_flow_entry_from_cbor(cn_cbor* cp);
#endif	/* FLOWTABLE_H */

