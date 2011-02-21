// receive.c
//
#include "headers.h"
#include "download.h"

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#if defined (CONFIG_MACH_QUATTRO)
#include <mach/instinctq.h>
#elif defined (CONFIG_MACH_ARIES)
//#include <mach/gpio-jupiter.h>	// Yongha for Victory WiMAX 20100208
#if defined CONFIG_S5PV210_VICTORY
#include <mach/victory/gpio-aries.h>	// Yongha for Victory WiMAX 20100208
#elif defined CONFIG_S5PV210_ATLAS
#include <mach/atlas/gpio-aries.h>	// Yongha for Victory WiMAX 20100208
#endif
#endif

#if defined (CONFIG_MACH_QUATTRO)
#include <linux/i2c/pmic.h>
#endif

#include <plat/sdhci.h>
#include <plat/devs.h>
#include <linux/mmc/host.h>

#include "wimax_plat.h"

void DL_hwIndicatePacket(PMINIPORT_ADAPTER Adapter, PUCHAR pRecvPacket);
extern BOOLEAN SendImageInfoPacket(PMINIPORT_ADAPTER Adapter, unsigned short uiCmdId);
extern BOOLEAN SendImageDataPacket(PMINIPORT_ADAPTER Adapter, unsigned short uiCmdId);
extern VOID hwGetMacAddressThread(VOID *data);
extern BOOLEAN SendCmdPacket(PMINIPORT_ADAPTER Adapter, unsigned short uiCmdId);
extern int wimax_timeout_multiplier;
extern IMAGE_DATA g_stWiMAXImage;
extern DWORD	g_dwLineState;
//extern unsigned long wimax_download_start_time;

extern int getWiMAXSleepMode(void);

extern int s3c_bat_use_wimax(int onoff);	//cky 20100624

