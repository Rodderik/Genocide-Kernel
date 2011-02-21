#include "headers.h"


const eth_addr_t eaddr_broadcast = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/*
    Forward declarations
*/
static char ethIsBroadcast(unsigned char *packet)
{
	ENTER;
    if (!memcmp(packet, eaddr_broadcast, ETHERNET_ADDRESS_LENGTH)) {
		return 1;
    }
	LEAVE;
    return 0;
}

static char ethIsArpGratuitous(unsigned char *packet)
{
	int i;
	unsigned char *bd, *bs;
    eth_arp_packet_t* p = (eth_arp_packet_t*)packet;
    
	ENTER;
    bs = packet + sizeof(eth_arp_packet_t) + p->len_hw;
    bd = packet + sizeof(eth_arp_packet_t) + (p->len_hw * 2) + p->len_proto;
    
	if (!memcmp(bs, bd, p->len_proto)) 
		return TRUE;

    for (i = 0; i < p->len_proto; i++) {
		if (bs[i]) {
			return FALSE;
		}
	}
	LEAVE;
    return TRUE;
}

// is ARP packet?
BOOLEAN ethIsArpRequest(unsigned char *packet, unsigned long length)
{
    eth_arp_packet_t* p = (eth_arp_packet_t*)packet;
	ENTER;

    // check type
    if (p->header.type != ETHERNET_TYPE_LEN_ARP) {
		//DumpDebug(DUMP_INFO, "WiBro USB: Other type (%04x)", p->header.type);
		return FALSE;
    }
    if (p->opcode != ARP_OPCODE_REQUEST) {
		//DumpDebug(DUMP_INFO, "WiBro USB: Not ARP request (%04x)", p->opcode);
		return FALSE;
    }
    // check length
    if (length < (sizeof(eth_arp_packet_t) + (p->len_hw * 2) + (p->len_proto * 2))) {
		//DumpDebug(DUMP_INFO, "WiBro USB: Short packet (%lu)", length);
		return FALSE;
    }
    // check hw type
    if (p->type_hw != ARP_HW_TYPE_ETHERNET) {
		//DumpDebug(DUMP_INFO, "WiBro USB: Other hw type (%04x)", p->type_hw);
		return FALSE;
    }
    if (p->type_proto != ARP_PROTO_TYPE_IP4) {
		//DumpDebug(DUMP_INFO, "WiBro USB: Other protocol type (%04x)", p->type_proto);
		return FALSE;
    }
    if (p->len_hw != ETHERNET_ADDRESS_LENGTH) {
		//DumpDebug(DUMP_INFO, "WiBro USB: Wrong Ethernet header length (%04x)", p->len_hw);
		return FALSE;
    }
    
	LEAVE;
	return TRUE;
}

