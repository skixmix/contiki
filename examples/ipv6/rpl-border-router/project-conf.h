/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef PROJECT_ROUTER_CONF_H_
#define PROJECT_ROUTER_CONF_H_

#ifndef WITH_NON_STORING
#define WITH_NON_STORING 0 /* Set this to run with non-storing mode */
#endif /* WITH_NON_STORING */

#if WITH_NON_STORING
#undef RPL_NS_CONF_LINK_NUM
#define RPL_NS_CONF_LINK_NUM 40 /* Number of links maintained at the root */
#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES 5 /* No need for routes */
#undef RPL_CONF_MOP
#define RPL_CONF_MOP RPL_MOP_NON_STORING /* Mode of operation*/
#endif /* WITH_NON_STORING */

#ifndef UIP_FALLBACK_INTERFACE
#define UIP_FALLBACK_INTERFACE rpl_interface
#endif

#ifndef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM          8
#endif

#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE    140
#endif

#ifndef UIP_CONF_RECEIVE_WINDOW
#define UIP_CONF_RECEIVE_WINDOW  60
#endif

//ADDED
#undef UIP_CONF_TCP
#define UIP_CONF_TCP 0
#undef SICSLOWPAN_CONF_FRAG
#define SICSLOWPAN_CONF_FRAG 1
#define TESTBED 1


#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC     nullrdc_driver
#undef NULLRDC_CONF_802154_AUTOACK
#define NULLRDC_CONF_802154_AUTOACK       1

#if NETSTACK_CONF_SDN == 1
#include "net/ipv6/multicast/uip-mcast6-engines.h"
#define UIP_MCAST6_CONF_ENGINE UIP_MCAST6_ENGINE_SDN
#define SINK 1


#undef COAP_MAX_OBSERVERS
#define COAP_MAX_OBSERVERS             1
/* Filtering .well-known/core per query can be disabled to save space. */
#undef COAP_LINK_FORMAT_FILTERING
#define COAP_LINK_FORMAT_FILTERING     1
#undef COAP_PROXY_OPTION_PROCESSING
#define COAP_PROXY_OPTION_PROCESSING   0

/* Turn of DAO ACK to make code smaller */
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
#define UIP_CONF_ND6_SEND_NA 0
#undef RPL_CONF_STATS
#define RPL_CONF_STATS 0
#undef RPL_CONF_MAX_DAG_PER_INSTANCE
#define RPL_CONF_MAX_DAG_PER_INSTANCE 1
#undef PROCESS_CONF_STATS
#define PROCESS_CONF_STATS 0


#define RPL_DAO_ENABLE 0

#define RPL_CALLBACK_PARENT_SWITCH sdn_rpl_callback_parent_switch
#define SDN_CALLBACK_ADD_NEIGHBOR sdn_callback_neighbor

#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES 1 
#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS 15

#else       //ELSE

#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES 40 
#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS 15

#endif      //ENDIF
//ADDED

#ifndef WEBSERVER_CONF_CFS_CONNS
#define WEBSERVER_CONF_CFS_CONNS 0
#endif

#undef SLIP_ARCH_CONF_ENABLE
#define SLIP_ARCH_CONF_ENABLE 1

//For IoT-LAB testbed
#define RF2XX_TX_POWER  PHY_POWER_m10dBm
#define RF2XX_RX_RSSI_THRESHOLD  RF2XX_PHY_RX_THRESHOLD__m81dBm
#undef  RF2XX_CHANNEL
#define RF2XX_CHANNEL   16



#endif /* PROJECT_ROUTER_CONF_H_ */
