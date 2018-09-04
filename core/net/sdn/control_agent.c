/**
 * \file
 *         control_agent.c
 * \author
 *         Giulio Micheloni <giulio.micheloni@gmail.com>
 *
 * \updated by
 *         Simone Tavoletta <s.tavoletta@filmsimili.it>
 */

#include "net/sdn/control_agent.h"

#define SDN_STATS 1
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
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(addr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

PROCESS(coap_client_process, "Coap Client");
static struct ctimer timer_ttl;
static struct ctimer timer_topology;
static struct ringbufindex requests_ringbuf;
static request_t requests_array[MAX_REQUEST];
static pending_request_t pending_requests_array[MAX_REQUEST];
uip_ipaddr_t controller_ipaddr;

#define LOCAL_PORT      UIP_HTONS(COAP_DEFAULT_PORT + 1)
#define REMOTE_PORT     UIP_HTONS(COAP_DEFAULT_PORT) //COAP_DEFAULT_PORT = 5683

#define URI_QUERY_MAC_ADDR(buffer, addr) sprintf(buffer,"?mac=%02x%02x%02x%02x%02x%02x%02x%02x", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7])



/*#define URI_QUERY_ALL_PACKET(buffer, sdr, rcv) sprintf(buffer,"?type=all&tx=%02x%02x%02x%02x%02x%02x%02x%02x&rx=%02x%02x%02x%02x%02x%02x%02x%02x", \
                                                        ((uint8_t *)sdr)[0], ((uint8_t *)sdr)[1], ((uint8_t *)sdr)[2], ((uint8_t *)sdr)[3], ((uint8_t *)sdr)[4], ((uint8_t *)sdr)[5], ((uint8_t *)sdr)[6], ((uint8_t *)sdr)[7], \
                                                        ((uint8_t *)rcv)[0], ((uint8_t *)rcv)[1], ((uint8_t *)rcv)[2], ((uint8_t *)rcv)[3], ((uint8_t *)rcv)[4], ((uint8_t *)rcv)[5], ((uint8_t *)rcv)[6], ((uint8_t *)rcv)[7])

*/
#define URI_QUERY_ALL_PACKET(buffer, addr) sprintf(buffer,"?type=all&mac=%02x%02x%02x%02x%02x%02x%02x%02x", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7])

static uint8_t payload[MAX_DIM_PAYLOAD];
static char uri_query[50];
static clock_time_t topology_update_intervals[3];
static int topology_update_index;
//static int topology_up_counter; //Contatore per la topology update


void start_request(void){
    process_poll(&coap_client_process);
}

request_t* add_request(void){
  int index = ringbufindex_peek_put(&requests_ringbuf);
  //printf("RINGBUF_ADD = %i\n", index);
  if(index != -1){
      ringbufindex_put(&requests_ringbuf);
      return &requests_array[index];
  }
  return NULL;
}

request_t* get_ref_to_next_request(void){
  int index = ringbufindex_peek_get(&requests_ringbuf);
  //printf("RINGBUF_GET = %i\n", index);
  if(index != -1){
      //ringbufindex_get(&requests_ringbuf);
      return &requests_array[index];
  }
  return NULL;
}

request_t* advance_ring_buf(){
    int index = ringbufindex_get(&requests_ringbuf);
    //printf("RINGBUF_ADV = %i\n", index);
    if(index != -1){
      //ringbufindex_get(&requests_ringbuf);
      return &requests_array[index];
    }
    return NULL;
}


//Called from uip-ds6-nbr.c when removing a neighbor from the table (by Simone)
void remove_sdn_rule (const linkaddr_t *addr){
    rule_t* rule;
    action_t* action;
    entry_t* entry;
    if(addr == NULL)
        return;
    
    //Create the flow entry to remove
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
 	remove_entry(entry);  
    deallocate_entry(entry);   
	printf("DS6_CALLBACK_SDN: Removed neighbor entry from flow table as it is now unreachable\n");
}

//Called from the RPL module when it chooses a (new) parent
void sdn_rpl_callback_parent_switch(rpl_parent_t *old, rpl_parent_t *new){
    uint8_t addrDim;
    linkaddr_t llAddrParent;
    linkaddr_t llAddrRoot;
    rule_t* rule;
    action_t* action;
    entry_t* entry;
	uint8_t addr_tunslip[8]  = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    
    
    //Check if there was an old parent
    if(old != NULL){
		printf(">> Removing old parent\n");
        //If so, we must remove its entry from the flow table
        extractIidFromIpAddr(&llAddrParent, rpl_get_parent_ipaddr(old), &addrDim);
        extractIidFromIpAddr(&llAddrRoot, &old->dag->dag_id, &addrDim);
        
        entry = create_entry(2);
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
        deallocate_entry(entry);
        PRINTF("Control Agent: removed parent: ");
        PRINTLLADDR(&llAddrParent);
        PRINTF(" to root: ");
        PRINTLLADDR(&llAddrRoot);    
        PRINTF("\n");
	
		//Remove also tunslip rule (if any)
		rule_t* tunslip_rule1;
        action_t* tunslip_action1;
        entry_t* tunslip_entry1;
		
		tunslip_entry1 = create_entry(1);
        tunslip_rule1 = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_tunslip); //If tunslip mac
        add_rule_to_entry(tunslip_entry1, tunslip_rule1);
        tunslip_action1 = create_action(FORWARD, NO_FIELD, 0, 64, &llAddrParent); //Forward to parent
        add_action_to_entry(tunslip_entry1, tunslip_action1);
		//Remove entry
		remove_entry(tunslip_entry1);  
		deallocate_entry(tunslip_entry1);
    }
    
    
    //If the node has selected a new parent, add the rule how to reach 
    //the root of the DODAG into the flow table
    if(new != NULL){
		printf("Node has selected a new parent, adding rule to reach the root of DODAG\n");
        extractIidFromIpAddr(&llAddrParent, rpl_get_parent_ipaddr(new), &addrDim);
        extractIidFromIpAddr(&llAddrRoot, &new->dag->dag_id, &addrDim);
        
        entry = create_entry(2);
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
        printf(">> Parent rule allocated correctly {If dest is ");
		PRINTLLADDR(&llAddrRoot);
		printf(" => send to ");
		PRINTLLADDR(&llAddrParent); 
	    printf("}\n");
		
		//Now we need to set the tunslip rule: if the address MAC is tunslip, forward to parent => we get to the root!
		//This is needed because of the fact that for example a ping6 is executed through tunslip, so the destination the node will respond to
		//will be onlink and will have the tunslip mac address associated.
		rule_t* tunslip_rule;
        action_t* tunslip_action;
        entry_t* tunslip_entry;
		
		tunslip_entry = create_entry(1);
		tunslip_rule = create_rule(MH_DST_ADDR, 0, 64, EQUAL, addr_tunslip); //If tunslip mac
		add_rule_to_entry(tunslip_entry, tunslip_rule);
		tunslip_action= create_action(FORWARD, NO_FIELD, 0, 64, &llAddrParent); //Forward to parent
		add_action_to_entry(tunslip_entry, tunslip_action);
		add_entry_to_ft(tunslip_entry);
    }
	
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
    entry->stats.ttl = 60 * 20;       // 20 Minutes
    //Check if this neighbor already exits into the flow table
    app = find_entry(entry);
    if(app != NULL){
        //We can't confuse with the rule installed by RPL, and it may happen if the root is our neighbour
        if(app->stats.ttl != 0){
            //It exists, so just update the ttl field
            app->stats.ttl = 60 * 20;      // Refresh to 20 minutes
        }
        deallocate_entry(entry);
    }
    else{
        //It doesn't exist, so add it in to the flow table
        add_entry_to_ft(entry);
        PRINTF("Neighbour found: installed in the flow table");
        print_entry(entry);
        PRINTF("\n");
    }    
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
            PRINTF("EXPIRED Flow Table entry: ");
#if DEBUG == 1
            print_entry(app);
#endif
            PRINTF("\n");
            remove_entry(app);
        }
        else
            entry = entry->next;
    }
}

