/**
 * \file
 *         sdn.c
 * \author
 *         Giulio Micheloni <giulio.micheloni@gmail.com>
 */


#include "net/netstack.h"
#include "net/sdn/sdn.h"
#include "net/packetbuf.h"
#include "net/ip/uip-debug.h"
#include "net/ipv6/sicslowpan.h"
#include "net/linkaddr.h"
#include "net/sdn/datapath.h"
#include "net/link-stats.h"

#define SDN_STATS 1
#if SDN_STATS
#include <stdio.h>
#define PRINT_STAT(...) printf(__VA_ARGS__)
#define PRINT_STAT_LLADDR(addr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7])
#define MESH_TAG_CONTROL_HL             0x0c    
#define MESH_TAG_DATA_HL                0x0e    
#define MESH_TAG_RPL_HL                 0x0b 
#define MESH_TAG_MULTICAST_HL           0x0a
#else
#define PRINT_STAT(...)
#define PRINT_STAT_LLADDR(addr)
#endif


#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(addr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif


#define MAX_NUM_MESH_ADDRS 5
#define MAX_NUM_IP_ADDRS 1



/*---------------------------------Shared data--------------------------------*/
static RPLconfig rpl_config;                           //It specifies how to treat RPL packets
static linkaddr_t* nodeMAC;                            //Pointer to the MAC address associated with the node 
static linkaddr_t meshAddrList[MAX_NUM_MESH_ADDRS];    //Array of link-layer addresses (unicast and multicast) 
                                                //which are associated with the node
static uip_ipaddr_t ipAddrList[MAX_NUM_IP_ADDRS];      //Array of ip-layer addresses (unicast and multicast) 
                                                //which are associated with the node
static uint8_t lastIndex;
static mac_callback_t sent_callback;
static void *ptr_copy;

/*
 * Initialization function which is called during the bootstrap 
 */
void
sdn_init(void)
{
    lastIndex = 0;
    memset(ipAddrList, 0, sizeof(uip_ipaddr_t) * MAX_NUM_IP_ADDRS);
    memset(meshAddrList, 0, sizeof(linkaddr_t) * MAX_NUM_MESH_ADDRS);
    rpl_config = RPL_BYPASS;
    nodeMAC = &linkaddr_node_addr;              //Useless with the native code, since the real MAC addres is being taken few seconds later the stack initialization
    linkaddr_copy(&meshAddrList[lastIndex], nodeMAC);   //Useless with the native code  
    lastIndex++; 
    datapath_init();
    PRINTF("SDN layer started");
}

