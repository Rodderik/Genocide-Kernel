#include "headers.h"

#include <linux/delay.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#include "wimax_plat.h" //cky 20100427
#include <linux/wakelock.h>


#if defined (CONFIG_MACH_QUATTRO)
#include <mach/instinctq.h>
#elif defined (CONFIG_MACH_S5PC110_ARIES)
//#include <mach/victory/gpio-jupiter.h>	// Yongha for Victory WiMAX 20100208
#if defined CONFIG_S5PV210_VICTORY
#include <mach/victory/gpio-aries.h>	// Yongha for Victory WiMAX 20100208
#elif defined CONFIG_S5PV210_ATLAS
#include <mach/atlas/gpio-aries.h>	// Yongha for Victory WiMAX 20100208
#endif
#endif

#if defined (CONFIG_MACH_QUATTRO)
#include <linux/i2c/pmic.h>
#endif


/*
 * Version Information
 */

#define	WIBRO_USB_DRIVER_VERSION_STRING "1.0.1"	
#define DRIVER_AUTHOR "samsung.com"
#define DRIVER_DESC "Samsung WiBro SDIO Device Driver"

static const char driver_name[] = "C730SDIO";
MINIPORT_ADAPTER * Adapter_global = NULL;
static struct proc_dir_entry *g_proc_swibro_dir = NULL;

static u8 node_id[ETH_ALEN];

int g_temp_tgid = 0;	//cky 20100510
struct wake_lock wimax_wake_lock;	//s.Kiran 20100514
struct wake_lock wimax_rxtx_lock;		//cky 20100624

#define VERSION "0.1"
#define MOD_PARAM_PATHLEN	2048
char firmware_path[MOD_PARAM_PATHLEN];
module_param_string(firmware_path, firmware_path, MOD_PARAM_PATHLEN, 0);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

//cky 20100217 suspend/resume
void wimax_suspend(void);
void wimax_resume(void);
extern void set_wimax_pm(void(*suspend)(void), void(*resume)(void));
extern void unset_wimax_pm(void);

extern int isDumpEnabled(void);	//cky 20100428
extern int max8893_ldo_enable_direct(int ldo);	//cky 20100610
extern int max8893_ldo_disable_direct(int ldo);	//cky 20100611

extern int s3c_bat_use_wimax(int onoff);	//cky 20100624

#define WIBRO_DEBUG_TEST

#ifndef WIBRO_DEBUG_TEST
u_int g_uiWInitDumpLevel = 0;//DRV_ENTRY|HARDWARE|READ_REG|DISPATCH;
u_int g_uiWInitDumpDetails = 0;//DRV_ENTRY|HARDWARE|READ_REG;
u_int g_uiWTxDumpLevel= 0; //TX_CONTROL;// |TIMER|MP_SEND|TX_CONTROL;
u_int g_uiWTxDumpDetails = 0; //TX_FIFO;// |TIMER|MP_SEND|TX_CONTROL;
u_int g_uiWRxDumpLevel = 0; //RX_DPC;
u_int g_uiWRxDumpDetails = 0; //RX_DPC;
u_int g_uiWOtherDumpLevel= 0;/*CHECK_HANG|DUMP_INFO|MP_HALT|CP_CTRL_PKT|DUMP_CONTROL|CONN_MSG|LINK_UP_MSG;*/
u_int g_uiWOtherDumpDetails = 0;/*CHECK_HANG|DUMP_INFO|CP_CTRL_PKT|DUMP_CONTROL|CONN_MSG|LINK_UP_MSG;*/
#else
u_int g_uiWInitDumpLevel = DRV_ENTRY|READ_REG|DISPATCH|HARDWARE;
u_int g_uiWInitDumpDetails = DRV_ENTRY|READ_REG |HARDWARE;
u_int g_uiWTxDumpLevel= TX_CONTROL|TIMER|MP_SEND;//|NEXT_SEND;
u_int g_uiWTxDumpDetails = TX_FIFO|TX_PACKETS |TIMER|MP_SEND|TX_CONTROL;
u_int g_uiWRxDumpLevel = RX_DPC;// | FW_DNLD;
u_int g_uiWRxDumpDetails = RX_DPC;// | FW_DNLD;
u_int g_uiWOtherDumpLevel= CHECK_HANG|DUMP_INFO|MP_HALT|CP_CTRL_PKT|DUMP_CONTROL|CONN_MSG|LINK_UP_MSG;
u_int g_uiWOtherDumpDetails = CHECK_HANG|DUMP_INFO|CP_CTRL_PKT|DUMP_CONTROL|CONN_MSG|LINK_UP_MSG;
#endif

/* use ethtool to change the level for any given device */
static int msg_level = -1;
module_param (msg_level, int, 0);

MODULE_PARM_DESC (msg_level, "Override default message level");
MODULE_DEVICE_TABLE(sdio, Adapter_table);

#define UWBRBDEVMAJOR	233

extern int gpio_wimax_poweron(void);
extern int gpio_wimax_poweroff(void);

extern unsigned long WiBro_SdioInitializeSend(MINIPORT_ADAPTER *Adapter);
void	charCreateNewName(PUCHAR, ULONG);

struct sk_buff *pull_skb(MINIPORT_ADAPTER  *Adapter)
{
	int i;
	struct sk_buff *skb;

	for (i = 0; i < RX_SKBS; i++) {
		if (likely(Adapter->rx_pool[i] != NULL)) {
			skb = Adapter->rx_pool[i];
			Adapter->rx_pool[i] = NULL;
			return skb;
			
		}
	}
	return NULL;
}

static void free_skb_pool(MINIPORT_ADAPTER  *Adapter)
{
	int nSkbs;
	ENTER;
	for (nSkbs = 0; nSkbs < RX_SKBS; nSkbs++) {
		if (Adapter->rx_pool[nSkbs]) {
			dev_kfree_skb(Adapter->rx_pool[nSkbs]);
			Adapter->rx_pool[nSkbs] = NULL;
		}
	}
	LEAVE;
}

void fill_skb_pool(MINIPORT_ADAPTER *Adapter)
{
	int nSkbs;

	for (nSkbs = 0; nSkbs < RX_SKBS; nSkbs++) {
		if (Adapter->rx_pool[nSkbs])
			continue;
		Adapter->rx_pool[nSkbs] = dev_alloc_skb(BUFFER_DATA_SIZE + 2);
		/*
		 ** we give up if the allocation fail. the tasklet will be
		 ** rescheduled again anyway...
		 */
		if (Adapter->rx_pool[nSkbs] == NULL)
			return;
		Adapter->rx_pool[nSkbs]->dev = Adapter->net;
		skb_reserve(Adapter->rx_pool[nSkbs], 2);
	}
}

static int WiBroB_proc_reset(char *buf, char **start, off_t offset,
		                        int count, int *eof, void *data)
{
	ENTER;
	LEAVE;
	return 0;
}

static int WiBroB_proc_dma_reset(char *buf, char **start, off_t offset,
		                        int count, int *eof, void *data)
{
	ENTER;
	LEAVE;
	return 0;
}

static int WiBroB_proc_link(char *buf, char **start, off_t offset,
		                        int count, int *eof, void *data)
{
	ENTER;
	LEAVE;
	return 0;
}

