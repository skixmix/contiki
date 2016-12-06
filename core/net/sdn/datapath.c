/**
 * \file
 *         datapath.c
 * \author
 *         Giulio Micheloni <giulio.micheloni@gmail.com>
 */

#include "datapath.h"


uint8_t status_register[STATUS_REGISTER_SIZE];
uint8_t* ptr_to_packet;

uint8_t* getStatusRegisterPtr(uint8_t* dim){
    *dim = STATUS_REGISTER_SIZE;
    return status_register;
}

//Copy the value of a field from the packet into the copy_buffer
uint8_t getFieldFromPacket(field_t field, uint8_t* copy_buffer, uint8_t* buf_occupation){
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
            if(parseMeshHeader(ptr_to_packet, NULL, NULL, NULL, (linkaddr_t*)copy_buffer, buf_occupation) > 0)
                return 1;
            break;
        case MH_DST_ADDR: 
            if(parseMeshHeader(ptr_to_packet, NULL, (linkaddr_t*)copy_buffer, buf_occupation, NULL, NULL) > 0)
                return 1;
            break;
        case MH_HL:  
            break;
        case NODE_STATE: 
            //Not implemented here, but in the calling function
            break;
    }
    return 0;
}

//Return a pointer to the provided field inside the packet buffer, or the pointer to the state buffer
uint8_t* getFieldPtr(field_t field, uint8_t* dim){
    switch(field){
        case LINK_SRC_ADDR: 
            if(dim != NULL)
                dim = LINKADDR_SIZE;
            return packetbuf_addr(PACKETBUF_ADDR_SENDER);
        case LINK_DST_ADDR: 
            if(dim != NULL)
                dim = LINKADDR_SIZE;
            return packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
        case MH_SRC_ADDR:
            break;
        case MH_DST_ADDR: 
            break;
        case NODE_STATE: 
            if(dim != NULL)
                dim = STATUS_REGISTER_SIZE;
            return status_register;
    }
    return 0;
}

/* This function extracts the interested bits window from the copy_buffer
 * and it moves this window to the beginning of the copy_buffer.
 * If the window is smaller than 8 bits, it also moves these bits at the 
 * least significant position inside the byte.
 * 
 * Example:
 * Size: 3
 * Offset: 11
 * ----------Offset------|---|                                  |---|
 *           ----------------------------------            -----------------------------------
 * Buffer:   |01100011|00110101|01010100|.....|      ==>   |00000101|00110101|01010100|.....|
 *           ----------------------------------            -----------------------------------
 * 
 * Example:
 * Size: 10
 * Offset: 11
 * ----------Offset------|-----------|                        |-----------|
 *           ----------------------------------            -----------------------------------
 * Buffer:   |01100011|00110101|01010101|.....|      ==>   |00010101|01010100|00000000|.....|
 *           ----------------------------------            -----------------------------------
 */