//It returns -1 if there's no free place, or if the mesh address is already contained
//Otherwise, when the request was successfully added, it returns the array index
int add_pending_request(linkaddr_t* mesh_destination){
	printf("Trying to add coap request\n");
    int i, free_slot = -1; 
    //Clear the exipred request
    for(i = 0; i < MAX_REQUEST; i++){
        if(pending_requests_array[i].valid && timer_expired(&pending_requests_array[i].lifetime_timer)){   //Valid and expired -> remove it
            printf("Request for ");
            PRINTLLADDR(&pending_requests_array[i].mesh_address);
            printf(" has expired\n");
            pending_requests_array[i].valid = 0;
            if(pending_requests_array[i].payload != NULL)
                queuebuf_free(pending_requests_array[i].payload);
        }
    }
    //Search for the mesh_destination in valid request slot
    for(i = 0; i < MAX_REQUEST; i++){
        if(pending_requests_array[i].valid && linkaddr_cmp(mesh_destination, &pending_requests_array[i].mesh_address) != 0){   
            //A request for mesh_destination address has already been sent
            printf(">>> There is already a request for ");
            PRINTLLADDR(&pending_requests_array[i].mesh_address);
            printf("\n");
            return -1;
        }
        if(!pending_requests_array[i].valid)
            free_slot = i;
    }
    if(free_slot == -1){ //No free slot for this request
		printf(">> There is no free slot to send a request, aborting coap message\n");
        return -1;
	}
    //At this point it means that there's no a request with mesh_destination address
    //Thus, add it at the first free slot, set the timer and return the index
    linkaddr_copy(&pending_requests_array[free_slot].mesh_address, mesh_destination);
    pending_requests_array[free_slot].valid = 1;
    timer_set(&pending_requests_array[free_slot].lifetime_timer, CLOCK_SECOND * (COAP_MAX_RETRANSMIT * COAP_RESPONSE_TIMEOUT / 10)); //4 seconds
    pending_requests_array[free_slot].payload = NULL;
    printf("Set pending req for %u seconds with: ", COAP_MAX_RETRANSMIT * COAP_RESPONSE_TIMEOUT / 10); 
    PRINTLLADDR(&pending_requests_array[free_slot].mesh_address);
    printf("\n");
    return free_slot;
}

