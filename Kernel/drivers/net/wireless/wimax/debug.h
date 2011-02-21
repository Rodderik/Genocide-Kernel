
#ifndef _WIBRO_DEBUG_H
#define _WIBRO_DEBUG_H

#define ALL 0

//------------TX------------------------------//
// send.c ,Arp.c, Dhcp.c, LeakyBucket.c, And Qos.c
// total 17 macros
///////////////Send.c////////////////////////
#define TX 3      //0011
#define MP_SEND		(1<<0)
#define NEXT_SEND   (1<<1)
#define TX_FIFO		(1<<2)
#define TX_CONTROL	(1<<3)

////////////Arp.c//////////////////////////

#define IP_ADDR		(1<<4)
#define ARP_REQ		(1<<5)
#define ARP_RESP	(1<<6)

/////////////dhcp.c//////////////////////////
#define DHCP TX
#define DHCP_REQ	(1<<7)

////////////////leakybucket.c//////////////////
#define TOKEN_COUNTS (1<<8)
#define CHECK_TOKENS (1<<9)
#define TX_PACKETS	 (1<<10)
#define TIMER		 (1<<11)

///////////////////qos.c///////////////////////
#define QOS TX
#define QUEUE_INDEX (1<<12)
#define IPV4_DBG	(1<<13)
#define IPV6_DBG	(1<<14)
#define PRUNE_QUEUE	(1<<15)
#define SEND_QUEUE	(1<<16)


//--------------------INIT----------------------------
////////////////Miniport.c////////////////////
#define MP 1
#define DRV_ENTRY	(1<<0)
#define HARDWARE	(1<<1)
#define READ_REG	(1<<3)
#define DISPATCH	(1<<2)
#define CLAIM_ADAP	(1<<4)
#define REG_IO_PORT	(1<<5)
#define INIT_DISP	(1<<6)
#define RX_INIT		(1<<7)



//----------------RX----------------------------------
/////////////Receive.c///////////////////////
#define RX 7
#define RX_DPC		(1<<0)
#define FW_DNLD		(1<<1)
#define LINK_MSG	(1<<2)


//------------------OTHER ROUTINES------------------
// Isr,Halt,Reset,CheckForHang,PnP,Misc,CmHost
// total 12 macros
#define OTHERS 15
///////////ISR.C///////////////////////////

#define ISR OTHERS
#define MP_DPC		(1<<0)

/////////////HaltnReset.c//////////////////
#define HALT OTHERS
#define MP_HALT		(1<<1)
#define CHECK_HANG	(1<<2)
#define MP_RESET	(1<<3)
#define MP_SHUTDOWN	(1<<4)

///////////pnp.c////////////////////////////
#define PNP OTHERS
#define MP_PNP		(1<<5)

////////Misc.c////////////////////////////////
#define MISC OTHERS
#define DUMP_INFO	(1<<6)
#define CLASSIFY	(1<<7)
#define LINK_UP_MSG	(1<<8)
#define CP_CTRL_PKT	(1<<9)
#define DUMP_CONTROL (1<<10)



///////////cmhost.cpp////////////////////
#define CMHOST OTHERS
#define CONN_MSG    (1<<11)
#define SERIAL		(1<<12)
#define IDLE_MODE	(1<<13)

#define WRM			(1<<14)
#define RDM			(1<<15)

#if defined(__KERNEL__)

/******************************************************************************
 *  All the PCMCIA modules use PCMCIA_DEBUG to control debugging.  If
 *  you do not define PCMCIA_DEBUG at all, all the debug code will be
 *  left out.  If you compile with PCMCIA_DEBUG=0, the debug code will
 *  be present but disabled -- but it can then be enabled for specific
 *  modules at load time with a 'pc_debug=#' option to insmod.
 ******************************************************************************/
#define USB_DEBUG	0
#ifdef USB_DEBUG
//INT_MODULE_PARM(pc_debug, PCMCIA_DEBUG);
#define DEBUG(n, args...)	\
{	\
	printk("drv) %s:%d %s(): ", __FILE__, __LINE__, __FUNCTION__);	\
	printk(KERN_DEBUG args);	\
	printk("\n");	\
}
#else	/*  */
#define DEBUG(n, args...)	do {} while (0)
#endif	/*  */

/******************************************************************************
 *  assert with traditional printf/panic.
 *  -- from jfs_debug.h - KJG
 ******************************************************************************/
#ifdef CONFIG_KERNEL_ASSERTS
/* kgdb stuff */
#define ASSERT(p)	KERNEL_ASSERT(#p, p)
#else 	/* !CONFIG_KERNEL_ASSERTS */
#define ASSERT(p) {	\
	if (!(p)) {	\
		printk("*** ASSERT(%s)\n",#p);	\
		BUG();	\
	}	\
}
#endif 	/* CONFIG_KERNEL_ASSERTS */

#endif /* __KERNEL__ */

#endif	// _WIBROB_DEBUG_H

/* vim: set tabstop=4 shiftwidth=4: */

