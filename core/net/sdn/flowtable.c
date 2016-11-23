/* 
 * File:   flowtable.c
 * Author: Giulio Micheloni
 *
 * Created on 10 novembre 2016, 15.21
 */


#include "flowtable.h"



LIST(flowtable);
MEMB(entries_memb, entry_t, MAX_NUM_ENTRIES);
MEMB(actions_memb, action_t, MAX_NUM_ACTIONS);
MEMB(rules_memb, rule_t, MAX_NUM_RULES);
MEMB(bytes2_memb, bytes2_t, NUM_BYTES_2_BLOCKS);
MEMB(bytes4_memb, bytes4_t, NUM_BYTES_4_BLOCKS);
MEMB(bytes8_memb, bytes8_t, NUM_BYTES_8_BLOCKS);
MEMB(bytes16_memb, bytes16_t, NUM_BYTES_16_BLOCKS);


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
        //clean_up_oldest_entry();
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
    
    if(value == NULL){
        a->value.byte = 0;
        return a;
    }
    if(size <= 8){
        a->value.byte = *value;
    }
    else if(size > 8 && size <= 16){
        a->value.bytes = memb_alloc(&bytes2_memb);
        memcpy(a->value.bytes, value, 2);
    }
    else if(size > 16 && size <= 32){
        a->value.bytes = memb_alloc(&bytes4_memb);
        memcpy(a->value.bytes, value, 4);
    }
    else if(size > 32 && size <= 64){
        a->value.bytes = memb_alloc(&bytes8_memb);
        memcpy(a->value.bytes, value, 8);
    }
    else if(size > 64 && size <= 128){
        a->value.bytes = memb_alloc(&bytes16_memb);
        memcpy(a->value.bytes, value, 16);
    }
    return a;
}

rule_t* create_rule(field_t field, uint8_t offset, uint8_t size, operator_t operator, uint8_t* value){
    rule_t *r;
    uint8_t dim;
    if(size < 0 || size > 128 || offset < 0 || offset > 128 || value == NULL){
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
    
    //dim = size + offset;
    dim = size;
    
    if(dim <= 8){
        r->value.byte = *value; 
    }
    else if(dim > 8 && dim <= 16){
        r->value.bytes = memb_alloc(&bytes2_memb);
        memcpy(r->value.bytes, value, 2);
    }
    else if(dim > 16 && dim <= 32){
        r->value.bytes = memb_alloc(&bytes4_memb);
        memcpy(r->value.bytes, value, 4);
    }
    else if(dim > 32 && dim <= 64){
        r->value.bytes = memb_alloc(&bytes8_memb);
        memcpy(r->value.bytes, value, 8);
    }
    else if(dim > 64 && dim <= 128){
        r->value.bytes = memb_alloc(&bytes16_memb);
        memcpy(r->value.bytes, value, 16);
    }
    
    return r;
}

entry_t* create_entry(uint16_t priority){
    entry_t* entry;
    if(priority < 0){
        PRINTF("[FLT]: Failed to create a new entry bacause of invalid parameters\n");
        return NULL;
    }
    entry = allocate_entry();
    if(entry == NULL)
        return NULL;
    entry->priority = priority;
    return entry;
}

uint8_t add_rule_to_entry(entry_t* entry, rule_t* rule){
    if(rule == NULL){
        PRINTF("[FLT]: Failed to add a new rule bacause of invalid parameters\n");
        return 0;
    }
    list_add(entry->rules, rule);
}

uint8_t add_action_to_entry(entry_t* entry, action_t* action){
    if(action == NULL){
        PRINTF("[FLT]: Failed to add a new action bacause of invalid parameters\n");
        return 0;
    }
    list_add(entry->actions, action);
}

uint8_t add_entry_to_ft(entry_t* entry){
    entry_t* tmp = NULL;
    entry_t* help = NULL;
    uint16_t priority = entry->priority;
    
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
        PRINTF("VALUE: %x ", (char)a->value.byte);
    else{
        int i, dim;
        dim = a->size / 8;
        if(a->size % 8 > 0)
            dim += 1;
        PRINTF("VALUE: ");
        for(i = 0; i < dim; i++)
            PRINTF("%02x", a->value.bytes[i]);
    }
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
        PRINTF("VALUE: %x ", (char)r->value.byte);
    else{
        int i, dim;
        dim = r->size / 8;
        if(r->size % 8 > 0)
            dim += 1;
        PRINTF("VALUE: ");
        for(i = 0; i < dim; i++)
            PRINTF("%02x", r->value.bytes[i]);
    }
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

entry_t* getFlowTableHead(){
    return list_head(flowtable);
}

void flowtable_test(){
    flowtable_init();
    uint8_t addr_1[8]  = {0xc1, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t addr_2[8]  = {0xc1, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
    uint8_t addr_3[8]  = {0xc1, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
    uint8_t addr_4[8]  = {0xc1, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};
    uint8_t addr_5[8]  = {0xc1, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05};
    uint8_t addr_6[8]  = {0xc1, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06};
    rule_t* rule;
    action_t* action;
    entry_t* entry;
    
    
    
    if(memcmp(&linkaddr_node_addr, addr_2, 8) == 0){
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 54, 10, EQUAL, addr_1 + 6);  //Last 10 bits of the 64-bit long MAC address
        //rule = create_rule(MH_DST_ADDR, 60, 4, EQUAL, addr_1 + 7);     //Last 4 bits of the 64-bit long MAC address
        add_rule_to_entry(entry, rule); 
        rule = create_rule(LINK_SRC_ADDR, 0, 64, EQUAL, addr_5);
        add_rule_to_entry(entry, rule);  
        action= create_action(FORWARD, NO_FIELD, 0, 64, addr_3);        
        add_action_to_entry(entry, action);
        action= create_action(CONTINUE, NO_FIELD, 0, 0, NULL);        
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
        entry = create_entry(2);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_1);
        add_rule_to_entry(entry, rule); 
        rule = create_rule(LINK_SRC_ADDR, 0, 64, EQUAL, addr_6);
        add_rule_to_entry(entry, rule);  
        action= create_action(FORWARD, NO_FIELD, 0, 64, addr_4);        
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
    }
    
    if(memcmp(&linkaddr_node_addr, addr_3, 8) == 0){
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_1);
        action= create_action(FORWARD, NO_FIELD, 0, 64, addr_4);
        add_rule_to_entry(entry, rule);    
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
    }
    
    if(memcmp(&linkaddr_node_addr, addr_4, 8) == 0){
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_1);
        action= create_action(FORWARD, NO_FIELD, 0, 64, addr_1);
        add_rule_to_entry(entry, rule);    
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
    }
    
    if(memcmp(&linkaddr_node_addr, addr_5, 8) == 0){
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_1);
        action= create_action(FORWARD, NO_FIELD, 0, 64, addr_2);
        add_rule_to_entry(entry, rule);    
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
    }
    
    if(memcmp(&linkaddr_node_addr, addr_6, 8) == 0){
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_1);
        action= create_action(FORWARD, NO_FIELD, 0, 64, addr_2);
        add_rule_to_entry(entry, rule);    
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
    }
        
    
    
    
    
    print_flowtable();
}