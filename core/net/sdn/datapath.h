/**
 * \file
 *         datapath.h
 * \author
 *         Giulio Micheloni <giulio.micheloni@gmail.com>
 */

#ifndef DATAPATH_H
#define	DATAPATH_H

#define STATUS_REGISTER_SIZE    16
#define COPY_BUFFER_SIZE        16

#include "flowtable.h"
#include "net/sdn/sdn.h"
#include "net/sdn/control_agent.h"
#include "net/ipv6/sicslowpan.h"
#include "net/packetbuf.h"
#include "net/linkaddr.h"
#include "sys/ctimer.h"

#define MAX_BROADCAST_PKTS 4

typedef struct broadcast_pkt{
    struct queuebuf *broadcast_packet;
} broadcast_pkt_t; 

void datapath_init();
int processPacket();
uint8_t* getStateRegisterPtr(uint8_t* dim);

#endif	/* DATAPATH_H */

