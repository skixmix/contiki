/**
 * \file
 *         sdn.h
 * \author
 *         Giulio Micheloni <giulio.micheloni@gmail.com>
 */


#ifndef SDN_H_
#define SDN_H_

#include "net/sdn/interceptor.h"

extern const struct interceptor_driver sdn_driver;

typedef enum {
    RPL_BYPASS,
    NO_RPL_BYPASS
} RPLconfig;

/**
 * Function called by the Datapath module thanks to which it can forward a 
 * packet to the MAC layer.
 * \return An integer equals to -1 if some error occurs or if the parameter is 
 * not valid
 */
int forward();

int toUpperLayer();

int addMeshMulticastAddress(linkaddr_t* mesh_multicast);

#endif /* SDN_H_ */

