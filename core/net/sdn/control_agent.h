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


#define TTL_INTERVAL		(60 * CLOCK_SECOND)
#define MAX_REQUEST             4       //Must be a power of two

typedef enum {
    TABLE_MISS,
    TOPOLOGY_UPDATE
} request_type_t;

typedef struct request{
    coap_packet_t req_packet;
    request_type_t type;
} request_t;

void sdn_callback_neighbor(const linkaddr_t *addr);
void control_agent_init();

#endif	/* CONTROL_AGENT_H */

