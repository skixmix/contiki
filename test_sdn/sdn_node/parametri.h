//Using SDN
#undef NETSTACK_CONF_SDN
#define NETSTACK_CONF_SDN 1 //0 = off | 1 = on
//SDN
//Send NA (needed for updating the neighbor table)
#undef UIP_CONF_ND6_SEND_NA
#define UIP_CONF_ND6_SEND_NA 1