uint8_t applyMask(uint8_t size, uint8_t offset, uint8_t* copy_buffer, uint8_t buf_occupation){
    uint8_t first_byte, last_byte;
    uint8_t i, j, mask;
    first_byte = offset / 8;
    last_byte = (size + offset) / 8;
    //Offset and size are mesured in number of bits
    if(first_byte >= buf_occupation) 
        //It's trying to access a portion of the buffer which has no meaning
        return 0;
    if(last_byte > buf_occupation)
        //It's trying to access a portion of the buffer which has no meaning
        return 0;
    if(last_byte == buf_occupation && (size + offset) % 8 != 0)
        //It's trying to access a portion of the buffer which has no meaning
        return 0;
    
    //If the bits window that we want to compare is less than 1 byte
    if(size <= 8){
        //If the window is aligned to the byte
        if((offset + size) % 8 == 0){
            //Just apply the mask to set to 0 the bits we don't need
            mask = (0xff >> (offset % 8));
            copy_buffer[first_byte] &= mask;
        }
        else{
            //Same concept as above: prepare a mask (e.g. 00011100) to set to 0
            //the bits we don't need 
            mask = (0xff >> (offset % 8)) & (0xff << (8 - (offset + size) % 8));
            copy_buffer[first_byte] &= mask;
            //Since the bits windows is not aligned we move it at the least significant 
            //position inside the byte
            copy_buffer[first_byte] >>= (8 - (offset + size) % 8);
        }
    }
    //Otherwise the bits window is greater than 1 byte
    else{
        //From the first byte containing the window, set to 0 the 
        //most significant bits we don't need (e.g. 10010101 ---> 00000101)
        copy_buffer[first_byte] &= (1 << (8 - (offset % 8))) - 1;
        //From the last byte containing the window, set to 0 the  
        //least significant bits we don't need (e.g. 11011011 ---> 11010000)
        copy_buffer[last_byte] &= 0xff << (8 - ((offset + size) % 8));
        //Set to 0 the rest of the buffer not concerning the bits window
        for(i = 0; i < buf_occupation; i++){
            if(i < first_byte)
                copy_buffer[i] = 0;
            else if(i > last_byte)
                copy_buffer[i] = 0;
        }
    }
    //Independently of the size of the window
    //move the byte(s) containing the window at the beginning of the buffer
    j = 0;
    for(i = first_byte; i <= last_byte; i++){
        copy_buffer[j] = copy_buffer[i];
	j++;
    }
    
    return 1;
}

//Function that provided the rule, it evaluates the condition 
//and returns 1, if it is satisfied or 0 if it is not
uint8_t evaluateRule(rule_t* rule){
    uint8_t res;
    uint8_t copy_buffer[COPY_BUFFER_SIZE];
    uint8_t buf_occupation;
    uint8_t dim, first_byte;
    
    if(rule == NULL)
        return 0;
    
    //The final dimension measured in bytes
    dim = rule->size  / 8;
    //If the size is not aligned to the byte it involves that we need a byte more
    //Or if the size is less than a byte
    if(rule->size % 8 > 0)
        dim += 1;
    //Since the status buffer has a dimension much more greater than the copy_buffer
    //we handle this special case in a different way
    if(rule->field == NODE_STATE){ 
        //Derive the first byte that contains the most significant part of the bits window
        first_byte = rule->offset / 8;
        buf_occupation = COPY_BUFFER_SIZE;
        //Copy the part of the status register that contains the
        //entire bits window into the copy_buffer
        memcpy(copy_buffer, status_register + first_byte, buf_occupation);
        //Apply the mask provided by the size and offset fields
        //but scale the offset to address correctly the copy_buffer, since the 
        //bits window is stored from the first byte of the copy_buffer
        res = applyMask(rule->size, rule->offset % 8, copy_buffer, buf_occupation);
        if(res != 1){
            printf("Datapath - applyMask: error in applying the mask\n");
            return 0;
        }
    }
    //Otherwise retrieve the field from the packet
    else{
        //Copy the value of the field into the copy_buffer
        res = getFieldFromPacket(rule->field, copy_buffer, &buf_occupation);
        if(res != 1){
            printf("Datapath - evaluateRule: the field is not present\n");
            return 0;
        }
        //Apply the bits mask as specified by the size and offset fields
        res = applyMask(rule->size, rule->offset, copy_buffer, buf_occupation);
        if(res != 1){
            printf("Datapath - applyMask: error in applying the mask\n");
            return 0;
        }
    }
    
    
    //DEBUG
    if(rule->size <= 8)
        printf(" %02x",rule->value.byte);
    else
        print_ll_addr(rule->value.bytes);
    printf("\n");
    print_ll_addr(copy_buffer);
    printf("\n");
    //DEBUG
    
    //Carry out the actual comparison
    if(dim == 1)        //If the bits window is equal or less than a byte
        res = memcmp(copy_buffer, &rule->value.byte, dim);
    else                //Or if it is greater than 1 byte
        res = memcmp(copy_buffer, rule->value.bytes, dim);
    //Apply the operator and see if the result fits the condition
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

//Function that implements the forward type of action
uint8_t forward_action(action_t* action){
    if(action == NULL || action->size != 64 || action->value.bytes == NULL)
        return 0;
        
    printf("FORWARD TO: ");
    print_ll_addr(action->value.bytes);
    printf("\n");    
    
    //Set the next hop mac address
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, action->value.bytes);
    //Pass the control to the SDN module
    forward();
    return 1;
}

