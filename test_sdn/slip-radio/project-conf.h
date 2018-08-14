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
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#undef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM          40

#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE       400

#undef UIP_CONF_ROUTER
#define UIP_CONF_ROUTER            0

#define CMD_CONF_OUTPUT slip_radio_cmd_output

/* add the cmd_handler_cc2420 + some sensors if TARGET_SKY */
#ifdef CONTIKI_TARGET_SKY
#define CMD_CONF_HANDLERS slip_radio_cmd_handler,cmd_handler_cc2420
#define SLIP_RADIO_CONF_SENSORS slip_radio_sky_sensors
/* add the cmd_handler_rf230 if TARGET_NOOLIBERRY. Other RF230 platforms can be added */
#elif CONTIKI_TARGET_NOOLIBERRY
#define CMD_CONF_HANDLERS slip_radio_cmd_handler,cmd_handler_rf230
#elif CONTIKI_TARGET_ECONOTAG
#define CMD_CONF_HANDLERS slip_radio_cmd_handler,cmd_handler_mc1322x
#else
#define CMD_CONF_HANDLERS slip_radio_cmd_handler
#endif


/* configuration for the slipradio/network driver */
#undef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC     nullmac_driver

#undef NETSTACK_CONF_RDC
/* #define NETSTACK_CONF_RDC     nullrdc_noframer_driver */
#define NETSTACK_CONF_RDC     contikimac_driver

#undef NETSTACK_CONF_NETWORK
#define NETSTACK_CONF_NETWORK slipnet_driver

#undef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER no_framer

#undef CC2420_CONF_AUTOACK
#define CC2420_CONF_AUTOACK              1

#undef UART1_CONF_RX_WITH_DMA
#define UART1_CONF_RX_WITH_DMA           1

//Set to 1 in order to hardcode the MAC address
#ifndef IEEE_ADDR_CONF_HARDCODED
#define IEEE_ADDR_CONF_HARDCODED             1
#endif

//Needed when the definition above is set to 1
#ifndef IEEE_ADDR_CONF_ADDRESS
#define IEEE_ADDR_CONF_ADDRESS {0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01} //Last 2 bytes can be changed when doing the make operation
#endif

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

//Using RPL OF Zero (hop count) instead of etx
#undef RPL_CONF_OF_OCP
#define RPL_CONF_OF_OCP RPL_OCP_OF0

//Disable DAOs
#define RPL_DAO_ENABLE 0 

//Configure max number of neighbors
#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS 10

//Set max rest size to 128 to avoid bad CBOR packets format
#undef REST_MAX_CHUNK_SIZE
#define REST_MAX_CHUNK_SIZE 256

//Runs in non-storing mode, we don't need RPL for routing, only for forming the DODAG
#undef RPL_NS_CONF_LINK_NUM
#define RPL_NS_CONF_LINK_NUM 1 //1 link maintained
#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES 1 //1 route

#undef COAP_MAX_OBSERVERS
#define COAP_MAX_OBSERVERS             1
// Filtering .well-known/core per query can be disabled to save space. 
#undef COAP_LINK_FORMAT_FILTERING
#define COAP_LINK_FORMAT_FILTERING     1
#undef COAP_PROXY_OPTION_PROCESSING
#define COAP_PROXY_OPTION_PROCESSING   0

//Disable TCP
#undef UIP_CONF_TCP
#define UIP_CONF_TCP 0

#endif /* PROJECT_CONF_H_ */
