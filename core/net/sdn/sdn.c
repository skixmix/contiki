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
    memset(ipAddrList, 0, sizeof(uip_ipaddr_t) * MAX_NUM_IP_ADDRS);
    memset(meshAddrList, 0, sizeof(linkaddr_t) * MAX_NUM_MESH_ADDRS);
    rpl_config = RPL_BYPASS;
    nodeMAC = &linkaddr_node_addr;
    linkaddr_copy(&meshAddrList[0], nodeMAC); 
    
}

int isRPLMulticast(uip_ipaddr_t* ipAddr){
    uip_ipaddr_t all_rpl_nodes;
    uip_ip6addr(&all_rpl_nodes, 0xff02, 0, 0, 0, 0, 0, 0, 0x001a);
    if(uip_ipaddr_cmp(&all_rpl_nodes, ipAddr) != 0)
            return 1;
    return 0;
}

int meshAddrIsMulticast(linkaddr_t* meshAddr, uint8_t meshAddrDim){
    if(meshAddr == NULL)
        return -1;
    if(meshAddrDim == 2)
        if((meshAddr->u8[6] & 0xe0) == 0x80)
            return 1;
        else
            return 0;
    else
        //Let us assume that in our implementation when it comes an MAC address
        //64-bit long, it will never be a multicast one
        return 0;
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
    linkaddr_t finalAddr;
    uint8_t finalAddrDim;
    uip_ipaddr_t ipAddr;
    int res;
    uint8_t* ptr = packetbuf_dataptr();
    
    /*-------------------------------DEBUG--------------------------------*/
    printf("SDN: Packet received from: ");
    source = packetbuf_addr(PACKETBUF_ADDR_SENDER);
    dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
    PRINTADDR(source);
    
    /*------------------------------Logic------------------------------*/
    
    //Check if the packet has Mesh Header    
    if(pktHasMeshHeader(ptr) == 1){
        
        //DEBUG
        linkaddr_t origAddr;
        uint8_t origAddrDim;
        uint8_t hopLimit;
        parseMeshHeader(ptr, &hopLimit, &finalAddr, &finalAddrDim, &origAddr, &origAddrDim);
        printf("Mesh: Hop Limit: %u", hopLimit);
        printf(" Dim = %u Final Address: ", finalAddrDim);
        PRINTADDR(&finalAddr);
        printf(" Dim = %u Orig. Address: ", origAddrDim);
        PRINTADDR(&origAddr);
        printf("\n");
        //DEBUG
                
        //Read the Final Address from the Mesh Header
        parseMeshHeader(ptr, NULL, &finalAddr, &finalAddrDim, NULL, NULL);
        
        //Check if it is a mesh multicast address
        if(meshAddrIsMulticast(&finalAddr, finalAddrDim) == 1){
            //If so, first thing to do is to process it using the flow table
            
            /*
             * NOTE: the flow table could (and likely will) modify the packet.
             * Thus, if it turns out that the packet must be sent ALSO 
             * to the upper layer, we need to make a copy of the actual content
             * of the packet buffer, before sending it to the flow table.
            */
            
            //Then check if the packet is addressed to this node
            if(meshAddrListContains(&finalAddr) == 1){
                //This packet must be sent to the upper layer
                NETSTACK_NETWORK.input();
            }
        }
        else{
            //Check if this packet is addressed to me
            if(meshAddrListContains(&finalAddr) == 1){
                //This packet must be sent to the upper layer
                NETSTACK_NETWORK.input();
            }
            else{
                //Otherwise it must be handled by the Flow Table

                /*
                const linkaddr_t fixed = {0xc1, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
                const linkaddr_t new_dest = {0xc1, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};
                if(linkaddr_cmp(&fixed, nodeMAC) != 0){
                    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &new_dest);
                    printf("Set the Next Hop Addr = ");
                    PRINTADDR(&new_dest);
                    printf("\n");
                }
                NETSTACK_LLSEC.send(NULL, NULL);
                */
            }
        }
    }
    else{
        
        //Read the Destination Address from the IPv6 Header
        res = readIPaddr(&ipAddr);
        //DEBUG
        printf(" IP: ");
        res = readIPaddr(&ipAddr);
        if(res != -1)
            uip_debug_ipaddr_print(&ipAddr);
        //DEBUG
        if(res == -1){
            //Something went wrong, drop the packet
            return;
        }
        
        //Check if it is an RPL packet sent in multicast (all-rpl-nodes address)
        if(isRPLMulticast(&ipAddr) == 1){
            printf(" RPL PKT!");
            printf("\n");
            //If the configuration is RPL_BYPASS the RPL packet must 
            //be sent to the upper layer
            if(rpl_config == RPL_BYPASS){       
                NETSTACK_NETWORK.input();
            }
            else{
                //Otherwise handle it with the Flow Table
            }
        }
        else{
            //Check if it is a multicast address
            if(uip_is_addr_mcast(&ipAddr) == 1){
                //If so, first thing to do is to process the packet with the Flow Table
                
                /*
                * NOTE: the flow table could (and likely will) modify the packet.
                * Thus, if it turns out that the packet must be sent ALSO 
                * to the upper layer, we need to make a copy of the actual content
                * of the packet buffer, before sending it to the flow table.
                */
                
                //Then check if this packet belongs to a multicast group that I've joined
                if(ipAddrListContains(&ipAddr) == 1){
                    //This packet must be sent to the upper layer
                    NETSTACK_NETWORK.input();
                }
            }
            else{
                //If we are here it means that the packet doesn't belong 
                //to the multicast of RPL 
                printf(" NO RPL PKT\n");
                //Check if this packet is addressed to me
                if(ipAddrListContains(&ipAddr) == 1){
                    //This packet must be sent to the upper layer
                    NETSTACK_NETWORK.input();
                }
                else{
                    //Otherwise it must be handled by the Flow Table
                }
            }
        }    
    }
    
}