int addMeshMulticastAddress(linkaddr_t* mesh_multicast){
    if(mesh_multicast == NULL)
        return 0;
    if(lastIndex == MAX_NUM_MESH_ADDRS)
        return 0;
    linkaddr_copy(&meshAddrList[lastIndex], mesh_multicast);
    lastIndex++;
    return 1;
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
        if((meshAddr->u8[0] & 0xe0) == 0x80)
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


int forward(){
#if DEBUG   
    const linkaddr_t* dest;
    dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
    PRINTF("SDN: Packet is being sent to: ");
    PRINTLLADDR(dest);
    PRINTF("\n");
#endif
    /*------------------------STAT---------------------------*/
#if SDN_STATS
    if(pktHasMeshHeader(packetbuf_dataptr()) == 1){
        uint8_t hopLimit;
        parseMeshHeader(packetbuf_dataptr(), &hopLimit, NULL, NULL, NULL, NULL);
        if(hopLimit == MESH_TAG_CONTROL_HL)
            PRINT_STAT("\nS_FC_%u\n", packetbuf_datalen());
	else if(hopLimit == MESH_TAG_RPL_HL)
	    PRINT_STAT("\nS_FR_%u\n", packetbuf_datalen());
        else if(hopLimit == MESH_TAG_MULTICAST_HL)
            PRINT_STAT("\nS_FM_%u\n", packetbuf_datalen());
        else
            PRINT_STAT("\nS_FD_%u\n", packetbuf_datalen());
    }
    else
        PRINT_STAT("\nS_FR_%u\n", packetbuf_datalen());
#endif
    /*-------------------------------------------------------*/
    NETSTACK_LLSEC.send(sent_callback, ptr_copy);
    //NETSTACK_MAC.send(NULL,NULL);
    return 1;
}

int toUpperLayer(){
    NETSTACK_NETWORK.input();
    return 1;
}

/*
 * Input function which is called by the lower layer when a packet is received 
 */
static void
input(void)
{
    linkaddr_t finalAddr;
    uint8_t finalAddrDim;
    uip_ipaddr_t ipAddr;
    int res;
    uint8_t* ptr = packetbuf_dataptr();
    sent_callback = NULL;
    ptr_copy = NULL;
    linkaddr_copy(&meshAddrList[0], &linkaddr_node_addr);       //Because of the native code takes few seconds to set the MAC address from the slip-radio program which runs onto the real node 
    struct queuebuf *q;
    /*-------------------------------DEBUG--------------------------------*/
#if DEBUG
    const linkaddr_t* source;
    PRINTF("SDN: Packet received from: ");
    source = packetbuf_addr(PACKETBUF_ADDR_SENDER);
    PRINTLLADDR(source);
    PRINTF("\n");
#endif
    /*-------------------------------STAT----------------------------------*/
#if SDN_STATS
    if(pktHasMeshHeader(ptr) == 1){
        uint8_t hopLimit;
        parseMeshHeader(ptr, &hopLimit, NULL, NULL, NULL, NULL);
        if(hopLimit == MESH_TAG_CONTROL_HL)
            PRINT_STAT("\nS_IC_%u\n", packetbuf_datalen());
	else if(hopLimit == MESH_TAG_RPL_HL)
	    PRINT_STAT("\nS_IR_%u\n", packetbuf_datalen());
        else if(hopLimit == MESH_TAG_MULTICAST_HL)
            PRINT_STAT("\nS_IM_%u\n", packetbuf_datalen());
        else
            PRINT_STAT("\nS_ID_%u\n", packetbuf_datalen());
    }
    else
        PRINT_STAT("\nS_IR_%u\n", packetbuf_datalen());
#endif
    
    /*------------------------------Logic------------------------------*/
    //Update link statistics
    link_stats_input_callback(packetbuf_addr(PACKETBUF_ADDR_SENDER));
    
    //Check if the packet has Mesh Header    
    if(pktHasMeshHeader(ptr) == 1){
        
#if DEBUG
        linkaddr_t origAddr;
        uint8_t origAddrDim;
        uint8_t hopLimit;
        parseMeshHeader(ptr, &hopLimit, &(finalAddr.u8), &finalAddrDim, &(origAddr.u8), &origAddrDim);
        PRINTF("Mesh: Hop Limit: %u", hopLimit);
        PRINTF(" Dim = %u Final Address: ", finalAddrDim);
        if(finalAddrDim == 8)
            PRINTLLADDR(&finalAddr);
        else
            printf("[%02x:%02x]", finalAddr.u8[0], finalAddr.u8[1]);
        PRINTF(" Dim = %u Orig. Address: ", origAddrDim);
        PRINTLLADDR(&origAddr);
        PRINTF("\n");
#endif
                
        //Read the Final Address from the Mesh Header
        memset(&finalAddr, 0, sizeof(finalAddr));
        parseMeshHeader(ptr, NULL, &(finalAddr.u8), &finalAddrDim, NULL, NULL);
        
        //Check if it is a mesh multicast address
        if(meshAddrIsMulticast(&finalAddr, finalAddrDim) == 1){
            PRINTF("SDN: Received a multicast packet\n");
            q = queuebuf_new_from_packetbuf();
            if(q == NULL) {
              PRINTF("SDN: could not allocate queuebuf multicast packet, dropping packet\n");
              return 0;
            }
            
            //Then check if the packet is addressed to this node
            if(meshAddrListContains(&finalAddr) == 1){
                PRINTF("SDN: Send multicast packet to upper layer\n");
                //This packet must be sent to the upper layer
                NETSTACK_NETWORK.input();
            }
                        
            queuebuf_to_packetbuf(q);
            queuebuf_free(q);
            q = NULL;
            
            //If so, first thing to do is to process it using the flow table            
            processPacket();
        }
        else{
            //Check if this packet is addressed to me
            if(meshAddrListContains(&finalAddr) == 1){
                //This packet must be sent to the upper layer
                PRINTF("Packet forwarded to 6LoWPAN layer\n");
                NETSTACK_NETWORK.input();
            }
            else{
                //Otherwise it must be handled by the Flow Table
                processPacket();
            }
        }
    }
    else{
                
        //Read the Destination Address from the IPv6 Header
        res = readIPaddr(&ipAddr);
        //DEBUG
        PRINTF("IP: ");
        PRINT6ADDR(&ipAddr);
        PRINTF("\n");
        //DEBUG
        if(res == -1){
            //Something went wrong, drop the packet
            return;
        }
        
        //Check if it is an RPL packet sent in multicast (all-rpl-nodes address)
        if(isRPLMulticast(&ipAddr) == 1){
            //If the configuration is RPL_BYPASS the RPL packet must 
            //be sent to the upper layer
            if(rpl_config == RPL_BYPASS){       
                NETSTACK_NETWORK.input();
            }
            else{
                //Otherwise handle it with the Flow Table
                processPacket();
            }
        }
        else{
            //Check if it is a multicast address
            if(uip_is_addr_mcast(&ipAddr) == 1){
                //If so, first thing to do is to process the packet with the Flow Table
                processPacket();
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
                PRINTF(" NO RPL PKT\n");
                //Check if this packet is addressed to me
                if(ipAddrListContains(&ipAddr) == 1){
                    //This packet must be sent to the upper layer
                    NETSTACK_NETWORK.input();
                }
                else{
                    //Otherwise it must be handled by the Flow Table
                    processPacket();
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
    uip_ipaddr_t ipAddr;
    int res = 0;
    
    sent_callback = sent;
    ptr_copy = ptr;
#if DEBUG == 1
    uint8_t* ptr_to_pkt = packetbuf_dataptr();
    linkaddr_t* dest;
    uint8_t hopLimit;
    linkaddr_t finalAddr;
    uint8_t finalAddrDim;
    linkaddr_t origAddr;
    uint8_t origAddrDim;
    dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
    PRINTF("SDN: Packet is being sent to: ");
    PRINTLLADDR(dest);
    PRINTF("\n");
    if(pktHasMeshHeader(ptr_to_pkt)){
        parseMeshHeader(ptr_to_pkt, &hopLimit, &(finalAddr.u8), &finalAddrDim, &(origAddr.u8), &origAddrDim);
        PRINTF("Mesh: Hop Limit: %u", hopLimit);
        PRINTF(" Dim = %u Final Address: ", finalAddrDim);
        if(finalAddrDim == 8)
            PRINTLLADDR(&finalAddr);
        else
            printf("[%02x:%02x]", finalAddr.u8[0], finalAddr.u8[1]);
        PRINTF(" Dim = %u Orig. Address: ", origAddrDim);
        PRINTLLADDR(&origAddr);
        PRINTF("\n");
    }
#endif
    
    /*------------------------------Logic------------------------------*/
        
    //Read the IPv6 destination address
    //res = readIPaddr(&ipAddr);
    res = copyDestIpAddress(&ipAddr);
    if(res == -1){
        PRINTF("SDN layer: Failed in reading the IP address");
        //Something went wrong, drop the packet
        return;
    }

    //Check if the packet is an RPL one sent in multicast (all-rpl-nodes address)
    if(isRPLMulticast(&ipAddr) == 1){
        //Check the current configuration
        if(rpl_config == RPL_BYPASS){
            //If the configuration is RPL_BYPASS the RPL packet must 
            //be sent to the lower layer
            forward();
        }
        else{
            //Otherwise handle the packet using the Flow Table
            processPacket();
        }
    }
    else{
        //Otherwise handle the packet using the Flow Table
        processPacket();
    }
}

const struct interceptor_driver sdn_driver = {
  "sdn",
  sdn_init,
  send,
  input
};


