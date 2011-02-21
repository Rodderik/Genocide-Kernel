#ifndef _WIBRO_HEADERS_H
#define _WIBRO_HEADERS_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/proc_fs.h>

#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>

#include <asm/byteorder.h>
#include <asm/uaccess.h>

#include "xp2linux.h"

//#ifndef WIBRO_SDIO_WMI
//#define WIBRO_SDIO_WMI
//#endif

#ifdef WIBRO_SDIO_WMI 
#include "te_ss_if.h"
#include "wibro802_16.h" 
#endif

#include "debug.h"
#include "buffer.h"
#include "wibro_sdio.h"
#include "ethernet.h"
#include "adapter.h"

extern u_int g_uiWInitDumpLevel;
extern u_int g_uiWInitDumpDetails;
extern u_int g_uiWTxDumpLevel;
extern u_int g_uiWTxDumpDetails;
extern u_int g_uiWRxDumpLevel;
extern u_int g_uiWRxDumpDetails; 
extern u_int g_uiWOtherDumpLevel;
extern u_int g_uiWOtherDumpDetails;

//#ifndef WIBRO_DEBUG_TEST
//#define WIBRO_DEBUG_TEST
//#endif

#define WAKEUP_BY_GPIO
//#define HARDWARE_USE_SUPPRESS_MAC_HEADER
// align headres
#define HARDWARE_USE_ALIGN_HEADER

extern VOID		GetDeviceFeatures(MINIPORT_ADAPTER *Adapter);
extern VOID 	SendPrivateCommand(MINIPORT_ADAPTER *Adapter, PVOID Buffer, USHORT Length);
extern VOID	SendPrivateExCommand(MINIPORT_ADAPTER *Adapter, PVOID Buffer, USHORT Length);
extern INT 	LoadWiMaxImage(VOID);
extern VOID 	UnLoadWiMaxImage(VOID);

extern INT hwStart(MINIPORT_ADAPTER *Adapter);
extern INT hwStop(MINIPORT_ADAPTER *Adapter);
extern INT hwInit(MINIPORT_ADAPTER *Adapter);
extern VOID hwRemove(MINIPORT_ADAPTER *Adapter);

extern unsigned int SendDataOut(MINIPORT_ADAPTER *Adapter, PBUFFER_DESCRIPTOR dsc);
void write_bulk_callback(struct urb *urb);
extern u_long hwProcessPrivateCmd(MINIPORT_ADAPTER *Adapter, void *Buffer);
extern BOOLEAN ethIsArpRequest(PUCHAR packet, ULONG length);
extern VOID ethSendArpResponse(MINIPORT_ADAPTER	  *Adapter, PUCHAR packet, ULONG length);
extern unsigned int hwSendData(MINIPORT_ADAPTER *Adapter, void *Buffer, ULONG Length);
unsigned int processUrbReceiveData(MINIPORT_ADAPTER *Adapter, void *Buffer, long Timeout);
extern struct sk_buff *pull_skb(MINIPORT_ADAPTER  *Adapter);
unsigned int processSdioReceiveData(MINIPORT_ADAPTER *Adapter, void *Buffer, u_long Length, long Timeout);
extern void fill_skb_pool(MINIPORT_ADAPTER *Adapter);

extern unsigned int hwSendControl(MINIPORT_ADAPTER *Adapter, void *Buffer, u_long Length);
extern void SendNextControlPacket(MINIPORT_ADAPTER *Adapter);
extern VOID controlReceive(MINIPORT_ADAPTER   *Adapter, void *Buffer, unsigned long Length);
unsigned int EnqueueControlPacket(MINIPORT_ADAPTER * Adapter, void * Buffer, u_long Length);
extern unsigned int	controlInit(MINIPORT_ADAPTER *Adapter);
extern VOID	controlRemove(MINIPORT_ADAPTER *Adapter);

void hwTransmitThread(struct work_struct *work);
extern PCONTROL_PROCESS_DESCRIPTOR controlFindProcessById(PMINIPORT_ADAPTER Adapter, UINT Id);
extern PCONTROL_PROCESS_DESCRIPTOR controlFindProcessByType(PMINIPORT_ADAPTER Adapter, USHORT Type);
extern VOID controlDeleteProcessById(PMINIPORT_ADAPTER Adapter, UINT Id);
extern u_long bufFindCount(LIST_ENTRY ListHead);
extern PBUFFER_DESCRIPTOR bufFindByType(LIST_ENTRY ListHead, USHORT Type);
extern VOID hwReturnPacket(PMINIPORT_ADAPTER Adapter, USHORT Type);

#ifdef DISCONNECT_TIMER
void  hwInitDisconnectTimer(MINIPORT_ADAPTER *Adapter);
void  hwStartDisconnectTimer(MINIPORT_ADAPTER *Adapter);
void  hwStopDisconnectTimer(MINIPORT_ADAPTER *Adapter);
void  hwDisconnectTimerHandler(unsigned long ptr);
#endif

#ifdef WIBRO_SDIO_WMI
int mgtInit(MINIPORT_ADAPTER *Adapter);
void mgtReceive(MINIPORT_ADAPTER *Adapter, PVOID Buffer, ULONG Length);
void mgtRemove(MINIPORT_ADAPTER *Adapter);
#endif

#ifndef WIBRO_DEBUG_TEST
#define ENTER 
#define LEAVE 
#else
#define ENTER printk("<1>\x1b[1;33m--> %s() line %d\x1b[0m\n", __FUNCTION__, __LINE__);
#define LEAVE printk("<1>\x1b[1;33m<-- %s() line %d\x1b[0m\n", __FUNCTION__, __LINE__);
#endif

#define DumpDebug(a, args...) {	\
						{		\
							printk("<1>\x1b[1;33m[WiMAX] ");printk(args);printk("\x1b[0m\n");	\
						}}

#endif