static int WiBroB_proc_status(char *buf, char **start, off_t offset,
		                        int count, int *eof, void *data)
{
	int len;
	MINIPORT_ADAPTER *Adapter = (MINIPORT_ADAPTER *)data;
	
	ENTER;
	len = sprintf(buf, "%d\n",Adapter->WibroStatus);
	if(len > 0) *eof = 1;
	LEAVE;
	return len;
}

static int WiBroB_proc_idle(char *buf, char **start, off_t offset,
		                        int count, int *eof, void *data)
{
	ENTER;
	LEAVE;
	return 0;
}

static int WiBroB_proc_desc(char *buf, char **start, off_t offset,
		                        int count, int *eof, void *data)
{
	ENTER;
	LEAVE;
	return 0;
}

static int WiBroB_proc_rssi(char *buf, char **start, off_t offset,
		                        int count, int *eof, void *data)
{
	ENTER;
	LEAVE;
	return 0;
}

static int WiBroB_proc_rssi_level(char *buf, char **start, off_t offset,
		                        int count, int *eof, void *data)
{
	ENTER;
	LEAVE;
	return 0;
}

static int WiBroB_proc_cinr(char *buf, char **start, off_t offset,
		                        int count, int *eof, void *data)
{
	ENTER;
	LEAVE;
	return 0;
}

static int WiBroB_proc_txpwr(char *buf, char **start, off_t offset,
		                        int count, int *eof, void *data)
{
	ENTER;
	LEAVE;
	return 0;
}

static int WiBroB_proc_skbuff(char *buf, char **start, off_t offset,
		                        int count, int *eof, void *data)
{
	ENTER;
	LEAVE;
	return 0;
}

static int WiBroB_proc_driver_version(char *buf, char **start, off_t offset,
		                        int count, int *eof, void *data)
{
	int len;
	len = sprintf(buf, "%s\n",WIBRO_USB_DRIVER_VERSION_STRING);
	if(len > 0) *eof = 1;
	return len;
}

void addProcEntry(PMINIPORT_ADAPTER Adapter)
{
	ENTER;
	/* create procfs directory & entry */
	g_proc_swibro_dir = proc_mkdir("swibro", NULL);
	create_proc_read_entry("reset", 0644, g_proc_swibro_dir,
			WiBroB_proc_reset, (void *)Adapter);
	create_proc_read_entry("dma_reset", 0644, g_proc_swibro_dir,
			WiBroB_proc_dma_reset, (void *)Adapter);
	create_proc_read_entry("link", 0644, g_proc_swibro_dir,
			WiBroB_proc_link, (void *)Adapter);
	create_proc_read_entry("status", 0644, g_proc_swibro_dir,
			WiBroB_proc_status, (void *)Adapter);
	create_proc_read_entry("idle", 0644, g_proc_swibro_dir,
			WiBroB_proc_idle, (void *)Adapter);
	create_proc_read_entry("desc", 0644, g_proc_swibro_dir,
			WiBroB_proc_desc, (void *)Adapter);
	create_proc_read_entry("rssi", 0644, g_proc_swibro_dir,
			WiBroB_proc_rssi, (void *)Adapter);
	create_proc_read_entry("rssi_level", 0644, g_proc_swibro_dir,
			WiBroB_proc_rssi_level, (void *)Adapter);
	create_proc_read_entry("cinr", 0644, g_proc_swibro_dir,
			WiBroB_proc_cinr, (void *)Adapter);
	create_proc_read_entry("txpwr", 0644, g_proc_swibro_dir,
			WiBroB_proc_txpwr, (void *)Adapter);
	create_proc_read_entry("skbuff", 0644, g_proc_swibro_dir,
			WiBroB_proc_skbuff, (void *)Adapter);
	create_proc_read_entry("driver_version", 0644, g_proc_swibro_dir,
			WiBroB_proc_driver_version, (void *)Adapter);
	LEAVE;
}

void removeProcEntry(void)
{
	ENTER;
	remove_proc_entry("reset", g_proc_swibro_dir);
	remove_proc_entry("dma_reset", g_proc_swibro_dir);
	remove_proc_entry("link", g_proc_swibro_dir);
	remove_proc_entry("status", g_proc_swibro_dir);
	remove_proc_entry("idle", g_proc_swibro_dir);
	remove_proc_entry("desc", g_proc_swibro_dir);
	remove_proc_entry("rssi", g_proc_swibro_dir);
	remove_proc_entry("rssi_level", g_proc_swibro_dir);
	remove_proc_entry("cinr", g_proc_swibro_dir);
	remove_proc_entry("txpwr", g_proc_swibro_dir);
	remove_proc_entry("skbuff", g_proc_swibro_dir);
	remove_proc_entry("driver_version",g_proc_swibro_dir);
//	remove_proc_entry("swibro", &proc_root);	// sangam : wrong param crash
	remove_proc_entry("swibro", NULL);
	LEAVE;
	return;
}

void mpFreeResources(PMINIPORT_ADAPTER Adapter)
{
	ENTER;

	free_skb_pool(Adapter);
	if (Adapter->rx_skb)
		dev_kfree_skb(Adapter->rx_skb);

	// remove control process list
	controlRemove(Adapter);
	
	//remove hardware interface
	hwRemove(Adapter);	
	
	// move to deinit UnLoadWiMaxImage();
	removeProcEntry();

	LEAVE;
	return;
}

int Adapter_sdio_rx_packet(MINIPORT_ADAPTER *Adapter)
{
	struct net_device *net;
	int err = 1;
	unsigned int len=0;
	unsigned int remained_len=0;
		
//	ENTER;
	if (unlikely(!Adapter))
		return err;

	net = Adapter->net;

	if (unlikely(!netif_device_present(net))){
		DumpDebug(READ_REG, "!device_present!");
		return err;
	}
/*	if(unlikely(Adapter->bHaltPending)){
		DumpDebug(READ_REG, "read_bulk_callback Halt Pending --");
		return err;
	}
*/	//sumanth: the above check is already done in Adapter_interrupt. no need to do it again here.
//	memset(Adapter->hw.ReceiveTempBuffer, 0x0, SDIO_BUFFER_SIZE); 
	
//	if(Adapter->hw.ReceiveRequested)
//	{
		hwSdioReadCounter(Adapter, &len, &err); /* read received byte */
		if(unlikely(err !=0 || !len) )
		{
			DumpDebug(READ_REG, "!hwSdioReadCounter in Adapter_sdio_rx_packet!");
			Adapter->bHaltPending = TRUE;
			return err;
		}
		if(unlikely(len>SDIO_BUFFER_SIZE))	
			{
				DumpDebug(READ_REG, "ERROR RECV length (%d) > SDIO_BUFFER_SIZE", len);
				len = SDIO_BUFFER_SIZE;
			}

			u_char cnt=0;
			err = sdio_memcpy_fromio(Adapter->func, Adapter->hw.ReceiveTempBuffer+8, SDIO_DATA_PORT_REG ,len); //sumanth: leave some space to copy the ethernet header
			if(unlikely(err !=0 || !len) )
			{
				DumpDebug(READ_REG, "!Can't Read Data in Adapter_sdio_rx_packet!(err=%d at cnt=%d)",err,cnt);
				Adapter->bHaltPending = TRUE;
				return err;
			}
		
#if 0
		if(!Adapter->DownloadMode)
		{
			// dump packets
			UINT i, l = len;
			u_char *  b = (u_char *)Adapter->hw.ReceiveTempBuffer;
			DumpDebug(READ_REG, "Received packet [%d] = ", l);
			for (i = 0; i < l; i++) {
				DumpDebug(READ_REG, "%02x", b[i]);
				//if (i != (l - 1)) DumpDebug(READ_REG, ",");
				//if ((i != 0) && ((i%32) == 0)) DumpDebug(READ_REG, "");
			}
			//DumpDebug(READ_REG, "");
		}
#endif
//		memset(Adapter->hw.ReceiveBuffer, 0x0, SDIO_BUFFER_SIZE); 
		remained_len = processSdioReceiveData(Adapter, Adapter->hw.ReceiveTempBuffer+8, len, 0);	//sumanth: leave some space to copy the ethernet header
		if(unlikely(remained_len != 0)) DumpDebug(READ_REG, "Should we process for multi packet, Remained Length is = %d",remained_len);
//	}
//    LEAVE;
	return 0;
}