void remove_pending_req(linkaddr_t* mesh_destination){
    int i;
    for(i = 0; i < MAX_REQUEST; i++){
        if(pending_requests_array[i].valid && linkaddr_cmp(mesh_destination, &pending_requests_array[i].mesh_address) != 0){   
            //A response for mesh_destination address has arrived
            printf("Entry for ");
            PRINTLLADDR(&pending_requests_array[i].mesh_address);
            printf(" has arrived, remove it\n");
            pending_requests_array[i].valid = 0;
            if(pending_requests_array[i].payload != NULL)
                queuebuf_free(pending_requests_array[i].payload);
            break;
        }        
    }
}

void handleTableMiss(linkaddr_t* L2_receiver, linkaddr_t* L2_sender, uint8_t* ptr_to_pkt, uint16_t pkt_dim){
	printf("Handling a Table Miss, requesting information to SDN-Controller\n");
    request_t* req = NULL;
    int pending_index = 0;
    linkaddr_t* nodeAddr = &linkaddr_node_addr;
    linkaddr_t mesh_destination;
    struct queuebuf* q;
    if(queuebuf_numfree() == 0)
        return;
    q = queuebuf_new_from_packetbuf();
    if(q == NULL)
        return;
    if(parseMeshHeader(ptr_to_pkt, NULL, &(mesh_destination.u8), NULL, NULL, NULL) > 0){
        pending_index = add_pending_request(&mesh_destination);
        if(pending_index == -1){
            queuebuf_free(q);
            return;
        }
        pending_requests_array[pending_index].payload = q;
    }
    req = add_request();
    if(req == NULL){
        if(pending_index != -1)
            pending_requests_array[pending_index].valid = 0;
        queuebuf_free(q);
        return;
    }
	    
    //Set type of message: CON and POST
    coap_init_message(&req->req_packet, COAP_TYPE_CON, COAP_POST, 0);
    //Set the target resource 
    coap_set_header_uri_path(&req->req_packet, "fe");
	
    //Set the query parameter: 
    //URI_QUERY_ALL_PACKET (uri_query, L2_sender, L2_receiver); //"?type=all&tx=transmitter&rx=receiver"
	URI_QUERY_ALL_PACKET (uri_query, nodeAddr); //"?type=all&mac=<sender's mac address>"
	
    coap_set_header_uri_query(&req->req_packet, uri_query);        
    //Set the type of request needed to the working process in order to select the right callback function
    req->type = TABLE_MISS;    
    //Set payload type and the actual content
    //memcpy(payload, ptr_to_pkt, pkt_dim);
    coap_set_payload(&req->req_packet, queuebuf_dataptr(q), queuebuf_datalen(q));
    //queuebuf_free(q);                     //TODO: this function is supposed to be called after the second process sends the request
    start_request();                        //But it seems to cause a segmentation fault
	printf("--> actually generating table miss request [packet size: %u, max_coap: %d]\n", queuebuf_datalen(q), COAP_MAX_BLOCK_SIZE);
}

