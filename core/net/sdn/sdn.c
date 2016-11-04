/**
 * \file
 *         sdn.c
 * \author
 *         Giulio Micheloni <giulio.micheloni@gmail.com>
 */


#include "net/netstack.h"
#include "net/sdn/sdn.h"
#include <stdio.h>

void
sdn_init(void)
{
    
}

static void
input(void)
{
    printf("SDN MODULE: input %i\n", i);
    NETSTACK_NETWORK.input();
}

static void
send(mac_callback_t sent, void *ptr)
{
    printf("SDN MODULE: send %i\n", i);
    NETSTACK_LLSEC.send(sent, ptr);
}

const struct interceptor_driver sdn_driver = {
  "sdn",
  sdn_init,
  send,
  input
};
