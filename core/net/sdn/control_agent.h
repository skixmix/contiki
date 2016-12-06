/**
 * \file
 *         control_agent.h
 * \author
 *         Giulio Micheloni <giulio.micheloni@gmail.com>
 */

#ifndef CONTROL_AGENT_H
#define	CONTROL_AGENT_H

#include "net/sdn/datapath.h"
#include "net/sdn/flowtable.h"
#include "net/ipv6/sicslowpan.h"
#include "net/rpl/rpl.h"
#include "sys/ctimer.h"

#define TTL_INTERVAL		(1 * CLOCK_SECOND)

void sdn_callback_neighbor(const linkaddr_t *addr);
void control_agent_init();

#endif	/* CONTROL_AGENT_H */

