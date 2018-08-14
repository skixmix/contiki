//SDN Node conf

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#include "parametri.h"

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC     nullrdc_driver
#undef NULLRDC_CONF_802154_AUTOACK
#define NULLRDC_CONF_802154_AUTOACK       1

#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE    400 //Do NOT go under 200
#endif

/* Define as minutes */
#define RPL_CONF_DEFAULT_LIFETIME_UNIT   60

/* 10 minutes lifetime of routes */
#define RPL_CONF_DEFAULT_LIFETIME        10

#define RPL_CONF_DEFAULT_ROUTE_INFINITE_LIFETIME 1

#define SERVER_REPLY 1
//Only one DODAG per instance
#undef RPL_CONF_MAX_DAG_PER_INSTANCE
#define RPL_CONF_MAX_DAG_PER_INSTANCE 1
#undef RPL_CONF_STATS
#define RPL_CONF_STATS 0
#undef PROCESS_CONF_STATS
#define PROCESS_CONF_STATS 0

#ifndef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM          8 //Buffer
#endif

#define TESTBED 1
//Radio communication driver
#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC     nullrdc_driver
#undef NULLRDC_CONF_802154_AUTOACK
#define NULLRDC_CONF_802154_AUTOACK       1

//ADDED

//For experiment purposes
#define PrintStatistics 1 
#define TESTING 1

//Rpl ZERO OF
#undef RPL_CONF_OF_OCP
#define RPL_CONF_OF_OCP RPL_OCP_OF0
//End For experiment purposes

//Set to 1 in order to hardcode the MAC address
#ifndef IEEE_ADDR_CONF_HARDCODED
#define IEEE_ADDR_CONF_HARDCODED             1
#endif

//Needed when the definition above is set to 1
#ifndef IEEE_ADDR_CONF_ADDRESS
#define IEEE_ADDR_CONF_ADDRESS {0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01} //Last 2 bytes can be changed when doing the make operation (es. 00:02 for node 2...)
#endif

//Using 6lowpan fragmentation (costs alot in terms of memory occupancy)
#undef SICSLOWPAN_CONF_FRAG
#define SICSLOWPAN_CONF_FRAG 1

//Set max rest size to 128 to avoid bad CBOR packets format
#undef REST_MAX_CHUNK_SIZE
#define REST_MAX_CHUNK_SIZE 256


//---------------------------------------- If using SDN
#if NETSTACK_CONF_SDN == 1
#include "net/ipv6/multicast/uip-mcast6-engines.h"
#define UIP_MCAST6_CONF_ENGINE UIP_MCAST6_ENGINE_SDN

#define SINK 0 //I am not the sink

#undef COAP_MAX_OBSERVERS
#define COAP_MAX_OBSERVERS             1
/* Filtering .well-known/core per query can be disabled to save space. */
#undef COAP_LINK_FORMAT_FILTERING
#define COAP_LINK_FORMAT_FILTERING     1
#undef COAP_PROXY_OPTION_PROCESSING
#define COAP_PROXY_OPTION_PROCESSING   0

/* Turn off DAO ACK to make code smaller */
#undef RPL_CONF_WITH_DAO_ACK
#define RPL_CONF_WITH_DAO_ACK          0
//Turn off TCP
#undef UIP_CONF_TCP
#define UIP_CONF_TCP 0
//Turn off observing
#undef COAP_OBSERVING
#define COAP_OBSERVING 0
#undef COAP_BLOCK
#define COAP_BLOCK 0
#undef COAP_SEPARATE
#define COAP_SEPARATE 0

//Disable DAO messages to DODAG root, we want a DAO-Free WSN
#define RPL_DAO_ENABLE 0 

#define RPL_CALLBACK_PARENT_SWITCH sdn_rpl_callback_parent_switch
#define SDN_CALLBACK_ADD_NEIGHBOR sdn_callback_neighbor

//Configure max number of neighbors
#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS     10 //Default for zoul is 16, default for contiki is 8, do not go above 7 (otherwise change REST_MAX_CHUNK_SIZE to 256)

//Runs in non-storing mode, we don't need RPL for routing, only for forming the DODAG
#undef RPL_NS_CONF_LINK_NUM
#define RPL_NS_CONF_LINK_NUM 1 //1 link maintained
#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES 1 //0 routes
//#undef RPL_CONF_MOP
//#define RPL_CONF_MOP RPL_MOP_NON_STORING // Mode of operation, NON storing

//-------------------------------- ELSE, not using SDN
#else       

/* configure number of neighbors and routes */
#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#undef UIP_CONF_MAX_ROUTES
#define NBR_TABLE_CONF_MAX_NEIGHBORS     10
#define UIP_CONF_MAX_ROUTES   20 //Default value

#endif      //ENDIF SDN
//ADDED

#endif
