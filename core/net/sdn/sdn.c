/**
 * \file
 *         sdn.c
 * \author
 *         Giulio Micheloni <giulio.micheloni@gmail.com>
 */


#include "net/netstack.h"
#include "net/sdn/sdn.h"
#include <stdio.h>
#include "net/packetbuf.h"
#include "net/ip/uip-debug.h"
#include "net/ipv6/sicslowpan.h"

#define DEBUG 1
#if DEBUG
#define PRINTADDR(addr) printf(" %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x ", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7])
#else
#define PRINTADDR(addr)
#endif

#define MAX_NUM_MESH_ADDRS 5
#define MAX_NUM_IP_ADDRS 5



/*---------------------------------Shared data--------------------------------*/
RPLconfig rpl_config;                           //It specifies how to treat RPL packets
linkaddr_t* nodeMAC;                            //Pointer to the MAC address associated with the node 
linkaddr_t meshAddrList[MAX_NUM_MESH_ADDRS];    //Array of link-layer addresses (unicast and multicast) 
                                                //which are associated with the node
uip_ipaddr_t ipAddrList[MAX_NUM_IP_ADDRS];      //Array of ip-layer addresses (unicast and multicast) 
                                                //which are associated with the node

/*
 * Initialization function which is called during the bootstrap 
 */
void
sdn_init(void)
{
    rpl_config = RPL_BYPASS;
    nodeMAC = &linkaddr_node_addr;
    linkaddr_copy(&meshAddrList[0], nodeMAC); 
    
    uip_ip6addr(&ipAddrList[0], 0xff02, 0, 0, 0, 0, 0, 0, 0x001a);
}

int meshAddrIsMulticast(linkaddr_t* meshAddr){
    if(meshAddr == NULL)
        return -1;
}

int ipAddrIsMulticast(uip_ipaddr_t* ipAddr){
    if(ipAddr == NULL)
        return -1;
}

int meshAddrListContains(linkaddr_t* meshAddr){
    if(meshAddr == NULL)
        return -1;
    int i;
    for(i = 0; i < MAX_NUM_MESH_ADDRS; i++){
        if(linkaddr_cmp(&meshAddrList[i], meshAddr) != 0)
            return 1;        
    }
    return 0;
}

int ipAddrListContains(uip_ipaddr_t* ipAddr){
    if(ipAddr == NULL)
        return -1;
    int i;
    for(i = 0; i < MAX_NUM_IP_ADDRS; i++){
        if(uip_ipaddr_cmp(&ipAddrList[i], ipAddr) != 0)
            return 1;        
    }
    return 0;
}

/*
 * Input function which is called by the lower layer when a packet is received 
 */
static void
input(void)
{
    linkaddr_t* source;
    linkaddr_t* dest;
    uip_ipaddr_t ipAddr;
    int res;
    printf("SDN: Packet received from: ");
    source = packetbuf_addr(PACKETBUF_ADDR_SENDER);
    dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
    PRINTADDR(source);
    printf(" to ");
    PRINTADDR(dest);
    printf(" IP: ");
    res = readIPaddr(&ipAddr);
    if(res != -1)
        uip_debug_ipaddr_print(&ipAddr);
    
    if(ipAddrListContains(&ipAddr) == 1 && rpl_config == RPL_BYPASS){
        printf(" RPL PKT!");
        printf("\n");
        NETSTACK_NETWORK.input();
    }
    else{
        printf(" NO RPL PKT\n");
        NETSTACK_NETWORK.input();
    }
}

/*
 * Send function which is called by the 6LoWPAN layer when it wants to send a packet 
 */
static void
send(mac_callback_t sent, void *ptr)
{
    linkaddr_t* source;
    linkaddr_t* dest;
    const linkaddr_t fixed = {0xc1, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05};
    const linkaddr_t new_dest = {0xc1, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
    dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
    printf("SDN: Packet is being sent to: ");
    PRINTADDR(dest);
    printf("\n");
    /*
    if(linkaddr_cmp(&fixed, nodeMAC) != 0){
        packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &new_dest);
    }
    */
    NETSTACK_LLSEC.send(sent, ptr);
}

const struct interceptor_driver sdn_driver = {
  "sdn",
  sdn_init,
  send,
  input
};



int forward(uint8_t nextLayer){
    return -1;
}