// send response to ARP packet
// packet = skb->data
void ethSendArpResponse(MINIPORT_ADAPTER *Adapter, unsigned char *packet, unsigned long length)
{
	unsigned char *b1, *b2;
    eth_arp_packet_t* d;				// destination packet
    eth_arp_packet_t* s = (eth_arp_packet_t*)packet;	// source packet
	struct sk_buff *dsc;

	ENTER;
    // ARP: who has our host IP?
    if (ethIsArpGratuitous(packet)) 
	{
		Adapter->stats.arp.G++;
		// skip it
		DumpDebug(DUMP_INFO, "WiBro USB: Gratuitous ARP");
		return;
    }
	
	dsc = dev_alloc_skb(MINIPORT_MAX_TOTAL_SIZE + 2);
    // prepare response
	// fill out data
	d = (eth_arp_packet_t*)dsc->data;
	// constant part
	d->header.type = ETHERNET_TYPE_LEN_ARP;
	d->type_hw     = ARP_HW_TYPE_ETHERNET;
	d->type_proto  = ARP_PROTO_TYPE_IP4;
	d->len_hw      = ETHERNET_ADDRESS_LENGTH;
//	d->len_proto   = IPv4_ADDRESS_LENGTH; //sangam : latest winxp driver ported
	d->len_proto   = s->len_proto; 
	d->opcode      = ARP_OPCODE_REPLY;

	// exchange IP addresses
	b1 = dsc->data + sizeof(eth_arp_packet_t) + (s->len_hw * 2) + s->len_proto;	// dst proto
	b2 = packet + sizeof(eth_arp_packet_t) + s->len_hw;				// src proto
	memcpy(b1, b2, s->len_proto);
	b1 = dsc->data + sizeof(eth_arp_packet_t) + s->len_hw;				// src proto
	b2 = packet + sizeof(eth_arp_packet_t) + (s->len_hw * 2) + s->len_proto;	// dst proto
	memcpy(b1, b2, s->len_proto);

	// exchange MAC addresses
	if (ethIsBroadcast(packet)) 
	{ 	
		// make new MAC address
    	UCHAR mac [ETHERNET_ADDRESS_LENGTH];
    	DumpDebug(DUMP_INFO, "WiBro USB: Broadcast ARP");
    	// copy source address
    	memcpy(mac, s->header.source, ETHERNET_ADDRESS_LENGTH);
    	// add 1 to last byte
    	mac[ETHERNET_ADDRESS_LENGTH - 1] += 1;
    	DumpDebug(DUMP_INFO, "WiBro USB: Source MAC = %02x:%02x:%02x:%02x:%02x:%02x, New MAC = %02x:%02x:%02x:%02x:%02x:%02x", 
		s->header.source[0], s->header.source[1], s->header.source[2], s->header.source[3], s->header.source[4], s->header.source[5],
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    	
		// header
    	memcpy(d->header.destination, s->header.source, ETHERNET_ADDRESS_LENGTH);
    	memcpy(d->header.source, mac, ETHERNET_ADDRESS_LENGTH);
    	
		// body
	    b1 = dsc->data + sizeof(eth_arp_packet_t) + s->len_hw + s->len_proto;		// dst hw
	    b2 = packet + sizeof(eth_arp_packet_t);					// src hw
	    memcpy(b1, b2, ETHERNET_ADDRESS_LENGTH);
	    b1 = dsc->data + sizeof(eth_arp_packet_t);					// src hw
	    memcpy(b1, mac, ETHERNET_ADDRESS_LENGTH);
	}
	else { // simple exchange
    	DumpDebug(DUMP_INFO, "WiBro USB: Direct ARP");
    	// header
    	memcpy(d->header.destination, s->header.source, ETHERNET_ADDRESS_LENGTH);
    	memcpy(d->header.source, s->header.destination, ETHERNET_ADDRESS_LENGTH);
    	// body
	    b1 = dsc->data + sizeof(eth_arp_packet_t) + s->len_hw + s->len_proto;		// dst hw
	    b2 = packet + sizeof(eth_arp_packet_t);					// src hw
	    memcpy(b1, b2, ETHERNET_ADDRESS_LENGTH);
	    b1 = dsc->data + sizeof(eth_arp_packet_t);					// src hw
	    b2 = packet + sizeof(eth_arp_packet_t) + s->len_hw + s->len_proto;		// dst hw
	    memcpy(b1, b2, ETHERNET_ADDRESS_LENGTH);
	}
		// copy rest of packet

	if (length > (sizeof(eth_arp_packet_t) + (s->len_hw * 2) + (s->len_proto * 2))) {
	    ULONG  l = sizeof(eth_arp_packet_t) + (s->len_hw * 2) + (s->len_proto * 2);
	    memcpy(dsc->data + l, packet + l, length - l);
	}

	// indicate to stack
	dsc->dev = Adapter->net;
		
	skb_put(dsc, length);
		
	dsc->protocol = eth_type_trans(dsc, Adapter->net);
	Adapter->net->last_rx = jiffies;
	// add statistics
	Adapter->netstats.rx_packets++;
	Adapter->netstats.rx_bytes += dsc->len;		
	Adapter->stats.rx.Bytes += length;
	Adapter->stats.rx.Data++;
	Adapter->stats.arp.Count++;

	netif_rx(dsc);
    DumpDebug(DUMP_INFO, "Sent the Arp Response");
		
    LEAVE;
    return;
}