static struct net_device_stats *Adapter_netdev_stats(struct net_device *dev)
{
	ENTER;
	return &((MINIPORT_ADAPTER *) netdev_priv(dev))->netstats; 
}

static void Adapter_set_multicast(struct net_device *net)
{
	return;
}

static int Adapter_start_xmit(struct sk_buff *skb, struct net_device *net)
{
	MINIPORT_ADAPTER *Adapter = netdev_priv(net);
	int len = ((skb->len) & 0x3f) ? skb->len : skb->len + 1;
	ENTER;
	netif_stop_queue(net); 

	if(!Adapter->MediaState || Adapter->bHaltPending)
	{
		DumpDebug(DISPATCH,("Driver already halted. Returning Failure..."));
		DumpDebug(DISPATCH,("Driver Not Connected..."));
		dev_kfree_skb(skb);
		Adapter->netstats.tx_dropped++;
		net->trans_start = jiffies;
		Adapter->XmitErr += 1;
		return 0;
	}
#if 0	
	DumpDebug(READ_REG, "******************DATA PACKET Adapter_start_xmit************************");
	{
	    // dump packets
	    UINT i;
	    PUCHAR  b = (PUCHAR)skb->data;
	    printk("Adapter_start_xmit = ");
	    for (i = 0; i < skb->len; i++) {
		printk("%02x", b[i]);
		//if (i != (skb->len - 1)) printk(",");
		//if ((i != 0) && ((i%32) == 0)) printk("");
	    }
	    //printk("");
	}
#endif	
	hwSendData(Adapter, skb->data, len);
	dev_kfree_skb(skb);

	if(Adapter->MediaState)
		netif_wake_queue(Adapter->net);

	LEAVE;
	return 0;
}

static int
netdev_ethtool_ioctl(struct net_device *dev, void *useraddr)
{
	u32	ethcmd;
	ENTER;
	if (copy_from_user(&ethcmd, useraddr, sizeof(ethcmd)))
		return -EFAULT;

	switch (ethcmd)
	{		
		case ETHTOOL_GDRVINFO:
		{
			struct ethtool_drvinfo info = { ETHTOOL_GDRVINFO };
			strncpy(info.driver, "C730USB", sizeof(info.driver) - 1);
			if (copy_to_user(useraddr, &info, sizeof(info)))
				return -EFAULT;
			
			return 0;
		}
	}
	LEAVE;
	return -EOPNOTSUPP;
}

static int Adapter_ioctl(struct net_device *net, struct ifreq *rq, int cmd)
{
	MINIPORT_ADAPTER *Adapter = netdev_priv(net);
    
	ENTER;
	if(Adapter->bHaltPending) {
		DumpDebug(DISPATCH,("Driver already halted. Returning Failure..."));
		return STATUS_UNSUCCESSFUL;
	}
	
	switch (cmd)
	{
		case SIOCETHTOOL:
			return netdev_ethtool_ioctl(net, (void *)rq->ifr_data);

		default:
			return -EOPNOTSUPP;
	}
        LEAVE;
	return 0;
}

static void Adapter_tx_timeout(struct net_device *net)
{
	MINIPORT_ADAPTER *Adapter = netdev_priv(net);
	if (netif_msg_timer(Adapter))
		printk(KERN_WARNING "%s: tx timeout", net->name);
	Adapter->netstats.tx_errors++;
}

static void  Adapter_reset(MINIPORT_ADAPTER *Adapter)
{
	ENTER;
	Adapter->stats.tx.Bytes = 0;
	Adapter->stats.rx.Bytes = 0;
	LEAVE;
	return ;
}

static int Adapter_init(struct net_device *net)
{
	DumpDebug(DRV_ENTRY, "Adapter driver init success!!!!!!!");
	
	wake_lock_init(&wimax_wake_lock, WAKE_LOCK_SUSPEND, "wibro_wimax_sdio");
	wake_lock_init(&wimax_rxtx_lock, WAKE_LOCK_SUSPEND, "wibro_wimax_rxtx");
	
	//local_info_t * local = dev->priv;
	int	status = 0;
	return status;
}

static void Adapter_uninit(struct net_device *net)
{
	DumpDebug(DRV_ENTRY, "Adapter driver uninit success!!!!!!!");
	wake_lock_destroy(&wimax_wake_lock);
	wake_lock_destroy(&wimax_rxtx_lock);

	return;
}

static int Adapter_close(struct net_device *net)
{
	ENTER;
	DumpDebug(DRV_ENTRY, "Adapter driver close success!!!!!!!");

	s3c_bat_use_wimax(0);	//cky 20100624
	
	netif_stop_queue(net);
//	WiBro_USBFreePendingList(Adapter);	
	LEAVE;
	return 0;
}

static int Adapter_open(struct net_device *net)
{
	MINIPORT_ADAPTER *Adapter;
	int res = 0;
	ENTER;

	Adapter = netdev_priv(net);
	
	if( Adapter == NULL || Adapter->bHaltPending )
	{
		DumpDebug(DRV_ENTRY, "can't find adapter OR bHaltPending ");
		return -ENODEV; 
	}
#if 0	//cky 20100323	no need to allocate rx_skb buffer here (sangam) it should be allocated in rx routine
	if (Adapter->rx_skb == NULL)
		Adapter->rx_skb = pull_skb(Adapter);
	
	if (!Adapter->rx_skb) {
		DumpDebug(DRV_ENTRY, "Adapter->rx_skb null");
		return -ENOMEM;
	}
#endif

	Adapter_reset(Adapter);
	
	if(Adapter->MediaState)
		netif_wake_queue(net);
	//	netif_start_queue(net);
	else
		netif_stop_queue(net);
	
#ifdef WIBRO_SDIO_WMI
	Adapter->Mgt.Enabled = TRUE;
	if(Adapter->Mgt.Enabled)
		if(mgtInit(Adapter))
			mgtRemove(Adapter);
#endif

//	netif_carrier_on(net); //replacing set_carrier
//	netif_start_queue(net); //sangam change
	if (netif_msg_ifup(Adapter))
		DumpDebug(DRV_ENTRY, "netif msg if up");
	res = 0;

	s3c_bat_use_wimax(1);	//cky 20100624
	
	DumpDebug(DRV_ENTRY, "Adapter driver open success!!!!!!!");

	LEAVE;
	return res;
}

