/**
 * \file
 *         control_agent.h
 * \author
 *         Giulio Micheloni <giulio.micheloni@gmail.com>
 */

#ifndef CONTROL_AGENT_H
#define	CONTROL_AGENT_H

#define REST coap_rest_implementation

#include "contiki.h"
#include "net/sdn/datapath.h"
#include "net/sdn/flowtable.h"
#include "net/ipv6/sicslowpan.h"
#include "lib/ringbufindex.h"
#include "net/rpl/rpl.h"
#include "sys/ctimer.h"
#include "er-coap-engine.h"
#include "net/linkaddr.h"
#include "net/nbr-table.h"
#include "net/link-stats.h"
#include "net/sdn/cn-cbor.h"
#include "net/queuebuf.h"
//#include "dev/battery-sensor.h"

#define TTL_INTERVAL		(1 * CLOCK_SECOND)
#define TOP_UPDATE_FIRST       (1 * 60 * CLOCK_SECOND)             //First topology update after one minute
#define TOP_UPDATE_SECOND       (3 * 60 * CLOCK_SECOND)           //Second topology update after 3 minutes starting from the first one
#define TOP_UPDATE_PERIOD       (5 * 60 * CLOCK_SECOND)         //After the second update, send reports once every 5 minutes
#define MAX_DIM_PAYLOAD         8 + (16 * NBR_TABLE_CONF_MAX_NEIGHBORS)            
#define MAX_REQUEST             4       //Must be a power of two

typedef enum {
    NONE,
    TABLE_MISS,
    TOPOLOGY_UPDATE
} request_type_t;

typedef struct request{
    coap_packet_t req_packet;
    request_type_t type;
} request_t;

typedef struct pending_request{
    linkaddr_t mesh_address;
    struct timer lifetime_timer;
    uint8_t valid;          //0: not valid, 1: valid
    struct queuebuf* payload;
} pending_request_t;  

void handleTableMiss(linkaddr_t* L2_receiver, linkaddr_t* L2_sender, uint8_t* ptr_to_pkt, uint16_t pkt_dim);
void sdn_callback_neighbor(const linkaddr_t *addr);
void control_agent_init();
//By Simone
void remove_sdn_rule (const linkaddr_t *addr);

#endif	/* CONTROL_AGENT_H */