//Given the pointer to field and pointer to the buffer containing the new value
//this function writes the new value inside the field, taking care of 
//the position and size of the bits window, represented by size and offset parameters
uint8_t write_field(uint8_t* field, uint8_t fieldDim, uint8_t* value, uint8_t size, uint8_t offset){
    uint8_t first_byte, last_byte;
    uint8_t carry = 0;
    uint8_t mask;
    int i, j;
    
    //Derive the index of the first and last byte that contain the terminal parts of the window
    first_byte = offset / 8;
    last_byte = (size + offset) / 8;
    
    if(field == NULL || value == NULL)
        return 0;    
    //Offset and size are mesured in number of bits
    if(first_byte >= fieldDim) 
        //It's trying to access a portion of the buffer which has no meaning
        return 0;
    if(last_byte > fieldDim)
        //It's trying to access a portion of the buffer which has no meaning
        return 0;
    if(last_byte == fieldDim && (size + offset) % 8 != 0)
        //It's trying to access a portion of the buffer which has no meaning
        return 0;
    
    //If the window size is equal or less than a byte
    if(size <= 8){
        //Prepare a mask (e.g. 11100111) to set to 0 the bits that have to be overwritten  
        mask = ~((0xff >> (offset % 8)) & (0xff << (8 - (offset + size) % 8)));
        field[first_byte] &= mask;
        //Move the bits of the value byte at the right position and write their value into the field
        field[first_byte] |= *value << (8 - (offset + size) % 8);
    }
    //Otherwise window size is greater than 1 byte
    else{
        //Clean the memory space where the window of bits will be copied
        field[first_byte] &= ~((1 << (8 - (offset % 8))) - 1);      //Set to 0 the least significant bits
        field[last_byte] &= ~(1 << (8 - ((offset + size) % 8)));    //Set to 0 the most significant bits
        for(i = first_byte + 1; i < last_byte; i++){        
            field[i] = 0;
        }
        //Set the window of bits uqual to the value taking care of non-aligned bits 
        j = 0;
        for(i = first_byte; i <= last_byte; i++){
            field[i] |= carry << (8  - (offset % 8));
            if(i != last_byte)
                field[i] |= value[j] >> (offset % 8);
            carry = value[j] & ((1 << (offset % 8)) - 1);
            j++;
        }
    }
    return 1;
}

//Function that implements the modify type of action
uint8_t modify_action(action_t* action){
    uint8_t* field_ptr;
    uint8_t dimField;
    uint8_t ret;
    if(action == NULL || action->size == 0)
        return 0;
    if(action->size > 8 && action->value.bytes == NULL)
        return 0;
    
    //Retrieve the pointer to the field that has to be modified
    field_ptr = getFieldPtr(action->field, &dimField);
    if(field_ptr == NULL)
        return 0;
    if(action->size <= 8)
        ret = write_field(field_ptr, dimField, &action->value.byte, action->size, action->offset);
    else
        ret = write_field(field_ptr, dimField, action->value.bytes, action->size, action->offset);
    if(ret == 0)
        printf("Datapath - modify_action: failed while writing the new value");
    return ret;
}

//Function that implements each type if action
uint8_t applyAction(action_t* action){
    if(action == NULL)
        return 0;
    
    switch(action->type){
        case FORWARD:
            return forward_action(action);
            break;
        case DROP:
            //TODO: deallocate the packet buffer?
            //packetbuf_clear();
            //TODO: Prevent any further action on this packet
            break;
        case MODIFY: 
            return modify_action(action);
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
            //TODO: the packet could have been modified, we need a copy of the buffer
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

