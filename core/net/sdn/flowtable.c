/* 
 * File:   flowtable.c
 * Author: Giulio Micheloni
 *
 * Created on 10 novembre 2016, 15.21
 */


#include "flowtable.h"


#define DEBUG 0
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



void deallocate_value_field(value_t* value, uint8_t size){
    if(size < 8 || value == NULL)
        return;
            
    if(size > 8 && size <= 16){
        memb_free(&bytes2_memb, value->bytes);
    }
    else if(size > 16 && size <= 32){
        memb_free(&bytes4_memb, value->bytes);
    }
    else if(size > 32 && size <= 64){
        memb_free(&bytes8_memb, value->bytes);
    }
    else if(size > 64 && size <= 128){
        memb_free(&bytes16_memb, value->bytes);        
    }
}

void deallocate_rule(rule_t* rule){
    if(rule == NULL)
        return;
    deallocate_value_field(&rule->value, rule->size);
    memb_free(&rules_memb, rule);
}

void deallocate_action(action_t* action){
    if(action == NULL)
        return;
    deallocate_value_field(&action->value, action->size);
    memb_free(&actions_memb, action);
}

void deallocate_entry(entry_t* entry){
    rule_t* rule = NULL;
    action_t* action = NULL;
    if(entry == NULL)
        return;
    
    rule = list_pop(entry->rules);
    while(rule != NULL){
        deallocate_rule(rule);
        rule = list_pop(entry->rules);
    }
    
    action = list_pop(entry->actions);
    while(action != NULL){
        deallocate_action(action);
        action = list_pop(entry->actions);
    }
    memb_free(&entries_memb,entry);
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
    r = memb_alloc(&rules_memb);
    if(r == NULL) {
        PRINTF("[FLT]: Failed to allocate a rule\n");
        return NULL;
    }
    memset(r, 0, sizeof(*r));
    return r;
}

uint8_t* create_value_field(uint8_t size, uint8_t* value){
    uint8_t* bytes;
    uint8_t dim;
    if(size == 0)
        return NULL;
    
    if(size > 8 && size <= 16){
        bytes = memb_alloc(&bytes2_memb);
        dim = 2;
    }
    else if(size > 16 && size <= 32){
        dim = 4;
        bytes = memb_alloc(&bytes4_memb);
    }
    else if(size > 32 && size <= 64){
        dim = 8;
        bytes = memb_alloc(&bytes8_memb);
    }
    else if(size > 64 && size <= 128){
        dim = 16;
        bytes = memb_alloc(&bytes16_memb);
    }
    if(bytes == NULL) {
        return NULL;
    }  
    memcpy(bytes, value, dim);
    
    
}