uint8_t computeNumOfNeighbours(){
    uint8_t num_of_neigh = 0;
    uip_ds6_nbr_t *nbr;
    if(ds6_neighbors == NULL)
        return 0;
    for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL; nbr = nbr_table_next(ds6_neighbors, nbr)) {
        num_of_neigh++;
		if(num_of_neigh == NBR_TABLE_CONF_MAX_NEIGHBORS)
			break;
    }
    return num_of_neigh;
}

//Build the topology update payload
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
    uint8_t header[] = {0x84, 0x00, 0x19, 0x00, 0x00, 0x18, 0x00, 0x00};
    uint8_t neighbour[] = {0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x82, 0x39, 0x00, 0x00, 0x19, 0x00, 0x00};
    //Get the number of neighbours, if it is 0 don't send any update to the Controller then
    num_of_neigh = computeNumOfNeighbours();
    if(num_of_neigh == 0){
        printf("Control agent: 0 NEIGHBOURS --> Not sending topology update\n");
        return 0;
    }
    //Set the state information regarding the sending node  
    header[1] = 1;                  							//Version of the update structure
    header[3] = 1 << 8;                 						//Battery level (on testbed nodes have no battery constraints)
    header[4] = 1 & ((1 << 8) - 1);	
    /*SENSORS_ACTIVATE(battery_sensor);
    header[3] = battery_sensor.value(0) << 8;                 	//Battery level (not for cooja)
    header[4] = battery_sensor.value(0) & ((1 << 8) - 1);
    SENSORS_DEACTIVATE(battery_sensor);*/	
    header[6] = 30;                 							//Queue utilization
    header[7] = 0xa0 | num_of_neigh;							//Set the Cbor major type "Array" with the number of items
    
	//Copy the first part of the POST payload
    memcpy(payload, header, sizeof(header));
    payload_dim = sizeof(header);
    
	//printf("Lista vicini (%u): ", num_of_neigh);
    for(nbr = nbr_table_head(ds6_neighbors); nbr != NULL; nbr = nbr_table_next(ds6_neighbors, nbr)) {
        //Get the neighbour's MAC address
        lladdr = uip_ds6_nbr_get_ll(nbr); 
		
		//PRINTLLADDR(lladdr);
		//printf(", ");
		
        //And its stats
	    stats = link_stats_from_lladdr(lladdr);
		if(stats == NULL){
			etx = 2000;
			rssi = (int16_t)-1 - (-2000);
		}
		else{
			rssi = (int16_t)-1 - stats->rssi;                   //This is needed because of the Cbor negative integer representation
			etx = stats->etx;
		}
		//Set the neighbour's MAC address into the Cbor structure
        memcpy(neighbour+1, lladdr, 8);
        //Set RSSI and ETX relative to the neighbour node
        neighbour[11] = (uint8_t)(rssi << 8);
        neighbour[12] = (uint8_t)(rssi & ((1 << 8) - 1));
        neighbour[14] = (uint8_t)(etx << 8);
        neighbour[15] = (uint8_t)(etx & ((1 << 8) - 1));
		
		
        //Copy the Cbor structure into the POST payload
        memcpy(payload+payload_dim, neighbour, sizeof(neighbour));
        payload_dim += sizeof(neighbour);
    }
	
	printf("\n");
    printf("Topology update CBOR [%u neighbors], size: %u\n", num_of_neigh, payload_dim, COAP_MAX_BLOCK_SIZE);
    return payload_dim;
}

static void topology_update(void *ptr){
	
	//Only 3 topology updates (warmup, 12 minutes)
	/*if(topology_up_counter >= 3)
	//	return;	
	topology_up_counter = topology_up_counter + 1;
	*/

	printf("Sending topology update\n");
	
    unsigned short delay = CLOCK_SECOND *(random_rand() % 30);
    request_t* req = NULL;
    linkaddr_t* nodeAddr = &linkaddr_node_addr;
    uint8_t payload_dim = 0;
    req = add_request();
	
    if(req != NULL){
		printf("--> actually generating the topology update COAP\n");
        //Set type of message: CON and POST
        coap_init_message(&req->req_packet, COAP_TYPE_CON, COAP_POST, 0);
        //Set the target resource 
        coap_set_header_uri_path(&req->req_packet, "6lo");
        //Set the query parameter: "?mac=<node's mac address>"
        URI_QUERY_MAC_ADDR(uri_query, nodeAddr);
        coap_set_header_uri_query(&req->req_packet, uri_query);
        //Set the type of request needed to the working process in order to select the right callback function
        req->type = TOPOLOGY_UPDATE;
        //Set payload type and the actual content
        coap_set_header_content_format(&req->req_packet, APPLICATION_CBOR);
        payload_dim = prepare_payload_top_update();   
        coap_set_payload(&req->req_packet, payload, payload_dim);   
    }    
    start_request();
	printf("--> Starting topology update request\n");
    //ctimer_reset(&timer_topology);
    if(topology_update_index == 0){
        topology_update_index = 1;
    }
    else if(topology_update_index == 1){
        topology_update_index = 2;
    }
    ctimer_set(&timer_topology, topology_update_intervals[topology_update_index] + delay, topology_update, NULL);
}


