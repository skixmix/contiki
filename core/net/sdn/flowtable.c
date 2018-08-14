/* 
 * File:   flowtable.c
 * Author: Giulio Micheloni
 *
 * Created on 10 novembre 2016, 15.21
 */


#include "flowtable.h"

#define SDN_STATS 0
#if SDN_STATS
#include <stdio.h>
#define PRINT_STAT(...) printf(__VA_ARGS__)
#define PRINT_STAT_LLADDR(addr) printf("%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7])
#else
#define PRINT_STAT(...)
#define PRINT_STAT_LLADDR(addr)
#endif

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

LIST(flowTable);
MEMB(entries_memb, entry_t, MAX_NUM_ENTRIES);
MEMB(actions_memb, action_t, MAX_NUM_ACTIONS);
MEMB(rules_memb, rule_t, MAX_NUM_RULES);
MEMB(bytes2_memb, bytes2_t, NUM_BYTES_2_BLOCKS);
MEMB(bytes4_memb, bytes4_t, NUM_BYTES_4_BLOCKS);
MEMB(bytes8_memb, bytes8_t, NUM_BYTES_8_BLOCKS);
MEMB(bytes16_memb, bytes16_t, NUM_BYTES_16_BLOCKS);


//Function by Simone
/*
void clean_up_oldest_entry(){
    //TODO: find the oldest flow table entry and clean up its space
    printf("--> Trying to free up some space\n");
    //Search for the entry with stats->count that is the smallest (count indicates the # of times an entry was utilized)
    uint16_t minimum_count;
    int start = 1;
    entry_t* candidate_entry;
    entry_t* entry;
    
    for(entry = getFlowTableHead(); entry != NULL; entry = entry->next){
        if(entry->priority > 1){
          if(start == 1){ //start
              minimum_count = (&entry->stats)->count;
              printf("START Count: %d | Priority: %d\n", minimum_count, entry->priority);
              start = 0;
          }

          printf("Count: %d | Priority: %d\n", minimum_count, entry->priority);

          if(minimum_count >= (&entry->stats)->count){
              printf("Updating minimum count -> %d\n", (&entry->stats)->count);
              minimum_count = (&entry->stats)->count; //Update minimum count
              candidate_entry = entry;
          }
       }
    }
    //Now we have the entry to delete
    printf(">> Current minimum count: %d\n", minimum_count); 
    deallocate_entry(candidate_entry); //Deallocate the entry
    printf(">> Entry deallocated\n");
}
*/


void flowtable_init(){
    list_init(flowTable);
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
        //clean_up_oldest_entry(); //Uncommented by Simone
        printf(">> Now retrying to allocate entry\n");
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
    
    return bytes;
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
    return 1;
}

uint8_t add_action_to_entry(entry_t* entry, action_t* action){
    if(entry == NULL){
        PRINTF("[FLT]: Failed to add a new action: invalid parameters\n");
        return 0;
    }
    if(action != NULL)
        list_add(entry->actions, action);
    return 1;
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
    for(tmp = list_head(flowTable); tmp != NULL; tmp = tmp->next){
        if(tmp->priority > priority)
            break;
        help = tmp;
    }
    //Last element of the list
    if(tmp == NULL && help != NULL)
        list_add(flowTable, entry);
    //First element of the list
    else if(help == NULL)
        list_push(flowTable, entry);
    //Insert the element in the middle of the list
    else{
        entry->next = help->next;
        help->next = entry;
    }
    return 1;
}