static int
uwbrdev_open (struct inode * inode, struct file * file)
{
	PCONTROL_PROCESS_DESCRIPTOR	process;	
	PMINIPORT_ADAPTER	Adapter;
	unsigned long	flags;

	ENTER;
	if((Adapter_global == NULL) || Adapter_global->bHaltPending )
	{
		DumpDebug(DRV_ENTRY, "can't find adapter or Device Removed");
		return -ENODEV; 
	}
	file->private_data = (void *)Adapter_global;
	Adapter = (PMINIPORT_ADAPTER)(file->private_data);
	DumpDebug(DRV_ENTRY, "open: tgid=%d, pid=%d",current->tgid, current->pid);
	
	if (Adapter->InitDone != TRUE || Adapter->bHaltPending )
	{
		DumpDebug(DRV_ENTRY, "Device not ready Retry..");
		LEAVE;
		return -ENXIO; 
	}
	
	process = controlFindProcessById(Adapter, current->tgid);
	if (process != NULL) {
		DumpDebug(DRV_ENTRY, "second open attemp from uid %d", current->tgid);
		return -EEXIST; 
	}
	else {	// init new process descriptor
		if((process = (PCONTROL_PROCESS_DESCRIPTOR)kmalloc(sizeof(CONTROL_PROCESS_DESCRIPTOR), GFP_KERNEL)) == NULL)
			return -ENOMEM;
		else {
			process->Id = current->tgid;
			process->Irp = FALSE; // no read thread
			process->Type = 0;
			init_waitqueue_head(&process->read_wait);
			spin_lock_irqsave(&Adapter->ctl.Apps.Lock, flags);
		 	QueuePutTail(Adapter->ctl.Apps.ProcessList, process->Node);
			spin_unlock_irqrestore(&Adapter->ctl.Apps.Lock, flags);
		}
	}
	LEAVE;
	return 0;
}

static int
uwbrdev_release (struct inode * inode, struct file * file)
{
	PCONTROL_PROCESS_DESCRIPTOR process;
	PMINIPORT_ADAPTER	Adapter;
	unsigned long flags;
	int current_tgid = 0;
	
	ENTER;
	DumpDebug(DRV_ENTRY, "release: tgid=%d, pid=%d",current->tgid, current->pid);
	Adapter = (PMINIPORT_ADAPTER)(file->private_data);
	if(Adapter == NULL)	// don't check for the haltpending
	{
		DumpDebug(DRV_ENTRY, "can't find adapter");
		return -ENODEV; 
	}

	//cky 20100510 process id 5
/*
	if (current->tgid == 5)
	{
		// process killed before read closed
		current_tgid = g_temp_tgid;
		DumpDebug(DRV_ENTRY, "release: pid change (%d -> %d)", current->tgid, current_tgid);
	}
	else
	{
		current_tgid = current->tgid;
	}
*/
	        //tsh.park 20100930
        current_tgid = current->tgid;	
	process = controlFindProcessById(Adapter, current_tgid);
	// If the opened process is not identical with the closing process
	if(process == NULL)
	{
		current_tgid = g_temp_tgid;
		DumpDebug(DRV_ENTRY, "release: pid changed: %d", current_tgid);
		process = controlFindProcessById(Adapter, current_tgid);
	}
	if(process != NULL) {
		// RELEASE READ THREAD
		if(process->Irp)
		{
			process->Irp = FALSE;
			wake_up_interruptible(&process->read_wait);
		}
		spin_lock_irqsave(&Adapter->ctl.Apps.Lock, flags);
		controlDeleteProcessById(Adapter, current_tgid);
		spin_unlock_irqrestore(&Adapter->ctl.Apps.Lock, flags);
	}
	else { //not found
		DumpDebug(DISPATCH, "process %d not found", current_tgid);
		return -ESRCH;
	}

	LEAVE;
	return 0;
}

