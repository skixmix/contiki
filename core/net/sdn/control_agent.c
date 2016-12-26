/**
 * \file
 *         control_agent.c
 * \author
 *         Giulio Micheloni <giulio.micheloni@gmail.com>
 */

#include "net/sdn/control_agent.h"


PROCESS(coap_client_process, "Coap Client");
static struct ctimer timer_ttl;
static struct ctimer timer_topology;
static struct ringbufindex requests_ringbuf;
static request_t requests_array[MAX_REQUEST];
uip_ipaddr_t server_ipaddr;

#define REMOTE_PORT     UIP_HTONS(COAP_DEFAULT_PORT)
#define SERVER_NODE(ipaddr)   uip_ip6addr(ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 0x0001) 

#define PRINTADDR(addr) printf(" %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x ", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7])
#define ADD_BYTE(buf, byte) *buf = byte
#define URI_QUERY_MAC_ADDR(buffer, addr) sprintf(buffer,"?mac=%02x%02x%02x%02x%02x%02x%02x%02x", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7])

static uint8_t payload[MAX_DIM_PAYLOAD];
static char uri_query[64];

const uint8_t msg[] = {0x84, 0x01, 0x18, 0x5f, 0x18, 0x2a, 0xa2, \
                    0x48, 0x02, 0x00, 0x01, 0x00, 0x00, 0x40, 0x00, 0x01, 0x82, 0x18, 0x3e, 0x18, 0x3a, \
                    0x48, 0x04, 0x02, 0x00, 0x00, 0x05, 0x00, 0x00, 0x02, 0x82, 0x18, 0x3e, 0x18, 0x3a};

void start_request(void){
    process_poll(&coap_client_process);
}

request_t* add_request(void){
  int index = ringbufindex_peek_put(&requests_ringbuf);
  printf("ADD INDEX=%i\n", index);
  if(index != -1){
      ringbufindex_put(&requests_ringbuf);
      return &requests_array[index];
  }
  return NULL;
}

request_t* get_next_request(void){
  int index = ringbufindex_get(&requests_ringbuf);
  printf("GET INDEX=%i\n", index);
  if(index != -1){
      return &requests_array[index];
  }
  return NULL;
}


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
        if(entry == NULL)
            return;
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, &llAddrRoot);
        if(rule == NULL){
            deallocate_entry(entry);
            return; 
        }
        action= create_action(FORWARD, NO_FIELD, 0, 64, &llAddrParent);
        if(action == NULL){
            deallocate_entry(entry);
            deallocate_rule(rule);
            return; 
        }
        add_rule_to_entry(entry, rule);
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
        if(entry == NULL)
            return;
        rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, &llAddrRoot);
        if(rule == NULL){
            deallocate_entry(entry);
            return; 
        }
        action= create_action(FORWARD, NO_FIELD, 0, 64, &llAddrParent);
        if(action == NULL){
            deallocate_entry(entry);
            deallocate_rule(rule);
            return; 
        }
        add_rule_to_entry(entry, rule);
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
    //print_flowtable();
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
    if(entry == NULL)
        return;
    rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr);
    if(rule == NULL){
        deallocate_entry(entry);
        return;
    }
    action= create_action(FORWARD, NO_FIELD, 0, 64, addr);
    if(action == NULL){
        deallocate_entry(entry);
        deallocate_rule(rule);
        return;
    }
    add_rule_to_entry(entry, rule);
    add_action_to_entry(entry, action);
    entry->stats.ttl = 60*10;       // 10 minutes
    //Check if this neighbor already exits into the flow table
    app = find_entry(entry);
    if(app != NULL){
        //We mustn't confuse with the rule installed by RPL, and it may happen if the root is our neighbour
        if(app->stats.ttl != 0){
            //It exists, so just update the ttl field
            app->stats.ttl = 60*10;      // 10 minutes
        }
        deallocate_entry(entry);
    }
    else{
        //It doesnt't exist, so add it in to the flow table
        add_entry_to_ft(entry);
    }    
    
    //print_flowtable();
}

static void callback_decrement_ttl(void *ptr){
    entry_t* entry, *app; 
    ctimer_reset(&timer_ttl);
    entry = getFlowTableHead();
    while(entry != NULL){
        if(entry->stats.ttl == 0){
            entry = entry->next;
            continue;
        }
        entry->stats.ttl--;
        if(entry->stats.ttl == 0){
            app = entry;
            entry = entry->next;
            remove_entry(app);
        }
        else
            entry = entry->next;
    }
    
    
}


void handleTableMiss(uint8_t ptr_to_pkt){
    
}

uint8_t computeNumOfNeighbours(){
    uint8_t num_of_neigh = 0;
    uip_ds6_nbr_t *nbr;
    for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL; nbr = nbr_table_next(ds6_neighbors, nbr)) {
        num_of_neigh++;
    }
    return num_of_neigh;
}