void control_agent_init(){
	
	//topology_up_counter = 0;
	
    int i; 
    unsigned short delay = CLOCK_SECOND *(random_rand() % 30);
    topology_update_index = 0;
    topology_update_intervals[0] = TOP_UPDATE_FIRST;
    topology_update_intervals[1] = TOP_UPDATE_SECOND;
    topology_update_intervals[2] = TOP_UPDATE_PERIOD;
    
    ctimer_set(&timer_ttl, TTL_INTERVAL, callback_decrement_ttl, NULL);
    ctimer_set(&timer_topology, topology_update_intervals[topology_update_index] + delay, topology_update, NULL);
    flowtable_init();
	
	#if SINK == 1 //If this is the BR/Sink SDN Node
	  set_up_br_rule();
	#endif
	
    //init pending request    
    for(i = 0; i < MAX_REQUEST; i++){
        pending_requests_array[i].valid = 0;
    }
	
    ringbufindex_init(&requests_ringbuf, MAX_REQUEST);
    process_start(&coap_client_process, NULL);
	
	printf("Control agent init ok\n");
	printf("My MAC address: ");
    PRINTLLADDR(&linkaddr_node_addr);
    printf("\n");
}


void parse_table_miss_response(const uint8_t* chunk, int len){
    const cn_cbor *cb;
    cn_cbor_errback err;
    cn_cbor* cp;
    linkaddr_t* mesh_destination;
    if(chunk == NULL || len == 0)
        return;
    cb = cn_cbor_decode((const char*)chunk, len, &err);
    if(cb == NULL){
        printf("Control Agent: parsing Cbor payload has failed:");
        printf(" Err %u pos %i\n", err.err, err.pos);
        return;
    }
    if(cb->type == CN_CBOR_ARRAY){
        for (cp = cb->first_child; cp != NULL; cp = cp->next) {
            mesh_destination = (linkaddr_t*)install_flow_entry_from_cbor(cp);
            if(mesh_destination == NULL){
                PRINTF("Control Agent: inserting new flow entries has failed\n");
                break;
            }
            else{
                remove_pending_req(mesh_destination);
            }
        }          
    }
    else{
        PRINTF("Control Agent: wrong structure of Cbor response\n");
    }
    cn_cbor_free(cb);
}


void client_table_miss_handler(void *response){
	printf(">> Received table miss response from controller\n");
    const uint8_t *chunk;
    int len;
    
    if(response == NULL)
        return;
    len = coap_get_payload(response, &chunk);
    if(len == 0)
        return;
	printf(">>> Parsing table miss response\n");
    parse_table_miss_response(chunk, len);
}

void client_topology_update_handler(void *response){
  printf(">> Received topology update response from controller\n");
  const uint8_t *chunk;
  if(response == NULL)
      return;
  int len = coap_get_payload(response, &chunk);
  if(len == 0)
      return;
}

void put_handler_flowtable(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset){    
  uint8_t *incoming = NULL;
  size_t len = 0;  
  const cn_cbor *cb;
  cn_cbor_errback err;
  cn_cbor* cp;
  linkaddr_t* mesh_destination;
  if((len = REST.get_request_payload(request, (const uint8_t **)&incoming))){
      
    /*
#if DEBUG == 1
    int i;
    printf("Received payload len %d: ", len);
    for (i = 0; i < len; i++)
    {
      printf("%02x", incoming[i]);
    }
    printf("\n");
#endif
    */
      
    cb = cn_cbor_decode((const char*)incoming, len, &err);
    if(cb == NULL){
        printf("Control Agent: parsing Cbor payload has failed:");
        printf(" Err %u pos %i\n", err.err, err.pos);
        REST.set_response_status(response, REST.status.BAD_REQUEST);
        return;
    }
    if(cb->type == CN_CBOR_ARRAY){
        for (cp = cb->first_child; cp; cp = cp->next) {
            mesh_destination = (linkaddr_t*)install_flow_entry_from_cbor(cp);
            if(mesh_destination != NULL){
                 remove_pending_req(mesh_destination);
            }
        }          
    }
    else{
        PRINTF("Control Agent: wrong structure of Cbor response\n");
        REST.set_response_status(response, REST.status.BAD_REQUEST);
    }
    cn_cbor_free(cb);
    REST.set_response_status(response, REST.status.CREATED);
  }
  else
      REST.set_response_status(response, REST.status.BAD_REQUEST);  
}