static int
uwbrdev_ioctl (struct inode * inode, struct file * file, u_int cmd, u_long arg)
{
	int ret = 0;
	MINIPORT_ADAPTER *Adapter;
	PCONTROL_PROCESS_DESCRIPTOR process;
	unsigned long	flags;

	ENTER;
	Adapter = (PMINIPORT_ADAPTER)(file->private_data);
	
	if((Adapter == NULL) || Adapter->bHaltPending )
	{
		DumpDebug(DRV_ENTRY, "can't find adapter or Device Removed");
		return -ENODEV; 
	}
	//DumpDebug(DISPATCH, "CMD: %x",cmd);

	switch(cmd)
	{
		case CONTROL_IOCTL_WRITE_REQUEST :
		{
			SAMSUNG_ETHER_HDR hdr;
			PCONTROL_ETHERNET_HEADER ctlhdr; 	
		
			//DumpDebug(DISPATCH, "ioctl: write: process tgid=%d, pid=%d",current->tgid, current->pid);
			
			if((char *)arg == NULL)
				return -EFAULT;
	 		hdr.length = ((SAMSUNG_ETHER_HDR *)arg)->length;
			
			if(hdr.length < MINIPORT_MAX_TOTAL_SIZE) {
				if(copy_from_user(hdr.bytePacket, (void *)(arg+sizeof(int)), hdr.length))
					return -EFAULT;
			}
			else
				return -EFBIG;

			spin_lock_irqsave(&Adapter->ctl.Apps.Lock, flags);
			process = controlFindProcessById(Adapter, current->tgid);
		
			if(process == NULL) { // process not found
				DumpDebug(DISPATCH, "process %d not found", current->tgid);
				ret = -EFAULT;
				spin_unlock_irqrestore(&Adapter->ctl.Apps.Lock, flags);
				break;
			}
			ctlhdr = (PCONTROL_ETHERNET_HEADER)hdr.bytePacket;	
			process->Type = ctlhdr->Type;

			hwSendControl(Adapter, hdr.bytePacket, hdr.length);	
			Adapter->stats.tx.Control++;
			spin_unlock_irqrestore(&Adapter->ctl.Apps.Lock, flags);
			break;
		}
#ifdef	CONTROL_USE_WCM_LINK_INDICATION 
		case CONTROL_IOCTL_LINK_UP :
		{
			DumpDebug(DISPATCH, ("ioctl CONTROL_IOCTL_LINK_UP."));
			if (Adapter->MediaState != MEDIA_CONNECTED)
			{
				Adapter->MediaState = MEDIA_CONNECTED; 
				netif_start_queue(Adapter->net);
			}
			break;
		}
		case CONTROL_IOCTL_LINK_DOWN :
		{
			int		flags;
			DumpDebug(DISPATCH, ("ioctl CONTROL_IOCTL_LINK_DOWN."));
			if (Adapter->MediaState != MEDIA_DISCONNECTED)
			{
				Adapter->MediaState = MEDIA_DISCONNECTED; 
				netif_stop_queue(Adapter->net);
				Adapter_reset(Adapter);
			}
			break;
		}
#endif		
		case CONTROL_IOCTL_GET_DRIVER_VERSION : //sangam : check the format of the request**
		{
			SAMSUNG_ETHER_HDR driVersion;
			DumpDebug(DISPATCH, ("ioctl CONTROL_IOCTL_GET_DRIVER_VERSION."));
			memset(&driVersion, 0, sizeof (SAMSUNG_ETHER_HDR));	
			if ( (((SAMSUNG_ETHER_HDR *)arg)->length < sizeof(SAMSUNG_ETHER_HDR))
					|| (((SAMSUNG_ETHER_HDR *)arg)->bytePacket ==NULL) )
			{
				DumpDebug(DISPATCH, ("Buffer Too small or Buff Memory not allocated...."));
				return -EFAULT;
			}
			strncpy(driVersion.bytePacket, WIBRO_USB_DRIVER_VERSION_STRING, sizeof(WIBRO_USB_DRIVER_VERSION_STRING));	
			driVersion.length = sizeof(WIBRO_USB_DRIVER_VERSION_STRING);
			
			if (copy_to_user((void *)arg, (void *)&driVersion, sizeof(SAMSUNG_ETHER_HDR)))
				return -EFAULT;
			DumpDebug(DISPATCH, ("Driver version sent to application."));
			break;
		}
		case CONTROL_IOCTL_GET_NETWORK_STATISTICS : // sangam: check the format of the request
		{
			CONTROL_NETWORK_STATISTICS stats;

			DumpDebug(DISPATCH, ("ioctl CONTROL_IOCTL_GET_NETWORK_STATISTICS."));
			memset(&stats, 0, sizeof (CONTROL_NETWORK_STATISTICS));	
			if ( (((SAMSUNG_ETHER_HDR *)arg)->length < sizeof(CONTROL_NETWORK_STATISTICS))
					|| (((SAMSUNG_ETHER_HDR *)arg)->bytePacket ==NULL) ) 
			{
				DumpDebug(DISPATCH, ("Buffer Too small or Buff Memory not allocated...."));
				return -EFAULT;
			}
			stats.XmitOkBytes = Adapter->stats.tx.Bytes;
			stats.RcvOkBytes = Adapter->stats.rx.Bytes;

			if (copy_to_user( (void *)(((SAMSUNG_ETHER_HDR *)arg)->bytePacket), (void *)&stats, sizeof(CONTROL_NETWORK_STATISTICS)))
				return -EFAULT;
			DumpDebug(DISPATCH, ("Driver version sent to application."));
			break;
		}
#ifdef DISCONNECT_TIMER		
		case CONTROL_IOCTL_SET_DISCONNECT_TIMEOUT:
		{
			DumpDebug(DISPATCH, ("ioctl CONTROL_IOCTL_SET_DISCONNECT_TIMEOUT."));
			if ( (((SAMSUNG_ETHER_HDR *)arg)->length < sizeof(unsigned int))
					|| (((SAMSUNG_ETHER_HDR *)arg)->bytePacket ==NULL) )
			{
				DumpDebug(DISPATCH, ("Buffer Too small or Buff Memory not allocated...."));
				return -EFAULT;
			}	// argument data is in millisecond timeout period
			Adapter->DisconnectTimer.expires = jiffies + ( HZ * 100 / (*(unsigned int *)(((SAMSUNG_ETHER_HDR *)arg)->bytePacket)) );
			break;
		}
#endif		
		case CONTROL_IOCTL_GET_DEVICE_INFO:
		{
			DumpDebug(DISPATCH, ("ioctl CONTROL_IOCTL_GET_DEVICE_INFO."));
			if ( (((SAMSUNG_ETHER_HDR *)arg)->length < sizeof(SAMSUNG_ETHER_HDR))
					|| (((SAMSUNG_ETHER_HDR *)arg)->bytePacket ==NULL) )
			{
				DumpDebug(DISPATCH, ("Buffer Too small or Buff Memory not allocated...."));
				return -EFAULT;
			}
			break;
		}
#ifdef WIBRO_USB_SELECTIVE_SUSPEND		
		case SAMSUNG_WAKEUP_REQ :
		{
			DumpDebug(DISPATCH, ("ioctl SAMSUNG_WAKEUP_REQ."));
			break;
		}
#endif		
	}
	
	LEAVE;
	return ret;
}

static ssize_t
uwbrdev_read (struct file * file, char * buf, size_t count, loff_t *ppos)
{
	PCONTROL_PROCESS_DESCRIPTOR process;
	PBUFFER_DESCRIPTOR dsc;
	PMINIPORT_ADAPTER	Adapter;
	int rlen = 0;
	u_long ReadLength, flags;
//    UCHAR stopMessage[4] = {0x1f, 0x2f,0x3f,0x4f};

	ENTER;
	Adapter = (PMINIPORT_ADAPTER)(file->private_data);
	if((Adapter == NULL) || Adapter->bHaltPending )
	{
		// sangam : ******* check how to indicate appln through  mesg or error status??
		DumpDebug(DRV_ENTRY, "can't find adapter or Device Removed");
		return -ENODEV; 
	}
	ReadLength = count;
	if (buf == NULL) {
		DumpDebug(DISPATCH, "BUFFER is NULL");
		return -EFAULT; // bad address
	}
	//DumpDebug(DRV_ENTRY, "uwbrdev_read: process tgid=%d, pid=%d",current->tgid, current->pid);

	process = controlFindProcessById(Adapter, current->tgid);
	if(process == NULL) {
		DumpDebug(DRV_ENTRY, "uwbrdev_read: process %d not exist",current->tgid);
		return -ESRCH;
	}

	if(process->Irp == FALSE) {
		dsc = bufFindByType(Adapter->ctl.Q_Received.Head, process->Type);
		if(dsc == NULL) {
			//DumpDebug(DISPATCH, ("uwbrdev_read: Wait......"));
			process->Irp = TRUE;	
			if(wait_event_interruptible( process->read_wait,
					((process->Irp == FALSE ) || (Adapter->bHaltPending == TRUE))) ) {
				//DumpDebug(DISPATCH, "uwbrdev_read: sangam dbg irp = %d, halt = %d", process->Irp, Adapter->bHaltPending);
				//DumpDebug(DISPATCH, ("uwbrdev_read: Application exit Indicated to Appln..."));
				process->Irp = FALSE;
				g_temp_tgid = current->tgid;
				return -ERESTARTSYS;	// sangam : ALSO process exit in case of close call
			}
			if (Adapter->bHaltPending == TRUE) {
				DumpDebug(DISPATCH, ("uwbrdev_read: Card Removed Indicated to Appln..."));
				/*********** SANGAM : check the card remove indication method ****************/
				/*		if(copy_to_user(buf, stopMessage, sizeof(stopMessage)))
						return -EFAULT;
						rlen = sizeof(stopMessage);
						return rlen;
				*/
				process->Irp = FALSE;
				g_temp_tgid = current->tgid;
				return -ENODEV;
			}
		}
	
		if (ReadLength == 1500) {	// our application changed from 2050
			spin_lock_irqsave(&Adapter->ctl.Apps.Lock, flags);
			dsc = bufFindByType(Adapter->ctl.Q_Received.Head, process->Type);
			//DumpDebug(DISPATCH, "uwbrdev_read: sangam dbg addr = %lu ", (unsigned long)dsc);
			if (!dsc) {
				DumpDebug(DISPATCH, "uwbrdev_read: Fail...node is null");
				spin_unlock_irqrestore(&Adapter->ctl.Apps.Lock, flags);
				return -1;
			}
			spin_unlock_irqrestore(&Adapter->ctl.Apps.Lock, flags);
			
			if(copy_to_user(buf, dsc->Buffer, dsc->data.Length)) {
				DumpDebug(DISPATCH, "uwbrdev_read: sangam dbg : copy_to_user failed len=%lu !!!!!!!!!", dsc->data.Length);
				return -EFAULT;
			}
			//DumpDebug(DISPATCH, "uwbrdev_read: sangam dbg : copy_to_user len = %lu ", dsc->data.Length);
		
			//DumpDebug(DISPATCH, ("uwbrdev_read: Sent the control pkt to Apln......"));
			spin_lock_irqsave(&Adapter->ctl.Apps.Lock, flags);
			rlen = dsc->data.Length;
			hwReturnPacket(Adapter, dsc->data.Type);
			spin_unlock_irqrestore(&Adapter->ctl.Apps.Lock, flags);
		}
	}
	else {
		DumpDebug(DISPATCH, "uwbrdev_read: Read was sent twice by process %d", current->tgid);
		return -EEXIST;
	}

	LEAVE;
	return rlen;
}

