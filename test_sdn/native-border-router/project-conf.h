//SDN Sink conf (NATIVE BR)

#ifndef PROJECT_ROUTER_CONF_H_
#define PROJECT_ROUTER_CONF_H_

#include "parametri.h"

#ifndef UIP_FALLBACK_INTERFACE
#define UIP_FALLBACK_INTERFACE rpl_interface
#endif

#ifndef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM          40
#endif

#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE    400 // <---- THIS is the actual problem on the testbed, do not go under 200, otherwise we LOSE the CoAP requests
#endif

#ifndef UIP_CONF_RECEIVE_WINDOW
#define UIP_CONF_RECEIVE_WINDOW  60
#endif

#define SLIP_DEV_CONF_SEND_DELAY (CLOCK_SECOND / 32)

//ADDED

//Set max rest size to 256 to avoid bad CBOR packets format
#undef REST_MAX_CHUNK_SIZE
#define REST_MAX_CHUNK_SIZE 256

//6lowpan fragmentation
#undef SICSLOWPAN_CONF_FRAG
#define SICSLOWPAN_CONF_FRAG 1
#define TESTBED 1

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC border_router_rdc_driver
#undef NULLRDC_CONF_802154_AUTOACK
#define NULLRDC_CONF_802154_AUTOACK       1

//----------------------------- IF Using SDN
#if NETSTACK_CONF_SDN == 1
#include "net/ipv6/multicast/uip-mcast6-engines.h"
#define UIP_MCAST6_CONF_ENGINE UIP_MCAST6_ENGINE_SDN
#define SINK 1 //I am the Sink SDN Node

#undef COAP_MAX_OBSERVERS
#define COAP_MAX_OBSERVERS             1
// Filtering .well-known/core per query can be disabled to save space. 
#undef COAP_LINK_FORMAT_FILTERING
#define COAP_LINK_FORMAT_FILTERING     1
#undef COAP_PROXY_OPTION_PROCESSING
#define COAP_PROXY_OPTION_PROCESSING   0

// Turn off DAO ACK to make code smaller 
#undef RPL_CONF_WITH_DAO_ACK
#define RPL_CONF_WITH_DAO_ACK          0
#undef WITH_WEBSERVER
#define WITH_WEBSERVER 0
#undef COAP_OBSERVING
#define COAP_OBSERVING 0
#undef COAP_BLOCK
#define COAP_BLOCK 0
#undef COAP_SEPARATE
#define COAP_SEPARATE 0

#undef UIP_CONF_ND6_SEND_NA
#define UIP_CONF_ND6_SEND_NA 1 //Send NA
//Timeout on neighbours
//#undef UIP_CONF_ND6_REACHABLE_TIME
//#define UIP_CONF_ND6_REACHABLE_TIME 20000

#undef RPL_CONF_STATS
#define RPL_CONF_STATS 0
#undef RPL_CONF_MAX_DAG_PER_INSTANCE
#define RPL_CONF_MAX_DAG_PER_INSTANCE 1
#undef PROCESS_CONF_STATS
#define PROCESS_CONF_STATS 0

//Disable DAOs
#define RPL_DAO_ENABLE 0 

//Callback functions
#define RPL_CALLBACK_PARENT_SWITCH sdn_rpl_callback_parent_switch
#define SDN_CALLBACK_ADD_NEIGHBOR sdn_callback_neighbor

//Configure max number of neighbors
#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS 10 //Default for zoul is 16, for contiki 8, do not go over 7 (otherwise set REST_MAX_CHUNK_SIZE to 256

//Runs in storing mode
#undef RPL_NS_CONF_LINK_NUM
#define RPL_NS_CONF_LINK_NUM 10 //10 link maintained
#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES 10 //10 routes
//#undef RPL_CONF_MOP
//#define RPL_CONF_MOP RPL_MOP_NON_STORING // Mode of operation, NON storing

//-------------------------- ELSE, not using SDN
#else       

#undef RPL_NS_CONF_LINK_NUM
#define RPL_NS_CONF_LINK_NUM 40 //40 links maintained
#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES 40 
#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS 10

#endif      //ENDIF SDN
//ADDED

#ifndef WEBSERVER_CONF_CFS_CONNS
#define WEBSERVER_CONF_CFS_CONNS 2
#endif

#define SERIALIZE_ATTRIBUTES 1
#define CMD_CONF_OUTPUT border_router_cmd_output

/* used by wpcap (see /cpu/native/net/wpcap-drv.c) */
#define SELECT_CALLBACK 1

#endif /* PROJECT_ROUTER_CONF_H_ */