void DL_hwIndicatePacket(PMINIPORT_ADAPTER Adapter, PUCHAR pRecvPacket)
{
	PHW_DOWNLOAD_PACKET pDLPacket;
	CHAR *tmpByte;
	PUINT tmpAddress, tmpRecvDataSize;
	//unsigned long nj, msec;

	//DumpDebug(FW_DNLD, "--> %s() line %d", __FUNCTION__, __LINE__);
	pDLPacket = (PHW_DOWNLOAD_PACKET)pRecvPacket;

	if(pDLPacket->MsgType == be16_to_cpu(ETHERTYPE_DL))
	{
		//DumpDebug(FW_DNLD, "DL_hwIndicatePacket - Download Packet");
		
		switch (be16_to_cpu(pDLPacket->MsgId)) {
			case MSG_DRIVER_OK_RESP:
				//DumpDebug(FW_DNLD, "DL_hwIndicatePacket:  MSG_DRIVER_OK_RESP");
			    tmpByte = (CHAR *)(pRecvPacket+sizeof(HW_DOWNLOAD_PACKET));
				DumpDebug(FW_DNLD, "DL_hwIndicatePacket:  MSG_DRIVER_OK_RESP Value : %x\r", *tmpByte);

				//if(*tmpByte== 0x01)
				//	DumpDebug(FW_DNLD, "DL_hwIndicatePacket:  MSG_DRIVER_OK_RESP - ACK\r");

			    SendImageInfoPacket(Adapter, MSG_IMAGE_INFO_REQ);

			    break;
				
			case MSG_IMAGE_INFO_RESP:
				//DumpDebug(FW_DNLD, "DL_hwIndicatePacket:  MSG_IMAGE_INFO_RESP");
				tmpAddress = (PUINT)(pRecvPacket+sizeof(HW_DOWNLOAD_PACKET));
				DumpDebug(FW_DNLD, "DL_hwIndicatePacket:  MSG_IMAGE_INFO_RESP Value : %x", be32_to_cpu(*tmpAddress));		

				SendImageDataPacket(Adapter, MSG_IMAGE_DATA_REQ);

			    break;
				
			case MSG_IMAGE_DATA_RESP:
				//DumpDebug(FW_DNLD, "DL_hwIndicatePacket:  MSG_IMAGE_DATA_RESP");
				//tmpRecvDataSize = (PUINT)(pRecvPacket+sizeof(HW_DOWNLOAD_PACKET));
				//DumpDebug(FW_DNLD, "DL_hwIndicatePacket:  MSG_IMAGE_DATA_RESP Value : %d\r", be32_to_cpu(*tmpRecvDataSize));

				 if(g_stWiMAXImage.uiOffset == g_stWiMAXImage.uiSize)	// Image Data
				 {
					DumpDebug(FW_DNLD, "DL_hwIndicatePacket:  MSG_IMAGE_DATA_RESP Image Download Complete\r");	
				 	SendCmdPacket(Adapter, MSG_RUN_REQ); // Run Message Send
				 }
				 else
				 {
				 	SendImageDataPacket(Adapter, MSG_IMAGE_DATA_REQ);
					//mdelay(1);
				 }
				 	
				break;
				
			case MSG_RUN_RESP:
				DumpDebug(FW_DNLD, "DL_hwIndicatePacket:  MSG_RUN_RESP");
				tmpByte = (CHAR *)(pRecvPacket+sizeof(HW_DOWNLOAD_PACKET));
				//DumpDebug(FW_DNLD, "DL_hwIndicatePacket:  MSG_RUN_RESP Value : %x", *tmpByte);

				if(*tmpByte== 0x01)
				{
					DumpDebug(FW_DNLD, "DL_hwIndicatePacket:  MSG_RUN_RESP - ACK");

					if(g_dwLineState == SDIO_MODE || g_dwLineState == DM_MODE ||
						g_dwLineState == USB_MODE || g_dwLineState == USIM_RELAY_MODE)
					{
						init_completion(&Adapter->hFWInitCompleteEvent);
						if (wimax_timeout_multiplier>1)
							msleep(3000);
						if (kernel_thread(hwGetMacAddressThread, Adapter, 0) < 0) {
							DumpDebug(FW_DNLD, "Error :kernel_thread create fail");
							return;
						}
			      	}
					// sangam dbg : temporarily added, check for the flow..
					else if(g_dwLineState == WTM_MODE || g_dwLineState == AUTH_MODE) 
					{
						Adapter->bFWDNEndFlag = TRUE;
						wake_up_interruptible(&Adapter->hFWDNEndEvent);

						//sdhci_s3c_force_presence_change(&DEVICE_HSMMC);
					}
			    }
				//UnLoadWiMaxImage();
				break;
				
			default:
				DumpDebug(FW_DNLD, "DL_hwIndicatePacket:  Unkown MsgID Type\r");		
				break;			    
		}
	}
	else
		DumpDebug(FW_DNLD, "DL_hwIndicatePacket - is not download pakcet\r");

	//DumpDebug(FW_DNLD, "<-- %s() line %d", __FUNCTION__, __LINE__);
	return;
	// put descriptor back to free queue
       //DL_hwReturnPacket(Adapter, pDescr);
}