void post_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset){
	uint8_t *incoming = NULL;
    uip_ipaddr_t multicast_addr;
    linkaddr_t mesh_multicast_addr;
    uint8_t addrDim;
    int len;
    memset(&mesh_multicast_addr, 0, sizeof(mesh_multicast_addr));
    if((len = REST.get_request_payload(request, (const uint8_t **)&incoming)) > 0){
        memcpy(&multicast_addr, incoming, sizeof(uip_ipaddr_t));
        if (uip_ds6_maddr_add(&multicast_addr) != NULL){       
            extractIidFromIpAddr(&mesh_multicast_addr, &multicast_addr, &addrDim);
            if(addMeshMulticastAddress(&mesh_multicast_addr) == 1)               
                REST.set_response_status(response, REST.status.CHANGED);
            else
                REST.set_response_status(response, REST.status.INTERNAL_SERVER_ERROR);
#if DEBUG == 1
            PRINTF("Received multicast addr: ");
            PRINT6ADDR(&multicast_addr);
            PRINTF(" -> ");
            printf("%02x%02x\n", mesh_multicast_addr.u8[0], mesh_multicast_addr.u8[1]);
#endif
            
        }
        REST.set_response_status(response, REST.status.INTERNAL_SERVER_ERROR);
    }
    REST.set_response_status(response, REST.status.BAD_REQUEST);
}

/*EXPERIMENTAL PURPOSE*/
void get_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset){
#if DEBUG == 1
    PRINTF("Received get on coap group resource\n");            
#endif
    #if PrintStatistics == 1
		printf("COAP_GET_RECEIVED\n");
	#endif
    uint8_t length = 40;
    char message[length];

    sprintf(message, "Ciao a tutti, questa e una risorsa\n");
    length = strlen(message);
    memcpy(buffer, message, length);

    REST.set_header_content_type(response, REST.type.TEXT_PLAIN); 
    REST.set_header_etag(response, (uint8_t *) &length, 1);
    REST.set_response_payload(response, buffer, length);
}


RESOURCE(resource_flow_table, "title=\"Resource\";rt=\"Text\"", NULL, NULL, put_handler_flowtable, NULL);
RESOURCE(resource_coap_group, "title=\"Resource\";rt=\"Text\"", get_handler, post_handler, NULL, NULL);

PROCESS_THREAD(coap_client_process, ev, data){
    PROCESS_BEGIN();
    request_t* req;
    /* receives all CoAP messages */
    //coap_init_engine();
    rest_init_engine();
    rest_activate_resource(&resource_flow_table, "lca/ft");
    rest_activate_resource(&resource_coap_group, "cg");
	
	uip_ip6addr(&controller_ipaddr, 0x2001, 0x0db8, 0, 0xf101, 0, 0, 0, 0x0001);  //IP address of the controller (2001:0db8:0:f101::1), offlink
	//uip_ip6addr(&controller_ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 0x0001);
	
	printf("Controller IPv6 address: ");
	PRINT6ADDR(&controller_ipaddr);
	printf("\n");
	
    while(1) {
        PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
        while((req = get_ref_to_next_request()) != NULL){
            
            if(req != NULL){
                if(req->type == TABLE_MISS){
					printf("Table miss request sent correctly!\n");
                    COAP_BLOCKING_REQUEST(&controller_ipaddr, REMOTE_PORT, &req->req_packet, client_table_miss_handler);
                }
                else if(req->type == TOPOLOGY_UPDATE){
                    printf("Topology update sent correctly\n");
                    COAP_BLOCKING_REQUEST(&controller_ipaddr, REMOTE_PORT, &req->req_packet, client_topology_update_handler);
                }
                advance_ring_buf();
            }
        }
    }
    PROCESS_END();
}
