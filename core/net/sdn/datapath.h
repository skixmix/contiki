/**
 * \file
 *         datapath.h
 * \author
 *         Giulio Micheloni <giulio.micheloni@gmail.com>
 */

#ifndef DATAPATH_H
#define	DATAPATH_H

#define STATUS_REGISTER_SIZE    50
#define COPY_BUFFER_SIZE        16

#include "flowtable.h"
#include "sdn.h"
#include "net/ipv6/sicslowpan.h"
#include "net/packetbuf.h"
#include "net/linkaddr.h"

int matchPacket();
uint8_t* getStatusRegisterPtr(uint8_t* dim);

#endif	/* DATAPATH_H */

