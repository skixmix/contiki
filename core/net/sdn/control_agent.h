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

void sdn_callback_neighbor(const linkaddr_t *addr);

#endif	/* CONTROL_AGENT_H */

