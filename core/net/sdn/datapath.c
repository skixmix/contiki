/**
 * \file
 *         datapath.c
 * \author
 *         Giulio Micheloni <giulio.micheloni@gmail.com>
 */

#include "datapath.h"
#include "flowtable.h"
#include "sdn.h"
#include "net/ipv6/sicslowpan.h"
#include "net/packetbuf.h"
#include "net/linkaddr.h"

uint8_t status_register[STATUS_REGISTER_SIZE];
uint8_t* ptr_to_packet;


uint8_t getField(field_t field, uint8_t* copy_buffer, uint8_t* buf_occupation){
    switch(field){
        case LINK_SRC_ADDR: 
            linkaddr_copy((linkaddr_t *)copy_buffer, packetbuf_addr(PACKETBUF_ADDR_SENDER));
            *buf_occupation = LINKADDR_SIZE;
            return 1;
        case LINK_DST_ADDR: 
            linkaddr_copy((linkaddr_t *)copy_buffer, packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
            *buf_occupation = LINKADDR_SIZE;
            return 1;
        case MH_SRC_ADDR:
            if(parseMeshHeader(ptr_to_packet, NULL, NULL, NULL, (linkaddr_t*)copy_buffer, buf_occupation) > 0);
                return 1;
            break;
        case MH_DST_ADDR: 
            if(parseMeshHeader(ptr_to_packet, NULL, (linkaddr_t*)copy_buffer, buf_occupation, NULL, NULL) > 0);
                return 1;
            break;
        case MH_HL:  
            break;
        case NODE_STATE: 
            break;
    }
    return 0;
}

uint8_t applyMask(uint8_t size, uint8_t offset, uint8_t* copy_buffer, uint8_t buf_occupation){
    uint8_t first_byte, last_byte;
    uint8_t i;
    first_byte = offset / 8;
    last_byte = (size + offset) / 8;
    //Offset and size are mesured in number of bits
    if(first_byte >= buf_occupation) 
        //It's trying to access a portion of the buffer which has no meaning
        return 0;
    if(last_byte > buf_occupation)
        //It's trying to access a portion of the buffer which has no meaning
        return 0;
    if(last_byte == buf_occupation && size % 8 != 0)
        //It's trying to access a portion of the buffer which has no meaning
        return 0;
    
    copy_buffer[first_byte] &= (1 << (8 - (offset % 8))) - 1;
    copy_buffer[last_byte] &= 0xff << (8 - ((offset + size) % 8));
    for(i = 0; i < buf_occupation; i++){
        if(i < first_byte)
            copy_buffer[i] = 0;
        else if(i > last_byte)
            copy_buffer[i] = 0;
    }
    return 1;
}

uint8_t evaluateRule(rule_t* rule){
    uint8_t res;
    uint8_t copy_buffer[COPY_BUFFER_SIZE];
    uint8_t buf_occupation;
    uint8_t dim;
    
    if(rule == NULL)
        return 0;
    dim = ((rule->size + rule->offset) / 8);
    if((rule->size + rule->offset) % 8 > 0)
        dim += 1;
    
    res = getField(rule->field, copy_buffer, &buf_occupation);
    if(res != 1)
        return 0;
    res = applyMask(rule->size, rule->offset, copy_buffer, buf_occupation);
    if(res != 1)
        return 0;
    
    print_ll_addr(rule->value.bytes);
    printf("\n");
    print_ll_addr(copy_buffer);
    printf("\n");
    
    if(dim == 1)
        res = memcmp(copy_buffer, &rule->value.byte, dim);
    else
        res = memcmp(copy_buffer, rule->value.bytes, dim);
    switch(rule->operator){
        case EQUAL:
            if(res == 0)
                return 1;
            break;
        case NOT_EQUAL:
            if(res != 0)
                return 1;
            break;
        case GREATER: 
            if(res > 0)
                return 1;
            break;
        case LESS:
            if(res < 0)
                return 1;
            break;
        case GREATER_OR_EQUAL: 
            if(res >= 0)
                return 1;
            break;
        case LESS_OR_EQUAL:
            if(res <= 0)
                return 1;
            break;
        
    }
    
    return 0;
}

void updateStat(stats_t* stat){
    stat->count++;
}

uint8_t forward_action(action_t* action){
    printf("FORWARD TO: ");
    print_ll_addr(action->value.bytes);
    printf("\n");    
    
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, action->value.bytes);
    packetbuf_set_addr(PACKETBUF_ADDR_SENDER,(void*)&uip_lladdr);   //TODO: remove it
    forward();
}

uint8_t applyAction(action_t* action){
    switch(action->type){
        case FORWARD:
            return forward_action(action);
            break;
        case DROP: 
            break;
        case MODIFY: 
            break;
        case DECREMENT: 
            break;
        case INCREMENT: 
            break;
        case CONTINUE: 
            break;
        case TO_UPPER_L: 
            break;
        default: 
            PRINTF("NO_ACTION ");            
    }
    return 0;
}

int matchPacket(){
    entry_t* entry;
    rule_t* rule;
    action_t* action;
    uint8_t result;
    uint8_t continue_flag;
    ptr_to_packet = packetbuf_dataptr();
    printf("MATCH PACKET:\n");
    //Scan the entire flow table, one entry at a time 
    for(entry = getFlowTableHead(); entry != NULL; entry = entry->next){
        continue_flag = 0;
        //DEBUG
        print_entry(entry);
        printf("\n");
        //Check if every condition is satisfied
        for(rule = list_head(entry->rules); rule != NULL; rule = rule->next){
            //Assess the rule
            result = evaluateRule(rule);
            if(result == 0) //If the rule is not satisfied, go to the next entry
                break;
            printf("MATCH!\n");
        }
        if(rule != NULL)    //This means that at least one rule has not been satisfied
            continue;
        //If we are here it means that all rules have been satisfied, so let's apply the actions
        for(action = list_head(entry->actions); action != NULL; action = action->next){
            result = applyAction(action);
            if(action->type == CONTINUE)
                continue_flag = 1;
        }
        updateStat(&entry->stats);
        //Unless there was a "continue" action, stop scanning the flow table 
        if(continue_flag != 1)
            break;
    }
    /* TODO activate the table miss handling
    if(entry == NULL)   //This means that the entire table has been looked up
        tableMiss();    //without finding any right entry for this packet
    */
}

