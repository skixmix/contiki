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
#include <stdio.h>
#include <string.h>

#define DEBUG 1
#if DEBUG && (!SINK || DEBUG_SINK)
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*-------------------------Constants definition-------------------------------*/

#define MAX_NUM_ENTRIES         10
#define MAX_NUM_RULES           20
#define MAX_NUM_ACTIONS         20
#define NUM_BYTES_2_BLOCKS      10
#define NUM_BYTES_4_BLOCKS      10
#define NUM_BYTES_8_BLOCKS      10
#define NUM_BYTES_16_BLOCKS     5

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
    FORWARD,
    DROP,
    MODIFY,
    DECREMENT,
    INCREMENT,
    CONTINUE,
    TO_UPPER_L
} action_type_t;

typedef enum {
    NO_FIELD,
    LINK_SRC_ADDR,          //802.15.4 source address
    LINK_DST_ADDR,          //802.15.4 destination address
    MH_SRC_ADDR,            //Mesh Header Originator Address field
    MH_DST_ADDR,            //Mesh Header Final Address field
    MH_HL,                  //Mesh Header Hop Limit field
    SICSLO_DISPATCH,        //6LoWPAN Dispatch Type field(s)
    SICSLO_BRDCST_HDR,      //6LoWPAN Broadcast Header
    SICSLO_FRAG1_HDR,       //6LoWPAN Fragmentation Header (first fragment)
    SICSLO_FRAGN_HDR,       //6LoWPAN Fragmentation Header (subsequent fragments)
    SICSLO_IPHC,            //6LoWPAN IPv6 Header Compression
    SICSLO_NHC,             //6LoWPAN IPv6 Next Header Compression
    SICSLO_IPV6,            //6LoWPAN IPv6 Uncompressed IPv6 addresses
    IP_SRC_ADDR,            //IPv6 packet Source Address field
    IP_DST_ADDR,            //IPv6 packet Destination Address field
    IP_HL,                  //IPv6 packet Hop Limit field
    IP_TC,                  //IPv6 packet Traffic Class field
    IP_FL,                  //IPv6 packet Flow Label field
    IP_NH,                  //IPv6 packet Next Header field
    IP_PAYLOAD,             //Payload of the IPv6 packet
    NODE_STATE              //State array
} field_t;

typedef enum {
    EQUAL,
    NOT_EQUAL,
    GREATER,
    LESS,
    GREATER_OR_EQUAL,
    LESS_OR_EQUAL
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
    uint16_t priority;
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
void flowtable_test();
#endif	/* FLOWTABLE_H */