u_long hwProcessPrivateCmd(MINIPORT_ADAPTER *pAdapter, void *Buffer)
{
	PHW_PRIVATE_PACKET cmd = (PHW_PRIVATE_PACKET)Buffer;
	UCHAR *bufp;
	ENTER;

	switch(cmd->Code)
	{
		case HwCodeMacResponse:
		{
			DumpDebug(RX_DPC, "WiBro SDIO:  MAC Response");
			bufp = (PUCHAR)Buffer;

			/* processing for mac_req request */
			complete(&pAdapter->hFWInitCompleteEvent);
			DumpDebug(RX_DPC,"MAC address = {%x, %x, %x, %x, %x, %x}", bufp[3], bufp[4], bufp[5], bufp[6], bufp[7], bufp[8]);
			memcpy(pAdapter->CurrentAddress, bufp + 3, MINIPORT_LENGTH_OF_ADDRESS);

			// create ethernet header
			memcpy(pAdapter->hw.EthHeader, pAdapter->CurrentAddress, MINIPORT_LENGTH_OF_ADDRESS);
			memcpy(pAdapter->hw.EthHeader + MINIPORT_LENGTH_OF_ADDRESS, pAdapter->CurrentAddress, MINIPORT_LENGTH_OF_ADDRESS);
			pAdapter->hw.EthHeader[(MINIPORT_LENGTH_OF_ADDRESS * 2) - 1] += 1;					

			memcpy(pAdapter->net->dev_addr, bufp + 3, MINIPORT_LENGTH_OF_ADDRESS);
			pAdapter->acquired_mac_address=TRUE;					

			s3c_gpio_cfgpin(WIMAX_WAKEUP, S3C_GPIO_SFN(1));
			s3c_gpio_setpull(WIMAX_WAKEUP, S3C_GPIO_PULL_NONE);
			gpio_set_value(WIMAX_WAKEUP, GPIO_LEVEL_HIGH);	
				
			if(pAdapter->DownloadMode) {
				pAdapter->bFWDNEndFlag = TRUE;
				wake_up_interruptible(&pAdapter->hFWDNEndEvent);
			}
				
			return (sizeof(HW_PRIVATE_PACKET) + MINIPORT_LENGTH_OF_ADDRESS - sizeof(UCHAR));
		}
		case HwCodeLinkIndication:
		{
			DumpDebug(RX_DPC, " link indication packet (%04x), value(%04x)",cmd->Code, cmd->Value);
#ifdef MINIPORT_USE_WMI_OID
			if (pAdapter->mgt.Enabled) {
				DumpDebug(RX_DPC, "WiBro SDIO:  indication skipped");
				break;
			}
#endif
			if ((cmd->Value == HW_PROT_VALUE_LINK_DOWN) && (pAdapter->MediaState != MEDIA_DISCONNECTED)) {
			// start disconnect indication timer
#ifdef DISCONNECT_TIMER 
				hwStartDisconnectTimer(pAdapter);
#else
				pAdapter->MediaState = MEDIA_DISCONNECTED;
				netif_stop_queue(pAdapter->net);
				netif_carrier_off(pAdapter->net);	//cky 20100402
				pAdapter->WibroStatus = WIBRO_STATE_READY;	
				DumpDebug(RX_DPC, "LINK_DOWN_INDICATION");

				// AP sleep mode: WIMAX VI
				gpio_set_value(WIMAX_IF_MODE1, GPIO_LEVEL_LOW);

#endif
			}
			else if ((cmd->Value == HW_PROT_VALUE_LINK_UP) && (pAdapter->MediaState != MEDIA_CONNECTED)) {
				// set value to structure
				pAdapter->MediaState = MEDIA_CONNECTED;
				// indicate link up
				netif_start_queue(pAdapter->net);
				pAdapter->net->mtu = MINIPORT_MTU_SIZE;	//sangam set mtu after connect
				netif_carrier_on(pAdapter->net);
				pAdapter->WibroStatus = WIBRO_STATE_NORMAL;
				DumpDebug(RX_DPC, "LINK_UP_INDICATION");

				// AP sleep mode: WIMAX IDLE or VI
				gpio_set_value(WIMAX_IF_MODE1, getWiMAXSleepMode());
				DumpDebug(RX_DPC, "AP SLEEP: WIMAX %d (0: VI, 1: IDLE)", getWiMAXSleepMode());
			}
			break;
		}
		case HwCodeHaltedIndication:
		{
			DumpDebug(RX_DPC, "Device is about to reset, stop driver");
			pAdapter->bHaltPending = TRUE;
			break;
		}
		case HwCodeRxReadyIndication:
		{
			
			DumpDebug(RX_DPC, "Device RxReady");
			// stop timer
#ifdef DISCONNECT_TIMER		
			hwStopRetryTimer(pAdapter);
#endif
			// increase stats
			pAdapter->stats.tx.Ack++;
			// device ready to receive new packet
			DumpDebug(RX_DPC, "Ack");
			// sangam : to start the data packet send queue again after stopping in xmit
			if (pAdapter->MediaState == MEDIA_CONNECTED)
				netif_wake_queue(pAdapter->net);
			break;
		}
		case HwCodeIdleNtfy:
		{			
			// set idle
			DumpDebug(RX_DPC, "hwProcessPrivateCmd: Device is going to idle mode - Status:[%d]", pAdapter->WibroStatus);
			if(pAdapter->WibroStatus == WIBRO_STATE_NORMAL || pAdapter->WibroStatus == WIBRO_STATE_IDLE)
			{				
				DumpDebug(RX_DPC, "hwProcessPrivateCmd: Set IDLE");				
				pAdapter->WibroStatus = WIBRO_STATE_IDLE;
			}
			else
			{
				DumpDebug(RX_DPC, "hwProcessPrivateCmd: Set VIRTUAL IDLE - Status:[%d]", pAdapter->WibroStatus);
				pAdapter->WibroStatus = WIBRO_STATE_VIRTUAL_IDLE;
			}
			s3c_bat_use_wimax(0);	//cky 20100624
			break;
		}
		case HwCodeWakeUpNtfy:
		{			
			// clear idle
			DumpDebug(RX_DPC, "hwProcessPrivateCmd: Device is now in normal state and ready to operate - Status: [%d]", pAdapter->WibroStatus);
#ifdef WAKEUP_BY_GPIO
			if(pAdapter->WibroStatus == WIBRO_STATE_AWAKE_REQUESTED)
			{
				complete(&pAdapter->hAwakeAckEvent);
				break;
			}
#endif
			if(pAdapter->WibroStatus == WIBRO_STATE_IDLE)
			{
				DumpDebug(RX_DPC, "hwProcessPrivateCmd: IDLE -> NORMAL");				
				pAdapter->WibroStatus = WIBRO_STATE_NORMAL;
			}
			else if(pAdapter->WibroStatus == WIBRO_STATE_NORMAL)
			{
				DumpDebug(RX_DPC, "hwProcessPrivateCmd: Already Normal maybe AP Wakeup");
				pAdapter->WibroStatus = WIBRO_STATE_NORMAL;
			}
			else
			{
				DumpDebug(RX_DPC, "hwProcessPrivateCmd: VI -> READY");
				pAdapter->WibroStatus = WIBRO_STATE_READY;
			}
			s3c_bat_use_wimax(1);	//cky 20100624
			break;
		}
		
		default:
			DumpDebug(RX_DPC, "Command = %04x", cmd->Code);
		
	}
	
    return sizeof(HW_PRIVATE_PACKET);
	LEAVE;
}