uint8_t prepare_payload_top_update(){
    uip_ds6_nbr_t *nbr;
    uip_lladdr_t *lladdr;
    struct link_stats * stats;
    uint8_t num_of_neigh = 0;
    uint8_t payload_dim = 0;
    uint16_t etx;
    int16_t rssi;
    //These are the Cbor structures for the first part of the POST payload and for each neighbour node.
    //Where there is 0x00 it means that it is a field which will be set later.
    uint8_t header[] = {0x84, 0x00, 0x18, 0x00, 0x18, 0x00, 0x00};
    uint8_t neighbour[] = {0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x82, 0x39, 0x00, 0x00, 0x19, 0x00, 0x00};
    
    //Get the number of neighbours, if it is 0 don't send any update to the Controller then
    num_of_neigh = computeNumOfNeighbours();
    if(num_of_neigh == 0)
        return 0;
    //Set the state information regarding the sending node  
    header[1] = 1;                  //Version of the update structure
    header[3] = 60;                 //Battery level
    header[5] = 30;                 //Queue utilization
    //Set the Cbor major type "Array" with the number of items
    header[6] = 0xa0 | num_of_neigh;
    //Copy the first part of the POST payload
    memcpy(payload, header, sizeof(header));
    payload_dim = sizeof(header);
    
    //Scan the neighbours table 
    for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL; nbr = nbr_table_next(ds6_neighbors, nbr)) {
        num_of_neigh++;
        //Get the neighbour's MAC address
        lladdr = uip_ds6_nbr_get_ll(nbr); 
        //And its stats
	stats = link_stats_from_lladdr(lladdr);
        rssi = (int16_t)-1 - stats->rssi;                   //This is needed because of the Cbor negative integer representation
        etx = stats->etx;
        //Set the neighbour's MAC address into the Cbor structure
        memcpy(neighbour+1, lladdr, 8);
        //Set RSSI and ETX relative to the neighbour node
        neighbour[11] = rssi << 8;
        neighbour[12] = rssi & ((1 << 8) - 1);
        neighbour[14] = etx << 8;
        neighbour[15] = etx & ((1 << 8) - 1);
        //Copy the Cbor structure into the POST payload
        memcpy(payload+payload_dim, neighbour, sizeof(neighbour));
        payload_dim += sizeof(neighbour);
    }
    
    
    return payload_dim;
    
}

static void topology_update(void *ptr){
    request_t* req = NULL;
    linkaddr_t* nodeAddr = &linkaddr_node_addr;
    uint8_t payload_dim = 0;
    
    ctimer_reset(&timer_topology);
    req = add_request();
    if(req != NULL){
        //Set type of message: CON and POST
        coap_init_message(&req->req_packet, COAP_TYPE_CON, COAP_POST, 0);
        //Set the target resource 
        coap_set_header_uri_path(&req->req_packet, "Network");
        //Set the query parameter: "?mac=<node's mac address>"
        URI_QUERY_MAC_ADDR(uri_query, nodeAddr);
        coap_set_header_uri_query(&req->req_packet, uri_query);
        //Set the type of request needed to the working process in order to select the right callback function
        req->type = TOPOLOGY_UPDATE;
        //Set payload type and the actual content
        coap_set_header_content_format(&req->req_packet, APPLICATION_CBOR);
        payload_dim = prepare_payload_top_update();   
        if(payload_dim == 0)
            return;
        coap_set_payload(&req->req_packet, payload, payload_dim);

    }
    start_request();
}

 
void control_agent_init(){
    ctimer_set(&timer_ttl, TTL_INTERVAL, callback_decrement_ttl, NULL);
    ctimer_set(&timer_topology, TOP_UPDATE_PERIOD, topology_update, NULL);
    flowtable_init();
    flowtable_test();
    
    SERVER_NODE(&server_ipaddr);
    /* receives all CoAP messages */
    coap_init_engine();
    ringbufindex_init(&requests_ringbuf, MAX_REQUEST);
    process_start(&coap_client_process, NULL);
}



void client_table_miss_handler(void *response){
  const uint8_t *chunk;

  int len = coap_get_payload(response, &chunk);

  printf("Table miss: %.*s", len, (char *)chunk);
}

void client_topology_update_handler(void *response){
  const uint8_t *chunk;

  int len = coap_get_payload(response, &chunk);

  printf("Topology update: %.*s", len, (char *)chunk);
}

PROCESS_THREAD(coap_client_process, ev, data)
{
  PROCESS_BEGIN();
  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    request_t* req = get_next_request();
    if(req != NULL){
        if(req->type == TABLE_MISS){
            printf("TABLE MISS\n");
            COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, &req->req_packet, client_table_miss_handler);
        }
        else if(req->type == TOPOLOGY_UPDATE){
            printf("TOPOLOGY UPDATE\n");
            COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, &req->req_packet, client_topology_update_handler);
        }
        
    }

  }
  PROCESS_END();
}