void print_action(action_t* a){
#if DEBUG == 1
    switch(a->type){
        case FORWARD: PRINTF("FORWARD "); break;
        case DROP: PRINTF("DROP "); break;
        case MODIFY: PRINTF("MODIFY "); break;
        case DECREMENT: PRINTF("DECREMENT "); break;
        case INCREMENT: PRINTF("INCREMENT "); break;
        case CONTINUE: PRINTF("CONTINUE "); break;
        case TO_UPPER_L: PRINTF("TO_UPPER_L "); break;
        case BROADCAST: PRINTF("BROADCAST "); break;
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
#endif
}

void print_rule(rule_t* r){
#if DEBUG == 1
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
#endif
}

void print_entry(entry_t* e) {
#if DEBUG == 1
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
    PRINTF("}[TTL: %d COUNT: %d]\n", e->stats.ttl, e->stats.count);   
#endif
}

void print_flowtable() {
#if DEBUG == 1
    entry_t *e;
    int i;
    uint8_t dim;
    uint8_t* status_register = getStateRegisterPtr(&dim);
    PRINTF("\nNODE STATE: ");
    for(i = 0; i < dim; i++)
            PRINTF("%02x", status_register[i]);
    PRINTF("\n");
    for(e = list_head(flowTable); e != NULL; e = e->next) {
        PRINTF("[FLT]: ");
        print_entry(e);
        PRINTF("\n");  
    }
#endif
}

entry_t* getFlowTableHead(){
    return list_head(flowTable);
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
    
    if(entry_to_match == NULL)
        return NULL;
    
    rule_list = list_head(entry_to_match->rules);
    num_rules = list_length(entry_to_match->rules);
    action_list = list_head(entry_to_match->actions);
    
    //Search the entry with the same rules as the parameter
    for(entry = list_head(flowTable); entry != NULL; entry = entry->next){
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
    
    list_remove(flowTable, entry);
    deallocate_entry(entry);
    return 1;
}

//It returns the pointer to the field "value" of the first rule contained in the installed entry
//In our scenario this first rule targets the MESH_DST_ADDR.
//Returning this value allows the Local Control Agent to remove the relative pending request  
uint8_t* install_flow_entry_from_cbor(cn_cbor* flowEntry){
    uint16_t priority, ttl;
    cn_cbor *support, *child, *inner;
    rule_t* rule;
    action_t* action;
    entry_t* entry;
    field_t field;
    uint8_t offset, size;
    operator_t operator;
    action_type_t actionType;
    uint8_t* value;
    if(flowEntry == NULL)
        return NULL;
    if(flowEntry->type == CN_CBOR_ARRAY){
        support = flowEntry->first_child;
        if(support == NULL)
            return NULL;
        priority = (uint16_t)support->v.uint;
        support = support->next;
        if(support == NULL)
            return NULL;
        ttl = (uint16_t)support->v.uint;
        //Create the flow entry
        entry = create_entry(priority);
        if(entry == NULL)
            return NULL;
        entry->stats.ttl = ttl;
        support = support->next;
        if(support == NULL){
            deallocate_entry(entry);
            return NULL;
        }
        if(support->type == CN_CBOR_ARRAY){          //Rules
            for (child = support->first_child; child != NULL; child = child->next) {
                if(child->type == CN_CBOR_ARRAY && child->length == 5){
                    inner = child->first_child;
                    field = (field_t)inner->v.uint;
                    inner = inner->next;
                    operator = (operator_t)inner->v.uint;
                    inner = inner->next;
                    offset = (uint8_t)inner->v.uint;
                    inner = inner->next;
                    size = (uint8_t)inner->v.uint;
                    inner = inner->next;
                    value = (uint8_t*)inner->v.str;
                    rule = create_rule(field, offset, size, operator, value);
                    if(rule == NULL){
                        deallocate_entry(entry);
                        return NULL;
                    }
                }
                add_rule_to_entry(entry, rule);
            }  
        }
        else{
            deallocate_entry(entry);
            return NULL;
        }
        support = support->next;
        if(support == NULL){
            deallocate_entry(entry);
            return NULL;
        }
        if(support->type == CN_CBOR_ARRAY){          //Actions
            for (child = support->first_child; child != NULL; child = child->next) {
                if(child->type == CN_CBOR_ARRAY && child->length == 5){
                    inner = child->first_child;
                    actionType = (action_type_t)inner->v.uint;
                    inner = inner->next;
                    field = (field_t)inner->v.uint;
                    inner = inner->next;
                    offset = (uint8_t)inner->v.uint;
                    inner = inner->next;
                    size = (uint8_t)inner->v.uint;
                    inner = inner->next;
                    value = (uint8_t*)inner->v.str;
                    action = create_action(actionType, field, offset, size, value);
                    if(action == NULL){
                        deallocate_entry(entry);
                        return NULL;
                    }
                }
                add_action_to_entry(entry, action);
            }  
        }
        else{
            deallocate_entry(entry);
            return NULL;
        }
    }
    /*--------------------------STAT-----------------------------*/
	#if PrintStatistics == 1
	   printf("FTable_Entry_Installed\n");
	#endif
	/*
#if PrintStatistics == 1
    printf("FTable_Entry_Install_");
    if(rule->size == 64)
        PRINT_STAT_LLADDR(rule->value.bytes);
    else if(rule->size == 16)
        printf("%02x:%02x", rule->value.bytes[6], rule->value.bytes[7]);            
	
    if(action->size = 64 && action->size != 0){
		printf("_");
        PRINT_STAT_LLADDR(action->value.bytes);
	}
    printf("\n");
#endif
*/
    /*-----------------------------------------------------------*/
    PRINTF("\nInstalled: ");
#if DEBUG == 1
    print_entry(entry);
#endif    
    PRINTF("\n");
    if(add_entry_to_ft(entry) != 1){
        deallocate_entry(entry);
        return NULL;
    }
    else{
        rule = list_head(entry->rules);
        if(rule->field == MH_DST_ADDR)
            return rule->value.bytes;
        else 
            return NULL;
    }
}

void set_up_br_rule(){
    uint8_t addr_tunslip[8]  = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    //uint8_t addr_udpServer[8]  = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
    rule_t* rule;
    action_t* action;
    entry_t* entry;
    
    /*UDP Server, not present in my configuration
    entry = create_entry(1);
    rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_udpServer);
    action= create_action(TO_UPPER_L, NO_FIELD, 0, 0, NULL);
    add_rule_to_entry(entry, rule);    
    add_action_to_entry(entry, action);
    add_entry_to_ft(entry);
    */
    entry = create_entry(2);
    rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_tunslip); //If it is for tunslip MAC
    action= create_action(TO_UPPER_L, NO_FIELD, 0, 0, NULL); //Accept the packet and send to upper layer
    add_rule_to_entry(entry, rule);    
    add_action_to_entry(entry, action);
    add_entry_to_ft(entry);
    printf("BR-Tunslip mapping rule added\n");
    
    //Drop if source == BR and dest == Tunslip
   /* entry = create_entry(1);
    rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_tunslip);
    add_rule_to_entry(entry, rule);    
    rule = create_rule(MH_SRC_ADDR, 0, 64, EQUAL, &linkaddr_node_addr);
    add_rule_to_entry(entry, rule);  
    action= create_action(DROP, NO_FIELD, 0, 0, NULL);
    add_action_to_entry(entry, action);
    add_entry_to_ft(entry);
    printf("Drop rule for BR -> Tunlip packets added\n");
    */ 
}

void flowtable_test(){
	uint8_t multicastMesh = 0x80;
    rule_t* rule;
    action_t* action;
    entry_t* entry;
    
    //Drop rule for multicast packets (per tutti i nodi, ma serve?)
    /*
    entry = create_entry(80);
    rule = create_rule(MH_DST_ADDR, 0, 3, EQUAL, &multicastMesh);
    action= create_action(DROP, NO_FIELD, 0, 0, NULL);
    add_rule_to_entry(entry, rule);    
    add_action_to_entry(entry, action);
    add_entry_to_ft(entry);
    printf("Drop rule for multicast packets added\n");
    */
    //Print the flow table
    print_flowtable();
}