#if 0
unsigned int processSdioReceiveData(MINIPORT_ADAPTER *Adapter, void *Buffer, u_long Length, long Timeout)
{
	unsigned int offset = 0, PacketLen = 0, remain_len = 0;
	PHW_PACKET_HEADER hdr = (PHW_PACKET_HEADER)Buffer; 
	unsigned long PacketType;
	struct net_device 	*net = Adapter->net;
	u_long flags;
	
	ENTER;
	DumpDebug(RX_DPC,"Total length of packet = %lu", Length); 

	PacketType = HwPktTypeNone;
	PacketLen = Length;
#if 0		
	if(Adapter->bHaltPending)
		return 0;
#endif	

	if (hdr->Id0 != 'W') { // "WD", "WC", "WP" or "WE"
		DumpDebug(RX_DPC, "WiBro USB: Wrong packet ID (%c%c or %02x%02x)", hdr->Id0, hdr->Id1, hdr->Id0, hdr->Id1);
		// skip rest of packets
		PacketLen = 0;
	}
	// check packet type
	if ((hdr->Id0 == 'W') && (hdr->Id1 == 'P')) {
	    DumpDebug(RX_DPC, "WiBro USB: control packet (private) ");
	    // set packet type
	    PacketType = HwPktTypePrivate;
	}
	else if ((hdr->Id0 == 'W') && (hdr->Id1 == 'C')) {
	    DumpDebug(RX_DPC, "WiBro USB: CONTROL packet <<<-----------------------------------");
	    PacketType = HwPktTypeControl;
	}
	else if ((hdr->Id0 == 'W') && (hdr->Id1 == 'D')) {
	    DumpDebug(RX_DPC, "WiBro USB: DATA packet <<<-------------------------------------------------------------------");
	    PacketType = HwPktTypeData;
	}
	else
	{
		DumpDebug(RX_DPC, "WiBro USB: Wrong packet ID (%c%c or %02x%02x)", hdr->Id0, hdr->Id1, hdr->Id0, hdr->Id1);
		return 0;
	}
	
	if (PacketType != HwPktTypePrivate) 
	{
		if (!Adapter->DownloadMode) {
			if (hdr->Length > MINIPORT_MAX_TOTAL_SIZE) { 
				DumpDebug(RX_DPC,"WiBro USB: Packet length is too big (%d)", hdr->Length); 
				// skip rest of packets
				PacketLen = 0;
			}
		}
		
		if (hdr->Length)
		{
		    // change offset
		    offset += sizeof(HW_PACKET_HEADER);
#ifdef HARDWARE_USE_ALIGN_HEADER
			if(!Adapter->DownloadMode)
				offset += 2;		
#endif			
			if(PacketLen <4 ) {
				PacketLen = 0;
				return 0;		
			}

			PacketLen -= offset; /* from this line, Packet length is read data size without samsung header YHM */
		    // copy packet

#ifdef HARDWARE_USE_SUPPRESS_MAC_HEADER //sangam dbg:If suppress mac address enabled during send same applicable to receive
				memcpy((unsigned char *)Adapter->hw.ReceiveBuffer, Adapter->hw.EthHeader, MINIPORT_LENGTH_OF_ADDRESS * 2);
				memcpy((unsigned char *)Adapter->hw.ReceiveBuffer + (MINIPORT_LENGTH_OF_ADDRESS * 2),
						((unsigned char *) Buffer) + offset, PacketLen);
				PacketLen += (MINIPORT_LENGTH_OF_ADDRESS * 2);
#else
				memcpy((unsigned char *)Adapter->hw.ReceiveBuffer, ((unsigned char *) Buffer) + offset, PacketLen);
#endif				
			if(PacketType == HwPktTypeData)
			{
				if (Adapter->rx_skb == NULL)
				{
					spin_lock_irqsave(&Adapter->rx_pool_lock, flags);
					fill_skb_pool(Adapter);
					Adapter->rx_skb = pull_skb(Adapter);
					spin_unlock_irqrestore(&Adapter->rx_pool_lock, flags);
					if(Adapter->rx_skb == NULL ){
						DumpDebug(RX_DPC, "%s : Low on Memory",Adapter->net->name);
						return -ENOMEM;
					}
				}
				memcpy((unsigned char *)Adapter->rx_skb->data, (unsigned char *)Adapter->hw.ReceiveBuffer, PacketLen);
#ifdef HARDWARE_USE_SUPPRESS_MAC_HEADER
				if(hdr->Length == (PacketLen - (MINIPORT_LENGTH_OF_ADDRESS * 2))) 
#else	
				if(hdr->Length == PacketLen) 
#endif
					DumpDebug(RX_DPC,"Just one DATA packet processed and remained len is = %d",(PacketLen - hdr->Length))
				else {
					remain_len = PacketLen - hdr->Length;					
					DumpDebug(RX_DPC,"multiple DATA packet received and remained len is = %d",remain_len);
				}
			}
			else
			{			
#ifdef HARDWARE_USE_SUPPRESS_MAC_HEADER
				if(hdr->Length == (PacketLen - (MINIPORT_LENGTH_OF_ADDRESS * 2))) 
#else	
				if(hdr->Length == PacketLen) 
#endif
					DumpDebug(RX_DPC,"Just one C or P packet processed and remained len is = %d", (PacketLen - hdr->Length))
				else{
					remain_len = PacketLen - hdr->Length;					
					DumpDebug(RX_DPC,"multiple C or P packet received and remained len is = %d",remain_len);
				}				
			}
		}
		else
			PacketLen = 0;
	}
	
	switch(PacketType) {
		case HwPktTypeControl:
		{
			if (PacketLen) {
#ifndef WIBRO_SDIO_WMI				
				if(Adapter->DownloadMode)
				{
					//printk("sangam dbg : packet len = %d", PacketLen);
					DL_hwIndicatePacket(Adapter, Adapter->hw.ReceiveBuffer);
				}
				else
					controlReceive(Adapter, Adapter->hw.ReceiveBuffer, PacketLen); 
#else
				if(Adapter->Mgt.Enabled)
					mgtReceive(Adapter, Adapter->hw.ReceiveBuffer, PacketLen);
#endif
			}
			PacketLen = 0;
			return 0; //sangam continue read_callback
			//return;
		}
		case HwPktTypePrivate:
		{
			/*In here, received data doesn't copied to Adapter->hw.ReceiveBuffer yet , Data is in Buffer YHM */
			PHW_PRIVATE_PACKET  req;
 			u_char * bufp;// = mac_res; 
			u_long l = 0;
			if(!(Adapter->req_mac)){
				if (PacketLen) {
					l = hwProcessPrivateCmd(Adapter, Buffer, PacketLen);
				}
				// restart
				PacketLen = 0;
				return 0; //sangam continue read_callback
				//return;
			}
			else {
				bufp = Buffer;
				req = (PHW_PRIVATE_PACKET)Buffer;
				/* processing for mac_req request */
				DumpDebug(RX_DPC,"Received packet processing for mac_req request Length = 0x%x",PacketLen);
				if ((req->Code == HwCodeMacResponse) && (PacketLen >= (sizeof(HW_PRIVATE_PACKET) - sizeof(u_char) + MINIPORT_LENGTH_OF_ADDRESS))) 
				{
					complete(&Adapter->hFWInitCompleteEvent);
					DumpDebug(RX_DPC,"MAC address = {%x, %x, %x, %x, %x, %x}", bufp[3], bufp[4], bufp[5], bufp[6], bufp[7], bufp[8]);
					bufp[8] = 0x11; // sangam dbg : temporary
					memcpy(Adapter->CurrentAddress, bufp + 3, MINIPORT_LENGTH_OF_ADDRESS);
					// create ethernet header
					memcpy(Adapter->hw.EthHeader, Adapter->CurrentAddress, MINIPORT_LENGTH_OF_ADDRESS);
					memcpy(Adapter->hw.EthHeader + MINIPORT_LENGTH_OF_ADDRESS, Adapter->CurrentAddress, MINIPORT_LENGTH_OF_ADDRESS);
					Adapter->hw.EthHeader[(MINIPORT_LENGTH_OF_ADDRESS * 2) - 1] += 1;					
					memcpy(Adapter->net->dev_addr, bufp + 3, MINIPORT_LENGTH_OF_ADDRESS);
					Adapter->acquired_mac_address=TRUE;					

					s3c_gpio_setpull(WIMAX_WAKEUP, S3C_GPIO_PULL_NONE);
					gpio_set_value(WIMAX_WAKEUP, GPIO_LEVEL_HIGH);	
				
					if(Adapter->DownloadMode) {
							Adapter->bFWDNEndFlag = TRUE;
							wake_up_interruptible(&Adapter->hFWDNEndEvent);
					}
				}
				
				//memset(Adapter->ctrl_buf, 0x0, WIBRO_USB_ENDPOINT_TRANSFER_SIZE); //Reset adter complete urb processing :sangam
				Adapter->req_mac = FALSE;
				return 0;
				//return;
			}			
		}
		case HwPktTypeData:

			skb_put(Adapter->rx_skb, PacketLen);
			Adapter->rx_skb->protocol = eth_type_trans(Adapter->rx_skb, net);
			netif_rx(Adapter->rx_skb);
			Adapter->stats.rx.Data++;
			Adapter->netstats.rx_packets++;
			Adapter->netstats.rx_bytes += PacketLen;

			spin_lock(&Adapter->rx_pool_lock);
			Adapter->rx_skb = pull_skb(Adapter);
			spin_unlock(&Adapter->rx_pool_lock);
			return 0;
		
		case HwPktTypeTimeout: 
			DumpDebug(RX_DPC, "WiBro USB: Timeout, no device features found");
			Adapter->DevFeatures = 0;			
			return 0; //sangam continue read_callback
			//return;	
		default:
			// restart
			PacketLen = 0;
			return 0; //sangam continue read_callback
			//return;
		}
	/*
	 * If the packet is unreasonably long, quietly drop it rather than
	 * kernel panicing by calling skb_put.
	 */
	if (PacketLen > SDIO_BUFFER_SIZE)
		return 0;

	LEAVE;
	return 0;
}
#endif