static ssize_t
uwbrdev_write (struct file * file, const char * buf, size_t count, loff_t *ppos)
{
	PCONTROL_PROCESS_DESCRIPTOR process;
	PMINIPORT_ADAPTER	Adapter;
	
	ENTER;
	Adapter = (PMINIPORT_ADAPTER)(file->private_data);
	if((Adapter == NULL) || Adapter->bHaltPending )
	{
		// sangam : ******* check how to indicate appln through  mesg or error status??
		DumpDebug(DRV_ENTRY, "can't find adapter or Device Removed");
		return -ENODEV; 
	}
	
	process = controlFindProcessById(Adapter, current->tgid);

	if(process == NULL) {
		DumpDebug(DRV_ENTRY, "uwbrdev_write: process %d not exist", current->tgid);
		return -ESRCH;
	}

	process->Irp = FALSE;
	wake_up_interruptible(&process->read_wait);

	DumpDebug(DRV_ENTRY,"uwbrdev_write: release read operation tigd=%d", current->tgid);
	
	LEAVE;
	return 0;
}

static struct file_operations uwbr_fops = {
	owner:		THIS_MODULE,
	open:		uwbrdev_open,
	release:		uwbrdev_release,
	ioctl:		uwbrdev_ioctl,
	read:		uwbrdev_read,
	write:		uwbrdev_write,
};

void	charCreateNewName(PUCHAR str, ULONG index)
{
	UCHAR tempName[] = "uwbrdev";
	sprintf(str,"%s%lu", tempName,index);
	return;
}

static void Adapter_interrupt(struct sdio_func *func)
{
	MINIPORT_ADAPTER *Adapter = sdio_get_drvdata(func);
	HW_PRIVATE_PACKET hdr;
	int intrd = 0;
	int err;

	wake_lock_timeout(&wimax_rxtx_lock, 0.5 * HZ);
	
//	ENTER;
	if (likely(!Adapter->bHaltPending)) {
		// increase interrupts count
		Adapter->stats.rx.Ints++;
		// read interrupt identification register
		intrd = sdio_readb(func, SDIO_INT_STATUS_REG, NULL);

		sdio_writeb(func, ~intrd, SDIO_INT_STATUS_REG, NULL);
		if (likely(intrd & SDIO_INT_DATA_READY)) {
			if (unlikely(Adapter_sdio_rx_packet(Adapter) != 0)) {
				Adapter->netstats.rx_errors ++;
				DumpDebug(RX_DPC, "Adapter_sdio_rx_packet Error occurred!!!!!!!!!");
			}
		}
		else if(intrd & SDIO_INT_ERROR) {
			DumpDebug(RX_DPC, "Adapter_sdio_rx_packet intrd = SDIO_INT_ERROR occurred!!!!!!!!!");
		}
	}
	else {
		// done with interrupt
		DumpDebug(RX_DPC, "Adapter->bHaltPending=TRUE in Adapter_interrupt !!!!!!!!!");
		
		// send stop message
		hdr.Id0	 = 'W';
		hdr.Id1	 = 'P';
		hdr.Code  = HwCodeHaltedIndication;
		hdr.Value = 0;
		
		err=	sdio_memcpy_toio(Adapter->func,SDIO_DATA_PORT_REG, &hdr, sizeof(HW_PRIVATE_PACKET));
		if (err < 0) {			
			DumpDebug(RX_DPC, "Adapter->bHaltPending=TRUE and send HaltIndication to FW err = (%d)  !!!!!!!!!",err);
			return;
		}
	}
//	LEAVE;
}

extern int g_isWiMAXProbe;	//cky 20100511 wibrogpio.c
static struct net_device_ops wimax_net_ops = {
        .ndo_open = Adapter_open,
        .ndo_stop = Adapter_close,
        .ndo_get_stats = Adapter_netdev_stats,
        .ndo_do_ioctl = Adapter_ioctl,
        .ndo_start_xmit = Adapter_start_xmit,
        .ndo_set_mac_address = NULL,
        .ndo_set_multicast_list = Adapter_set_multicast
};

