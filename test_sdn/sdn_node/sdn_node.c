#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-udp-packet.h"
#include <stdio.h>
#include <string.h>

#if TESTING == 1
	#include "contiki-net.h"
	#include "rest-engine.h"
	#include "er-coap-engine.h"
	#include "er-coap-conf.h"
    #include "dev/button-sensor.h"
    #include "sys/etimer.h"
    #include <stdlib.h>
    #define REMOTE_PORT     UIP_HTONS(COAP_DEFAULT_PORT)
#endif

#if NETSTACK_CONF_SDN == 1
	#include "net/sdn/control_agent.h"
#endif

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"


PROCESS(sdn_node, "SDN Node Process");
AUTOSTART_PROCESSES(&sdn_node);
/*---------------------------------------------------------------------------*/

static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("SDN Node IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
      /* hack to make address "final" */
      if (state == ADDR_TENTATIVE) {
	uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
}

/*---------------------------------------------------------------------------*/
static void
set_global_address(void)
{
  uip_ipaddr_t ipaddr;

  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
  /*
  uip_ip6addr(&ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_prefix_add(&ipaddr, UIP_DEFAULT_PREFIX_LEN, 0, 0, 0, 0);
  */
}

#if TESTING == 1
void client_NON_handler(void* response){
	printf("Received NON response\n");
	return;
}

void client_CON_handler(void* response){
	printf("Received CON response\n");
	const uint8_t *chunk;
	uint16_t len = coap_get_payload(response, &chunk);
	printf("UDP_UPDOWN_ACK_%s\n", (char *)chunk);
	return;
}

void testing_get_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    uint8_t length = 3;
    char message[length];
	printf("UDP_POLL_GET_RECEIVED\n");
    sprintf(message, "Ok");
    length = strlen(message);
    memcpy(buffer, message, length);

    REST.set_header_content_type(response, REST.type.TEXT_PLAIN); 
    REST.set_header_etag(response, (uint8_t *) &length, 1);
    REST.set_response_payload(response, buffer, length);
}
RESOURCE(hello_resource, "title=\"Resource\";rt=\"Text\"", testing_get_handler, NULL, NULL, NULL);
#endif


/*---------------------------------- Main Process Thread ----------------------------------------------*/
PROCESS_THREAD(sdn_node, ev, data) {
	
	#if TESTING == 1
		uip_ipaddr_t controller;
		uip_ip6addr(&controller, 0x2001, 0x0db8, 0, 0xf101, 0, 0, 0, 0x0001);
	    static struct etimer timer;	
		static int onlyUp_id = 1;
	    static int upDown_id = 1;
	    static int msg_type = 0;
  	#endif
	
	PROCESS_BEGIN(); //Process BEGIN
	
	//For experiments
	#if TESTING == 1
		rest_init_engine();
	    rest_activate_resource(&hello_resource, "hello");
	#endif
	//Check if we are using SDN
  	#if NETSTACK_CONF_SDN == 1
   		control_agent_init();         
  	#endif
	
	set_global_address();
	PRINTF("SDN Node process started, max neighbors:%d | max routes:%d\n", NBR_TABLE_CONF_MAX_NEIGHBORS, UIP_CONF_MAX_ROUTES);
	print_local_addresses();

	

	while(1) {
		#if TESTING == 0
		    PROCESS_YIELD();
		#endif
		
		#if TESTING == 1
		    int delay = random_rand() % 30 + 5; //a delay between 5 and 30 seconds
		    etimer_set(&timer, (CLOCK_SECOND * 60 * 1) + (delay * CLOCK_SECOND)); //Every 1 minute + delay
		    printf("Scheduling request in 1 minutes and %d seconds\n", delay); 
		    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
			//PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
		    if(msg_type == 0){ //UDP Up only
				msg_type = 1;
				#if UDP_ONLYUP == 1
					coap_packet_t request[1];
					coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
					coap_set_header_uri_path(request, "ts");
					char uri_query[20];
					sprintf(uri_query, "?onlyup=1&id=%d", onlyUp_id);
					coap_set_header_uri_query(request, uri_query);      
					printf("UDP_ONLYUP_SEND_%d\n", onlyUp_id);
					onlyUp_id = onlyUp_id + 1;
					COAP_BLOCKING_REQUEST(&controller, REMOTE_PORT, request, client_NON_handler);
				#endif
			}else{ //UDP Up & Down
				msg_type = 0;
				#if UDP_UPDOWN == 1
					coap_packet_t request[1];
					coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
					coap_set_header_uri_path(request, "ts");
					char uri_query[20];
					sprintf(uri_query, "?updown=1&id=%d", upDown_id);
					coap_set_header_uri_query(request, uri_query);      
					printf("UDP_UPDOWN_SEND_%d\n", upDown_id);
					upDown_id = upDown_id + 1;
					COAP_BLOCKING_REQUEST(&controller, REMOTE_PORT, request, client_CON_handler);
				#endif
			}
		#endif
  	} 

	PROCESS_END();
}