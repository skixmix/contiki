/**
 * \file
 *         control_agent.c
 * \author
 *         Giulio Micheloni <giulio.micheloni@gmail.com>
 */

#include "net/sdn/control_agent.h"


//Called from the RPL module when it chooses a (new) parent
void sdn_rpl_callback_parent_switch(rpl_parent_t *old, rpl_parent_t *new){
    uint8_t addrDim;
    linkaddr_t llAddrParent;
    linkaddr_t llAddrRoot;
    rule_t* rule;
    action_t* action;
    entry_t* entry;
    
    
    //Check if there was an old parent
    if(old != NULL){
        //If so, we must remove its entry from the flow table
        extractIidFromIpAddr(&llAddrParent, rpl_get_parent_ipaddr(old), &addrDim);
        extractIidFromIpAddr(&llAddrRoot, &old->dag->dag_id, &addrDim);
        
        entry = create_entry(50);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, &llAddrRoot);
        add_rule_to_entry(entry, rule); 
        action= create_action(FORWARD, NO_FIELD, 0, 64, &llAddrParent);        
        add_action_to_entry(entry, action);
        remove_entry(entry);  
        
        /* DEBUG
        printf(" Removed parent: ");
        print_ll_addr(&llAddrParent);
        printf(" Root: ");
        print_ll_addr(&llAddrRoot);    
        printf("\n");
        */
    }
    
    
    //If the node has selected a new parent, add the rule how to reach 
    //the root of the DODAG into the flow table
    if(new != NULL){
        extractIidFromIpAddr(&llAddrParent, rpl_get_parent_ipaddr(new), &addrDim);
        extractIidFromIpAddr(&llAddrRoot, &new->dag->dag_id, &addrDim);
        
        entry = create_entry(50);
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, &llAddrRoot);
        add_rule_to_entry(entry, rule); 
        action= create_action(FORWARD, NO_FIELD, 0, 64, &llAddrParent);        
        add_action_to_entry(entry, action);
        add_entry_to_ft(entry);
        
    }
    /* DEBUG
    printf(" Mesh addr parent: ");
    print_ll_addr(&llAddrParent);
    printf(" Root: ");
    print_ll_addr(&llAddrRoot);    
    printf("\n");
    */
    print_flowtable();
}

//Called from the link-stat module when it receive a message from a neighbor
void sdn_callback_neighbor(const linkaddr_t *addr){
    rule_t* rule;
    action_t* action;
    entry_t* entry;
    entry_t* app;
    if(addr == NULL)
        return;
    
    //Create the relative flow entry
    entry = create_entry(60);
    rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr);
    add_rule_to_entry(entry, rule); 
    action= create_action(FORWARD, NO_FIELD, 0, 64, addr);        
    add_action_to_entry(entry, action);
    entry->stats.ttl = 100;
    //Check if this neighbor already exits into the flow table
    app = find_entry(entry);
    if(app != NULL){
        //It exists, so just update the ttl field
        app->stats.ttl = 100;
        deallocate_entry(entry);
    }
    else{
        //It doesnt't exist, so add it in to the flow table
        add_entry_to_ft(entry);
    }    
        
    printf("\nNeighbor: ");
    print_ll_addr(addr);
    printf("\n");
    
    print_flowtable();
}