action_t* create_action(action_type_t type, field_t field, uint8_t offset, uint8_t size, uint8_t* value){
    action_t* a;
    if(size < 0 || size > 128 || offset < 0 || offset > 128){
        PRINTF("[FLT]: Failed to create an action: invalid parameters\n");
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
    else{
        a->value.bytes = create_value_field(size, value);
        if(a->value.bytes == NULL){
            PRINTF("[FLT]: Failed to allocate the field value\n");
            deallocate_action(a);
            return NULL;
        }
    }
    return a;
}

rule_t* create_rule(field_t field, uint8_t offset, uint8_t size, operator_t operator, uint8_t* value){
    rule_t *r;
    uint8_t dim;
    if(size < 0 || size > 128 || offset < 0 || offset > 128 || value == NULL){
        PRINTF("[FLT]: Failed to create a rule: invalid parameters\n");
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
    else{
        r->value.bytes = create_value_field(size, value);
        if(r->value.bytes == NULL){
            PRINTF("[FLT]: Failed to allocate the field value\n");
            deallocate_rule(r);
            return NULL;
        }
    }    
    return r;
}

entry_t* create_entry(uint16_t priority){
    entry_t* entry;
    if(priority < 0){
        PRINTF("[FLT]: Failed to create a new entry: invalid parameters\n");
        return NULL;
    }
    entry = allocate_entry();
    if(entry == NULL)
        return NULL;
    entry->priority = priority;
    return entry;
}

uint8_t add_rule_to_entry(entry_t* entry, rule_t* rule){
    if(entry == NULL){
        PRINTF("[FLT]: Failed to add a new rule: invalid parameters\n");
        return 0;
    }
    if(rule != NULL)
        list_add(entry->rules, rule);
}

uint8_t add_action_to_entry(entry_t* entry, action_t* action){
    if(entry == NULL){
        PRINTF("[FLT]: Failed to add a new action: invalid parameters\n");
        return 0;
    }
    if(action != NULL)
        list_add(entry->actions, action);
}

uint8_t add_entry_to_ft(entry_t* entry){
    entry_t* tmp = NULL;
    entry_t* help = NULL;
    uint16_t priority;
    
    if(entry->priority == 0){                //Priority equals to 0 is not allowed because 0 is the "NULL" value of this field
        PRINTF("[FLT]: Failed to insert the entry: invalid priority\n");
        return 0;
    }
    priority = entry->priority;
    
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
        PRINTF("VAL: %x ", (char)a->value.byte);
    else{
        int i, dim;
        dim = a->size / 8;
        if(a->size % 8 > 0)
            dim += 1;
        PRINTF("VAL: ");
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
        case EQUAL: PRINTF("== "); break;
        case NOT_EQUAL: PRINTF("!= "); break;
        case GREATER: PRINTF("> "); break;
        case LESS: PRINTF("< "); break;
        case GREATER_OR_EQUAL: PRINTF(">= "); break;
        case LESS_OR_EQUAL: PRINTF("<= "); break;
    }
    if(r->size <= 8)
        PRINTF("VAL: %x ", (char)r->value.byte);
    else{
        int i, dim;
        dim = r->size / 8;
        if(r->size % 8 > 0)
            dim += 1;
        PRINTF("VAL: ");
        for(i = 0; i < dim; i++)
            PRINTF("%02x", r->value.bytes[i]);
    }
    PRINTF(" OFFSET: %u ", r->offset);
    PRINTF("SIZE: %u ", r->size);
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
    int i;
    uint8_t dim;
    uint8_t* status_register = getStatusRegisterPtr(&dim);
    PRINTF("\nNODE STATE: ");
    for(i = 0; i < dim; i++)
            PRINTF("%02x", status_register[i]);
    PRINTF("\n");
    for(e = list_head(flowtable); e != NULL; e = e->next) {
        PRINTF("[FLT]: ");
        print_entry(e);
        PRINTF("\n");  
    }
}

entry_t* getFlowTableHead(){
    return list_head(flowtable);
}

uint8_t compare_rules(rule_t* rule1, rule_t* rule2){
    int dim, res;
    if(rule1 == NULL || rule2 == NULL)
        return 0;
    if(rule1->offset == rule2->offset &&
            rule1->size == rule2->size &&
            rule1->operator == rule2->operator &&
            rule1->field == rule2->field){
        
        dim = rule1->size / 8; 
        if(rule1->size % 8 != 0)
            dim += 1;
        if(dim == 1)
            res = memcmp(&rule1->value.byte, &rule2->value.byte, dim);
        else
            res = memcmp(rule1->value.bytes, rule2->value.bytes, dim);
        if(res == 0)
            return 1;
    }
    return 0;           
}

uint8_t compare_actions(action_t* action1, action_t* action2){
    int dim, res;
    if(action1 == NULL || action2 == NULL)
        return 0;
    if(action1->offset == action2->offset &&
            action1->size == action2->size &&
            action1->type == action2->type &&
            action1->field == action2->field){
        
        dim = action1->size / 8; 
        if(action1->size % 8 != 0)
            dim += 1;
        
        if(dim == 1)
            res = memcmp(&action1->value.byte, &action2->value.byte, dim);
        else
            res = memcmp(action1->value.bytes, action2->value.bytes, dim);
        if(res == 0)
            return 1;
    }
    return 0;           
}

entry_t* find_entry(entry_t* entry_to_match){
    entry_t* entry = NULL;
    rule_t* rule = NULL;
    action_t* action = NULL;
    rule_t* rule_to_match = NULL;
    rule_t* rule_list = NULL;
    action_t* action_to_match = NULL;
    action_t* action_list = NULL;
    int num_rules;
    int num_actions;
    
    if(entry_to_match == NULL)
        return NULL;
    
    rule_list = list_head(entry_to_match->rules);
    num_rules = list_length(entry_to_match->rules);
    action_list = list_head(entry_to_match->actions);
    num_actions = list_length(entry_to_match->actions);
    
    //Search the entry with the same rules as the parameter
    for(entry = list_head(flowtable); entry != NULL; entry = entry->next){
        if(num_rules != list_length(entry->rules))
            continue;
        //Initialize support pointers
        rule_to_match = rule_list;
        action_to_match = action_list;
        
        //Check if the priority is set and, in case, if it is equal to the entry's one
        //If the entry to match has priority equals to 0 it means that any priority value is fine
        if(entry_to_match->priority != 0 && entry_to_match->priority != entry->priority){
            continue;
        }
        
        //For each rule
        for(rule = list_head(entry->rules); rule != NULL; rule = rule->next){
            
            //Compare the two rules and see if they are equal
            if(compare_rules(rule, rule_to_match) == 1){
                //Match! Check the next rule
                rule_to_match = rule_to_match->next;  
            }
            else
                //No match, go to the next table entry
                break;
        }
        if(rule != NULL)       //Premature exit from the loop means that at least one rule doesn't match
            continue;
        
        //If we are here it means that all the rules are equal to the reference ones
        //Now, check the actions
        for(action = list_head(entry->actions); action != NULL; action = action->next){
                       
            //Compare the two rules and see if they are equal
            if(compare_actions(action, action_to_match) == 1){
                //Match! Check the next rule
                action_to_match = action_to_match->next;  
            }
            else
                //No match, go to the next table entry
                break;
        }
        if(action == NULL)          //If the loop reached the end of the actions list
            break;                  //it means that every action has matched the reference ones
                                    //thus, we have found the right entry
    }
    if(entry == NULL)               //If the loop reached the end of the flow table
        return NULL;                //it means that there's no entry which has matched
    return entry;
}


uint8_t remove_entry(entry_t* entry_to_match){
    entry_t* entry;
    if(entry_to_match == NULL)
        return 0;
    
    entry = find_entry(entry_to_match);
    if(entry == NULL)
        return 0;
    
    list_remove(flowtable, entry);
    deallocate_entry(entry);
    return 1;
}

void flowtable_test(){
    uint8_t addr_tunslip[8]  = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t addr_1[8]  = {0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01};
    uint8_t addr_2[8]  = {0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02};
    uint8_t addr_3[8]  = {0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03};
    uint8_t addr_4[8]  = {0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04};
    uint8_t addr_5[8]  = {0xc1, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05};
    //uint8_t addr_6[8]  = {0xc1, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06};
    //uint8_t value = 3;
    rule_t* rule;
    action_t* action;
    entry_t* entry;
    
    if(memcmp(&linkaddr_node_addr, addr_1, 8) == 0){
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_3);
        action= create_action(FORWARD, NO_FIELD, 0, 64, addr_2);
        add_rule_to_entry(entry, rule);    
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
        
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_4);
        action= create_action(FORWARD, NO_FIELD, 0, 64, addr_2);
        add_rule_to_entry(entry, rule);    
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
    }
    
    if(memcmp(&linkaddr_node_addr, addr_2, 8) == 0){
        /*Assumption: the DODAG root's MAC address is known.
         * Without this static rule it is impossible to communicate with the external
         * virtual interface (FD00::1), called "tunslip", attached to the border router.
         * The problem comes from the fact that the border router takes the 
         * IP prefix from this virtual interface, and it uses that prefix (FD00) 
         * inside the RPL context.
         * For this reason, the IP address of the tunslip host is recognized by the
         * nodes as an on-link address, whereas the host is off-link.
         * Thus the nodes send packets using the tunslip host's mac address 
         * as final destination in the Mesh Header, though finding no matching
         * rules inside the flow table, preventing the nodes to communicate 
         * with external hosts, and one of them could be the SDN Controller.
         * To fix this issue single static rule is required: if the final address
         * of the mesh header is equal to the tunslip host's one then, modify it 
         * with the mac address of the dodag root, and continue to scan the 
         * flow table.
         * This way, we can exploit the dynamic rules added through RPL
         * in order to send packets toward the border router and, at the same 
         * time, avoiding inserting static paths. 
         */
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_tunslip);
        add_rule_to_entry(entry, rule);
        action= create_action(MODIFY, MH_DST_ADDR, 0, 64, addr_1);
        add_action_to_entry(entry, action);
        action= create_action(CONTINUE, NO_FIELD, 0, 0, NULL);
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
        
    }
    
    if(memcmp(&linkaddr_node_addr, addr_3, 8) == 0){
        //Rule for the tunslip host destination
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_tunslip);
        add_rule_to_entry(entry, rule);
        action= create_action(MODIFY, MH_DST_ADDR, 0, 64, addr_1);
        add_action_to_entry(entry, action);
        action= create_action(CONTINUE, NO_FIELD, 0, 0, NULL);
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
    }
    
    if(memcmp(&linkaddr_node_addr, addr_4, 8) == 0){
        //Rule for the tunslip host destination
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_tunslip);
        add_rule_to_entry(entry, rule);
        action= create_action(MODIFY, MH_DST_ADDR, 0, 64, addr_1);
        add_action_to_entry(entry, action);
        action= create_action(CONTINUE, NO_FIELD, 0, 0, NULL);
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
        
        /*
        entry = create_entry(100);
        rule = NULL;
        action= create_action(FORWARD, NO_FIELD, 0, 64, addr_2);
        add_rule_to_entry(entry, rule);    
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
        */
        
    }
    
    /*
    if(memcmp(&linkaddr_node_addr, addr_2, 8) == 0){
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_1);
        //rule = create_rule(MH_DST_ADDR, 54, 10, EQUAL, addr_1 + 6);  //Last 10 bits of the 64-bit long MAC address
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
    */
    /*
    if(memcmp(&linkaddr_node_addr, addr_3, 8) == 0){
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_1);
        action= create_action(FORWARD, NO_FIELD, 0, 64, addr_4);
        add_rule_to_entry(entry, rule);    
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
    }
    */
    /*
    if(memcmp(&linkaddr_node_addr, addr_4, 8) == 0){
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_3);
        action= create_action(FORWARD, NO_FIELD, 0, 64, addr_2);
        add_rule_to_entry(entry, rule);    
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
    }
    */
    /*
    if(memcmp(&linkaddr_node_addr, addr_5, 8) == 0){
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_1);
        action= create_action(FORWARD, NO_FIELD, 0, 64, addr_2);
        add_rule_to_entry(entry, rule);    
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
    }
    */
    /*
    if(memcmp(&linkaddr_node_addr, addr_6, 8) == 0){
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_1);
        action= create_action(FORWARD, NO_FIELD, 0, 64, addr_2);
        add_rule_to_entry(entry, rule);    
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
    }    
    */
    /*
    if(memcmp(&linkaddr_node_addr, addr_6, 8) == 0){
        entry = create_entry(1);
        rule = create_rule(MH_DST_ADDR, 54, 10, EQUAL, addr_1 + 6);
        add_rule_to_entry(entry, rule);   
        action= create_action(MODIFY, NODE_STATE, 45, 2, &value);         
        add_action_to_entry(entry, action);
        entry->stats.ttl=20;
        add_entry_to_ft(entry);
        
        entry = create_entry(2);
        rule = create_rule(NODE_STATE, 45, 2, EQUAL, &value);
        add_rule_to_entry(entry, rule);   
        action= create_action(MODIFY, NODE_STATE, 0, 64, addr_3);         
        add_action_to_entry(entry, action);
        entry->stats.ttl=20;
        add_entry_to_ft(entry);
        
        entry = create_entry(3);
        rule = create_rule(NODE_STATE, 0, 64, EQUAL, addr_3);
        add_rule_to_entry(entry, rule);   
        action= create_action(MODIFY, NODE_STATE, 64, 64, addr_4);         
        add_action_to_entry(entry, action);
        entry->stats.ttl=20;
        add_entry_to_ft(entry);
    }
    */
    //print_flowtable();
}