/*
 * Send function which is called by the 6LoWPAN layer when it wants to send a packet 
 */
static void
send(mac_callback_t sent, void *ptr)
{
    linkaddr_t finalAddr;
    uint8_t finalAddrDim;
    linkaddr_t origAddr;
    uint8_t origAddrDim;
    linkaddr_t* source;
    linkaddr_t* dest;
    uint8_t hopLimit;
    uint8_t* ptr_to_pkt = packetbuf_dataptr();
    uip_ipaddr_t ipAddr;
    int res;
    
    //DEBUG
    if(pktHasMeshHeader(ptr_to_pkt)){
        dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
        printf("SDN: Packet is being sent to: ");
        PRINTADDR(dest);
        printf("\n");
        parseMeshHeader(ptr_to_pkt, &hopLimit, &finalAddr, &finalAddrDim, &origAddr, &origAddrDim);
        printf("Mesh: Hop Limit: %u", hopLimit);
        printf(" Dim = %u Final Address: ", finalAddrDim);
        PRINTADDR(&finalAddr);
        printf(" Dim = %u Orig. Address: ", origAddrDim);
        PRINTADDR(&origAddr);
        printf("\n");
    }
    //DEBUG
    
    /*------------------------------Logic------------------------------*/
    
    //Read the IPv6 destination address
    res = readIPaddr(&ipAddr);
    if(res == -1){
        //Something went wrong, drop the packet
        return;
    }

    //Check if the packet is an RPL one sent in multicast (all-rpl-nodes address)
    if(isRPLMulticast(&ipAddr) == 1){
        //Check the current configuration
        if(rpl_config == RPL_BYPASS){
            //If the configuration is RPL_BYPASS the RPL packet must 
            //be sent to the lower layer
            NETSTACK_LLSEC.send(sent, ptr);
        }
        else{
            //Otherwise handle the packet using the Flow Table
        }
    }
    else{
        //Otherwise handle the packet using the Flow Table
    }
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