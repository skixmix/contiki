
#include "contiki.h"
#include "contiki-net.h"
#include "net/ipv6/multicast/uip-mcast6.h"
#include "net/ipv6/multicast/sdn_multicast_engine.h"
#include "net/rpl/rpl.h"
#include "net/netstack.h"
#include <string.h>

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

/*---------------------------------------------------------------------------*/
/* Macros */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Internal Data */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* uIPv6 Pointers */
/*---------------------------------------------------------------------------*/
#define UIP_IP_BUF        ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
/*---------------------------------------------------------------------------*/
#ifndef SINK
#define SINK 0
#endif
static uint8_t
in()
{    
#if SINK == 1
    UIP_IP_BUF->ttl--;
    tcpip_output(NULL);
    UIP_IP_BUF->ttl++;
#endif
    
    if(uip_ds6_maddr_lookup(&UIP_IP_BUF->destipaddr) != NULL){
        return UIP_MCAST6_ACCEPT;
    }
    else
        return UIP_MCAST6_DROP;
}
/*---------------------------------------------------------------------------*/
static void
init()
{
    
}
/*---------------------------------------------------------------------------*/
static void
out()
{
    return;
}
/*---------------------------------------------------------------------------*/
const struct uip_mcast6_driver sdn_multicast_driver = {
  "SDN_multicast",
  init,
  out,
  in,
};
/*---------------------------------------------------------------------------*/
/** @} */