unsigned int processSdioReceiveData(MINIPORT_ADAPTER *Adapter, void *Buffer, u_long Length, long Timeout)
{
	unsigned int rlen= Length;
	UINT machdrlen =  (MINIPORT_LENGTH_OF_ADDRESS * 2);	//sumanth : initialized it here
	PUCHAR ofs = (PUCHAR)Buffer;
	PHW_PACKET_HEADER hdr;
	unsigned long PacketType;
	struct net_device 	*net = Adapter->net;
	u_long flags;
	int res = 0;
	USHORT DataPacketLength;
	
//	ENTER;

//	if(!Adapter->DownloadMode)	
//		DumpDebug(RX_DPC,"Total length of packet = %lu", Length); 

	while((int)rlen>0)
	{
		hdr = (PHW_PACKET_HEADER)ofs; 
		PacketType = HwPktTypeNone;

		if (unlikely(hdr->Id0 != 'W')) { // "WD", "WC", "WP" or "WE"
			DumpDebug(RX_DPC, "WiBro USB: Wrong packet ID (%c%c or %02x%02x)", hdr->Id0, hdr->Id1, hdr->Id0, hdr->Id1);

#if 1	//cky 20100614 dump wrong packet
		{
			// dump packets
			UINT i;
			PUCHAR  b = (PUCHAR)Buffer;
			DumpDebug(TX_CONTROL, "Wrong packet dump [%d] = ",Length);

			printk("<1>\x1b[1;33m[WiMAX]");
			for (i = 0; i < Length; i++) {
				printk(" %02x", b[i+8]);
				if (((i + 1) != Length) && (i % 16 == 15))
				{
					printk("\x1b[0m\n<1>\x1b[1;33m[WiMAX]");
				}
			}
			printk("\x1b[0m\n");
		}
#endif

			// skip rest of packets
			return 0;
		}
	// check packet type
	if (unlikely(hdr->Id1 == 'P')) 
	{
		ULONG l = 0;
		
		PacketType = HwPktTypePrivate;
		//DumpDebug(RX_DPC, "private control packet");
		
		// process packet
		l = hwProcessPrivateCmd(Adapter, ofs);
		// shift
		ofs += l;
		rlen -= l;

		//DumpDebug(RX_DPC, "Privatecmd Length is %lu", l);
		//DumpDebug(RX_DPC, "Remaining Length is %d", rlen);

		continue;	// shift done so just continue
	}
		
	if (likely(!Adapter->DownloadMode)) {
			if ( unlikely(hdr->Length > MINIPORT_MAX_TOTAL_SIZE ||  ((hdr->Length + sizeof(HW_PACKET_HEADER)) > rlen)) ) { 
				DumpDebug(RX_DPC,"WiBro USB: Packet length is too big (%d)", hdr->Length); 
				// skip rest of packets
				return 0;
			}
	}
	 // change offset
	ofs += sizeof(HW_PACKET_HEADER);
	rlen -= sizeof(HW_PACKET_HEADER); 		

	// check packet type
	switch(hdr->Id1){
		case 'C':
#if 0
			if(!Adapter->DownloadMode)
				DumpDebug(RX_DPC, "hwParseReceivedData : control packet\r");
#endif
			PacketType = HwPktTypeControl;
			break;
		case 'D':
			//DumpDebug(RX_DPC, "hwParseReceivedData : data packet\r");
			PacketType = HwPktTypeData;
			break;
		case 'E':
			//DumpDebug(RX_DPC, "hwParseReceivedData : alignment  packet\r");
			// do not count
			Adapter->stats.rx.Count--;
			// skip rest of buffer
			return 0;
		default:
				DumpDebug(RX_DPC, "hwParseReceivedData : Wrong packet ID [%c%c or %02x%02x]\r",  hdr->Id0, hdr->Id1, hdr->Id0, hdr->Id1);
				// skip rest of buffer
				return 0;
		}


/*		if(likely(!Adapter->DownloadMode || PacketType == HwPktTypeData))
		{
		
#ifdef HARDWARE_USE_ALIGN_HEADER
			ofs += 2;		
			rlen -= 2;
#endif			
		}
*/	//sumanth: the above code is redundant check
	// copy ethernet header if it's a data packet
		if(likely(!Adapter->DownloadMode || PacketType == HwPktTypeData))
		{
#ifdef HARDWARE_USE_ALIGN_HEADER
			ofs += 2;		
			rlen -= 2;
#endif
			DataPacketLength = hdr->Length;	 //sumanth: store the packet length because we will overwrite the hardware packet header
//			machdrlen = (MINIPORT_LENGTH_OF_ADDRESS * 2);		//sumanth: already initialized earlier
			memcpy((unsigned char *)ofs -machdrlen, Adapter->hw.EthHeader, machdrlen); //sumanth: copy the MAC to ofs buffer
//			memcpy((unsigned char *)Adapter->hw.ReceiveBuffer + machdrlen, ofs, DataPacketLength); //sumanth: hw.ReceiveBuffer no longer necessary

			if(unlikely(PacketType == HwPktTypeControl))
			{
				controlReceive(Adapter, (unsigned char *)ofs -machdrlen, DataPacketLength + machdrlen);
			}
			else 
			{
				if (Adapter->rx_skb == NULL)
				{
					spin_lock_irqsave(&Adapter->rx_pool_lock, flags);
					fill_skb_pool(Adapter);
					Adapter->rx_skb = pull_skb(Adapter);
					spin_unlock_irqrestore(&Adapter->rx_pool_lock, flags);
					if(Adapter->rx_skb == NULL ){
						DumpDebug(RX_DPC, "%s : Low on Memory",Adapter->net->name);
						return -ENOMEM;
					}
				}
				memcpy(skb_put(Adapter->rx_skb, (DataPacketLength + machdrlen)), (unsigned char *)ofs -machdrlen, (DataPacketLength + machdrlen));

				Adapter->rx_skb->dev = net;
				Adapter->rx_skb->protocol = eth_type_trans(Adapter->rx_skb, net);
				Adapter->rx_skb->ip_summed = CHECKSUM_UNNECESSARY;
				res = netif_rx(Adapter->rx_skb);

				//DumpDebug(RX_DPC, "sangam dbg netif_rx: res = %d len = %d", res, (hdr->Length + machdrlen));

				Adapter->stats.rx.Data++;
				Adapter->netstats.rx_packets++;
				Adapter->netstats.rx_bytes += (DataPacketLength + machdrlen);

				spin_lock(&Adapter->rx_pool_lock);
				Adapter->rx_skb = pull_skb(Adapter);
				spin_unlock(&Adapter->rx_pool_lock);
			
			}
			hdr->Length = DataPacketLength;	//sumanth: restore the hardware packet length information
		}
		else
		{	// copy length should be less than header size
//			memcpy((unsigned char *)Adapter->hw.ReceiveBuffer, ofs, (hdr->Length -sizeof(HW_PACKET_HEADER)));
				//sumanth: we no longer use the ReceiveBuffer as it is logically not necessary
			hdr->Length -= sizeof(HW_PACKET_HEADER);
			DL_hwIndicatePacket(Adapter, ofs);
		}
	/*
	 * If the packet is unreasonably long, quietly drop it rather than
	 * kernel panicing by calling skb_put.
	 */
			// shift
	ofs += hdr->Length;
	rlen-= hdr->Length;

//	if (rlen > SDIO_BUFFER_SIZE)		//sumanth: we dont have to check this in the loop
//		return 0;

	}

//	LEAVE;
	return 0;
}