static int Adapter_probe(struct sdio_func *func,
			 const struct sdio_device_id *id)
{
	struct sdio_func_tuple *tuple = func->tuples;
	struct net_device *net;
	MINIPORT_ADAPTER *Adapter;
	int nRes = -ENOMEM;
	UCHAR charName[512] = "uwbrdev";
	ULONG	idx;

	ENTER;
	DumpDebug(DRV_ENTRY, "Probe!!!!!!!!!");
	g_isWiMAXProbe = 1;
	while (tuple) {
		//DumpDebug(DRV_ENTRY, "code 0x%x size %d", tuple->code, tuple->size);
		tuple = tuple->next;
	}

	net = alloc_etherdev(sizeof (struct MINIPORT_ADAPTER));
	if (!net)
	{
		dev_err(&func->dev, " error can't allocate %s", "device");
		goto alloceth_fail;
	}	
	Adapter = netdev_priv(net);
	memset (Adapter, 0, sizeof (struct MINIPORT_ADAPTER));
	Adapter_global = Adapter;

	// Initialize control
	controlInit(Adapter);

	if( (nRes = hwInit(Adapter)) ) {
		DumpDebug(DRV_ENTRY, "error Can't allocate receive buffer");
		goto hwInit_fail; 
	}
	
	spin_lock_init(&Adapter->rx_pool_lock);
	strcpy(net->name, "uwbr%d");

	Adapter->func = func;
	Adapter->net = net;
//	SET_MODULE_OWNER(net);
#if 0
	net->init = &Adapter_init;
	net->uninit = &Adapter_uninit;

	net->open = &Adapter_open;
	net->stop = &Adapter_close;
	net->tx_timeout = &Adapter_tx_timeout;
	net->do_ioctl = &Adapter_ioctl;
	net->hard_start_xmit = &Adapter_start_xmit;
	net->set_multicast_list = &Adapter_set_multicast;
	net->get_stats = &Adapter_netdev_stats;
#else
	net->netdev_ops = &wimax_net_ops;
#endif	
	net->watchdog_timeo = MINIPORT_ADAPTERX_TIMEOUT;
	net->mtu = MINIPORT_MTU_SIZE;
	Adapter->msg_enable = netif_msg_init (msg_level, NETIF_MSG_DRV
				| NETIF_MSG_PROBE | NETIF_MSG_LINK);
	
	ether_setup(net);
	net->flags |= IFF_NOARP;

	Adapter->MediaState = MEDIA_DISCONNECTED; 
	Adapter->InitDone = FALSE;
	Adapter->bHaltPending = FALSE;	// use for the char device remove also
	Adapter->DownloadMode = TRUE;
	Adapter->SurpriseRemoval = FALSE;

	// save adapter to adapters array
	if ((Adapter->AdapterIndex = adpSave(Adapter)) == (ULONG)(-1)) {
	    goto adpsave_fail;
	}

	fill_skb_pool(Adapter);
	
	if (Adapter->rx_skb == NULL)
		Adapter->rx_skb = pull_skb(Adapter); 
	
	sdio_set_drvdata(func, Adapter);
#ifdef WIBRO_USB_SELECTIVE_SUSPEND	
	Adapter->Power.DevicePowerState = PowerDeviceD0;
	Adapter->Power.IdleImminent = FALSE;
#endif

	netif_carrier_off(net);	//cky 20100402
	netif_tx_stop_all_queues(net);

	SET_NETDEV_DEV(net, &func->dev);
	nRes = register_netdev(net);	
	if (nRes)		
		goto regdev_fail;

	sdio_claim_host(Adapter->func);
	nRes = sdio_enable_func(Adapter->func);
	if(nRes < 0) {
		DumpDebug(DRV_ENTRY, "sdio_enable func error = %d",nRes);
		goto sdioen_fail;
	}
	
	nRes = sdio_claim_irq(Adapter->func, Adapter_interrupt);
	if (nRes < 0) {
		DumpDebug(DRV_ENTRY, "sdio_claim_irq = %d",nRes);
		goto sdioirq_fail;
	}
	//DumpDebug(DRV_ENTRY, "sangam dbg func->max_blksize = %d,  func->cur_blksize = %d", Adapter->func->max_blksize, Adapter->func->cur_blksize);
	if (!sdio_set_block_size(Adapter->func, 512))
		DumpDebug(DRV_ENTRY, "sangam dbg changed the block size to = %d", func->cur_blksize);

	//func->card->host->skip_pwrmgt = 1;
	
	sdio_release_host(Adapter->func); 
		
	// get adapter index
    idx = adpIndex(Adapter);
    if (idx == (ULONG)(-1)) {
		DumpDebug(DRV_ENTRY, "error can't get free device index");
    }
	
	memset(charName, '\0', sizeof(charName));
	charCreateNewName(charName, idx);
	if (register_chrdev(UWBRBDEVMAJOR, charName, &uwbr_fops) != 0) {
		DumpDebug(DRV_ENTRY, "WiBroB_drv: register_chrdev() failed");
		goto regchar_fail;
	}
	// sangam dbg : Dummy value for "ifconfig up" for 2.6.24 
	random_ether_addr(node_id);
	memcpy(net->dev_addr, node_id, sizeof(node_id));
	DumpDebug(DRV_ENTRY, "%s, dummy mac: %02x:%02x:%02x:%02x:%02x:%02x",
		net->name,
		net->dev_addr [0], net->dev_addr [1],
		net->dev_addr [2], net->dev_addr [3],
		net->dev_addr [4], net->dev_addr [5]);

#ifdef DISCONNECT_TIMER
	hwInitDisconnectTimer(Adapter);	
#endif

	if (hwStart(Adapter)) {
		// Free the resources and stop the driver processing
		unregister_chrdev(UWBRBDEVMAJOR, "uwbrdev");
		DumpDebug(DRV_ENTRY, "hwStart failed");
		goto regchar_fail;
	}
	Adapter->InitDone = TRUE; 

	addProcEntry(Adapter);
	// reinit flag
	adpSetHaltFlag(Adapter->AdapterIndex, FALSE);

	set_wimax_pm(wimax_suspend, wimax_resume);   //cky 20100427

	max8893_ldo_enable_direct(4);

	LEAVE;
	return 0;

regchar_fail:
	sdio_claim_host(Adapter->func);
	sdio_release_irq(Adapter->func);
sdioirq_fail:
	sdio_disable_func(Adapter->func);
sdioen_fail:	
   	sdio_release_host(Adapter->func); 
	unregister_netdev(Adapter->net);
regdev_fail:
	sdio_set_drvdata(func, NULL);
	free_skb_pool(Adapter);
	adpRemove(Adapter);
adpsave_fail:
	hwRemove(Adapter);	
hwInit_fail:	
	free_netdev(net);
alloceth_fail:
	LEAVE;
	return nRes;
	LEAVE;
}
	
static void Adapter_remove(struct sdio_func *func)
{
	MINIPORT_ADAPTER *Adapter = sdio_get_drvdata(func);

	ENTER;
	if (!Adapter) {
		dev_dbg(&func->dev, "unregistering non-bound device?");
		return;
	}
	
	Adapter->InitDone = FALSE;
	if (Adapter->MediaState == MEDIA_CONNECTED)  
	{
		netif_stop_queue(Adapter->net);		
		Adapter->MediaState = MEDIA_DISCONNECTED;   
	}
	// set halt flag
    adpSetHaltFlag(Adapter->AdapterIndex, TRUE);
    // remove adapter from adapters array
    adpRemove(Adapter);

	if(!Adapter->SurpriseRemoval) {
		hwStop(Adapter);	//sangam :to free hw in and out buffer
	}

	if(Adapter->DownloadMode) {
		Adapter->SurpriseRemoval = TRUE;
		if(!completion_done(&Adapter->hFWInitCompleteEvent))
			complete(&Adapter->hFWInitCompleteEvent);
		Adapter->bFWDNEndFlag = TRUE;
		wake_up_interruptible(&Adapter->hFWDNEndEvent);
	}

	if(!completion_done(&Adapter->hAwakeAckEvent))
		complete(&Adapter->hAwakeAckEvent);
	
	Adapter->flags |= ADAPTER_UNPLUG;
#ifdef DISCONECT_TIMER
	hwStopDisconnectTimer(Adapter);	
#endif

	mpFreeResources(Adapter);

#ifdef WIBRO_SDIO_WMI
	mgtRemove(Adapter);
#endif
	
	unregister_chrdev(UWBRBDEVMAJOR, "uwbrdev");
	if (Adapter->net)
		unregister_netdev(Adapter->net); 
	else
		DumpDebug(DISPATCH, "Fail...unregister_netdev");

	free_netdev(Adapter->net);

	unset_wimax_pm();    //cky 20100427

        max8893_ldo_disable_direct(4);       //cky 20100611

	LEAVE;
	return;
}
#if 0
static int Adapter_resume(struct sdio_func *func)
{
	MINIPORT_ADAPTER *Adapter = sdio_get_drvdata(func);

	ENTER;
	if (!Adapter) {
		dev_dbg(&func->dev, "unregistering non-bound device?");
		return;
	}
	Adapter_reset(Adapter);		// sangam :to Reset the queues
	LEAVE;
	return 0;
}
#endif
static const struct sdio_device_id Adapter_table[] = {
	{ SDIO_DEVICE(0x04e8, 0x6731) },
//	{ SDIO_DEVICE_CLASS(SDIO_CLASS_NONE) },
	{ }	/* Terminating entry */
};

