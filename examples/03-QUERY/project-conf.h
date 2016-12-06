#undef IEEE802154_CONF_PANID
#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC     nullrdc_driver
#undef NULLRDC_CONF_802154_AUTOACK
#define NULLRDC_CONF_802154_AUTOACK       1
//#define REST_MAX_CHUNK_SIZE    64
#define REST_MAX_CHUNK_SIZE    16
#undef COAP_MAX_OPEN_TRANSACTIONS
#define COAP_MAX_OPEN_TRANSACTIONS   4

/* Save some memory for the sky platform. */
#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS     4
#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES   4
#undef UIP_CONF_BUFFER_SIZE
//#define UIP_CONF_BUFFER_SIZE  280
#define UIP_CONF_BUFFER_SIZE    140

#if NETSTACK_CONF_SDN == 1
#undef UIP_CONF_TCP
#define UIP_CONF_TCP 0
#define RPL_CALLBACK_PARENT_SWITCH sdn_rpl_callback_parent_switch
#define SDN_CALLBACK_ADD_NEIGHBOR sdn_callback_neighbor
#endif

