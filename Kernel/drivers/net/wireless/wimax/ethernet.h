#ifndef _WIBRO_ETHERNET_H
#define _WIBRO_ETHERNET_H

/* 
    Ethernet 
*/
#define ETHERNET_ADDRESS_LENGTH		6
// type/length field (not full list, only supported ones)
#define ETHERNET_TYPE_LEN_IP		0x0008		// IPv4
#define ETHERNET_TYPE_LEN_ARP		0x0608		// ARP
// ethernet header length
#define ETHERNET_HEADER_LENGTH		sizeof(eth_header_t)

/*
    ARP
*/
// hardware type (only supported)
#define ARP_HW_TYPE_ETHERNET		0x0100		// ethernet
// protocol type
#define ARP_PROTO_TYPE_IP4		0x0008		// IP
// opcode
#define ARP_OPCODE_REQUEST		0x0100
#define ARP_OPCODE_REPLY		0x0200

/*
    IPv4
*/
#define IPv4_VERSION			0x40
#define IPv4_LENGTH_MASK		0x0f
#define IPv4_LENGTH_MIN			5
#define IPv4_DATA_LEN_OFFSET		2
#define IPv4_PROTO_OFFSET		9
#define IPv4_PROTO_ICMP			0x01		// ICMP			
#define IPv4_ADDRESSES_OFFSET		12
#define IPv4_ADDRESS_LENGTH		4

/*
    ICMP
*/
#define ICMP_TYPE_LENGTH		1
#define ICMP_TYPE_ECHO_REPLY 		0x00		// echo reply
#define ICMP_TYPE_ECHO_REQUEST		0x08		// echo request
#define ICMP_CODE_LENGTH		1
#define ICMP_CHECKSUM_LENGTH		2
#define ICMP_ECHO_HEADER_LENGTH		(ICMP_TYPE_LENGTH + ICMP_CODE_LENGTH + ICMP_CHECKSUM_LENGTH + 4)

/*
    UDP (ports for selective suspend)
*/
#define UDP_SKIP1_PORT_SRC_DST		0x8900
#define UDP_SKIP2_PORT_SRC		0x4400
#define UDP_SKIP2_PORT_DST		0x4300
#define UDP_SKIP3_PORT_DST		0x8a00
#define UDP_SKIP4_PORT_DST		0xeb14
#define UDP_SKIP5_PORT_DST		0x6c07
#define UDP_SKIP6_PORT_DST		0x3500


/* 
    ETHER Header TYPE
*/ 
#define ETHERTYPE_HIM 0x1500
#define ETHERTYPE_MC 0x1501
#define ETHERTYPE_DM 0x1502
#define ETHERTYPE_CT  0x1503
#define ETHERTYPE_VSP 0x1510
#define ETHERTYPE_DL 0x1504
#define ETHERTYPE_AUTH 0x1521

#define WIBRO_TYPE_SIZE		2
#define WIBRO_LENGTH_SIZE		2

#define WIBRO_TYPE_OFFSET		0
#define WIBRO_LENGTH_OFFSET	2
#define WIBRO_DATA_OFFSET		4

///////////////////////// FRAME FORMAT////////////////////////////

#define IP				0x0800
#define ARP				0x0806
#define IPV6			0x86dd
#define UDP				0x11
#define TCP				0x06

#define REQUEST			0x0001
#define ARP_RESPONSE	0x0002

#define BOOTPC			0x44
#define BOOTPS			0x43


#pragma pack(1)
/*
    Types
*/
//typedef UCHAR eth_addr_t[ETHERNET_ADDRESS_LENGTH];
typedef unsigned char eth_addr_t[ETHERNET_ADDRESS_LENGTH];


typedef struct {
    eth_addr_t	    destination;
    eth_addr_t	    source;
    unsigned short	    type;
} eth_header_t;

typedef struct {
    eth_header_t    header;
    unsigned short	    type_hw;
    unsigned short	    type_proto;
    unsigned char	    len_hw;
    unsigned char	    len_proto;
    unsigned short	    opcode;
} eth_arp_packet_t;

typedef struct {
	UCHAR	dst[ETHERNET_ADDRESS_LENGTH];
	UCHAR	src[ETHERNET_ADDRESS_LENGTH];
	USHORT 	type;
	USHORT 	him_type;
	USHORT 	subtype;
	USHORT 	length;
}ctl_pkt_t;

// modem up notification
typedef struct {
	UCHAR	data[2];
} him_modem_up_noti_t;

#pragma pack()

/*
    Functions
*/
//unsigned char ethIsArpRequest(unsigned char *packet, unsigned long length);
//void ethSendArpResponse(cmc710_t  *cmc710, unsigned char *packet, unsigned long length);

#endif