static struct sdio_driver Adapter_driver = {
	.name		= "C730SDIO",
	.probe		= Adapter_probe,
	.remove		= Adapter_remove,
	.id_table		= Adapter_table,
//	.resume		= Adapter_resume,
};

BOOLEAN hwGPIODeInit(void);

static int __init Adapter_init_module(void)
{
	int error = 0;
	pr_info("%s: %s, " DRIVER_DESC "\n", driver_name, WIBRO_USB_DRIVER_VERSION_STRING);
	
	DumpDebug(DRV_ENTRY, "SDIO driver installing");
	error = sdio_register_driver(&Adapter_driver);
	if (error)
		return error;
	adpInit();

	wake_lock_init(&wimax_wake_lock, WAKE_LOCK_SUSPEND, "wibro_wimax_sdio");
	wake_lock_init(&wimax_rxtx_lock, WAKE_LOCK_SUSPEND, "wibro_wimax_rxtx");

	return error;
}

static void __exit Adapter_exit(void)
{
	DumpDebug(DRV_ENTRY, "SDIO driver Uninstall");

	wake_lock_destroy(&wimax_wake_lock);
	wake_lock_destroy(&wimax_rxtx_lock);

#if 1	//cky 20100424 from hwRemove
	if (Adapter_global && Adapter_global->hw.ReceiveTempBuffer != NULL)
	{
		DumpDebug(DRV_ENTRY, "Free ReceiveBuffer");
		kfree(Adapter_global->hw.ReceiveTempBuffer);
	}
#endif

	UnLoadWiMaxImage();
	
//	gpio_wimax_poweroff();
	sdio_unregister_driver(&Adapter_driver);
}

//cky 20100217 suspend/resume
void wimax_suspend(void)
{
	//DumpDebug(DRV_ENTRY, "wimax_suspend() +++");

#if defined (CONFIG_MACH_QUATTRO)	//cky 20100224

	// set MEM0 interface
	s3c_gpio_cfgpin(WIMAX_RESET, S3C_GPIO_SFN(2));

	// set WIMAX_INT
	s3c_gpio_cfgpin(WIMAX_INT, S3C_GPIO_SFN(2));	// ext int
	s3c_gpio_setpull(WIMAX_INT, S3C_GPIO_PULL_UP);
	
#elif defined (CONFIG_MACH_ARIES)

	// cky 20100427 wimax state changed to VI/IDLE
	if (gpio_get_value(WIMAX_IF_MODE1))		// 1: IDLE
	{
#if 0	//cky 20100503
		// IDLE noti
		char sendBuf[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0x15,0x00,
							0x00, 0x06, 0x00, 0x04, 0x00, 0x00};
		int sendLength = 20;
		controlReceive(Adapter_global, sendBuf, sendLength);
#endif
		// set driver status IDLE
		Adapter_global->WibroStatus = WIBRO_STATE_IDLE;
		DumpDebug(RX_DPC, "Force WIMAX IDLE");
	}
	else		// 0: VI
	{
#if 0	//cky 20100503
		// VI noti
		char sendBuf[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0x15,0x00,
							0x00, 0x06, 0x00, 0x07, 0x00, 0x00};
		int sendLength = 20;
		
		controlReceive(Adapter_global, sendBuf, sendLength);
#endif
#if 1	//cky 20101008 NO VI REQ by driver (platform make modem VI)
		//cky 20100702 workaround: do not send vi req all cases
		// *** it must be modified if the VI setting is used!! ***
		gpio_set_value(WIMAX_IF_MODE1, GPIO_LEVEL_HIGH);
#else
		//cky 20100528 workaround for VI -> VI
		if (Adapter_global->WibroStatus == WIBRO_STATE_VIRTUAL_IDLE)
		{
			DumpDebug(RX_DPC, "WIMAX Already VI");
			gpio_set_value(WIMAX_IF_MODE1, GPIO_LEVEL_HIGH);
		}
		else
		{
			// set driver status VI		
			Adapter_global->MediaState = MEDIA_DISCONNECTED;
			netif_stop_queue(Adapter_global->net);
			netif_carrier_off(Adapter_global->net);	//cky 20100402
			Adapter_global->WibroStatus = WIBRO_STATE_VIRTUAL_IDLE;
			DumpDebug(RX_DPC, "Force WIMAX VI");
		}
#endif
	}

	if (isDumpEnabled())
	{
		// change UART path to WIMAX
		s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_LOW);
		s3c_gpio_cfgpin(GPIO_UART_SEL1, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL1, GPIO_LEVEL_HIGH);
	}
	
	msleep(10);

	//cky 20100428 PDA Active pins low
	s3c_gpio_cfgpin(WIMAX_CON1, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_CON1, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(WIMAX_CON2, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_CON2, GPIO_LEVEL_LOW);
	
#endif
}

//cky 20100217 suspend/resume
void wimax_resume(void)
{
	//DumpDebug(DRV_ENTRY, "wimax_resume() +++");

#if defined (CONFIG_MACH_QUATTRO)	//cky 20100224

	// set OUTPUT
	s3c_gpio_cfgpin(GPIO_WIMAX_RESET_N, S3C_GPIO_SFN(1));

	// clear WIMAX_INT
	s3c_gpio_cfgpin(WIMAX_INT, S3C_GPIO_SFN(0));	// input
	s3c_gpio_setpull(WIMAX_INT, S3C_GPIO_PULL_NONE);
	
#elif defined (CONFIG_MACH_ARIES)

	if (Adapter_global->WibroStatus == WIBRO_STATE_VIRTUAL_IDLE)
	{
		gpio_set_value(WIMAX_IF_MODE1, GPIO_LEVEL_LOW);
	}
	
	wake_lock_timeout(&wimax_wake_lock, 5 * HZ);

	if (isDumpEnabled())
	{
		// UART path to PDA
		s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH);
		s3c_gpio_cfgpin(GPIO_UART_SEL1, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL1, GPIO_LEVEL_LOW);
	}

	//msleep(20);	//cky 20100608 wait for SDIO ready
	
	// PDA active pins high
	s3c_gpio_cfgpin(GPIO_WIMAX_CON1, S3C_GPIO_SFN(1));
	gpio_set_value(GPIO_WIMAX_CON1, GPIO_LEVEL_HIGH);
	s3c_gpio_cfgpin(GPIO_WIMAX_CON2, S3C_GPIO_SFN(1));
	gpio_set_value(GPIO_WIMAX_CON2, GPIO_LEVEL_HIGH);

#endif
}

module_init(Adapter_init_module);
module_exit(Adapter_exit);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");

