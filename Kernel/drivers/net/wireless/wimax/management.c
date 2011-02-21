
#include "headers.h"

#ifdef WIBRO_SDIO_WMI
/*
    Debug
*/

extern u_int g_uiWOtherDumpLevel;// CHECK_HANG|DUMP_INFO|MP_HALT|CP_CTRL_PKT|DUMP_CONTROL|CONN_MSG|LINK_UP_MSG;
extern u_int g_uiWOtherDumpDetails;// CHECK_HANG|DUMP_INFO|CP_CTRL_PKT|DUMP_CONTROL|CONN_MSG|LINK_UP_MSG;

// Forward declarations
static BOOLEAN mgtProcessLinkResp(MINIPORT_ADAPTER *Adapter , PUCHAR Buffer, ULONG Length);
static BOOLEAN mgtProcessPowerResp(MINIPORT_ADAPTER *Adapter , PUCHAR Buffer, ULONG Length);
static BOOLEAN mgtProcessSSInfResp(MINIPORT_ADAPTER *Adapter , PUCHAR Buffer, ULONG Length);
static BOOLEAN mgtProcessAuthResp(MINIPORT_ADAPTER *Adapter , PUCHAR Buffer, ULONG Length);
static BOOLEAN mgtProcessHandoverResp(MINIPORT_ADAPTER *Adapter , PUCHAR Buffer, ULONG Length);
static BOOLEAN mgtProcessCtPacket(MINIPORT_ADAPTER *Adapter , PUCHAR Buffer, ULONG Length);
static BOOLEAN mgtProcessDmPacket(MINIPORT_ADAPTER *Adapter , PUCHAR Buffer, ULONG Length);
static LONG    mgtSwapShort2Long(SHORT Value);
//static void mgtRetryAutoConnection(unsigned long ptr); 

int mgtInit(MINIPORT_ADAPTER *Adapter )
{
	UCHAR  buf[512];
	ULONG  length;
	USHORT ix;

	ENTER;
	if (!Adapter->Mgt.Enabled) 
		return -EFAULT;
		// zero all structure
	memset(&Adapter->Mgt, 0, sizeof(WIBRO_MGT_INFO));
	Adapter->Mgt.Enabled = TRUE;
	Adapter->Mgt.isInited = TRUE;

	// prepare memory for freq list
	Adapter->Mgt.FreqList = kmalloc(CT_FREQ_MAX_NUMBER * sizeof(WIBRO_802_16_FREQUENCY) + sizeof(ULONG), GFP_ATOMIC);
    
	if (Adapter->Mgt.FreqList == NULL) {
		DumpDebug(CONN_MSG, "Can't allocate memory for frequency list");
		return -ENOMEM;
	}
//sangam dbg	
//	init_timer(&Adapter->RetryAutoConnect);	//sangam : For retry connection
//   	Adapter->RetryAutoConnect.function = mgtRetryAutoConnection; 
//	Adapter->RetryAutoConnect.data = (unsigned long) Adapter; 
//	Adapter->RetryAutoConnect.expires = jiffies + HZ * 20; //less time period during initial state
//	add_timer(&Adapter->RetryAutoConnect);
	
	// fill out state
	Adapter->Mgt.State   = Wibro802_16State_Null;
	Adapter->Mgt.PwrMode = Wibro802_16PM_Normal;
	// prepare vendor string
	strcpy(Adapter->Mgt.VendorInfo.Vendor, MINIPORT_VENDOR_DESCRIPTION);
    
	// get SS information
	memset(buf, 0, sizeof(buf));
	length = himSendSSInfoReq(buf);
	hwSendControl(Adapter , buf, length);
    
	// get IMEI
	memset(buf, 0, sizeof(buf));
	length = himSendImeiReq(buf);
	if(hwSendControl(Adapter , buf, length))
		return -ENOMEM;
	// get auth mode TODO
   
	memset(buf, 0, sizeof(buf));
	length = himSendConnect(buf);
	if(hwSendControl(Adapter , buf, length))
		return -ENOMEM;
	
	// get state
	memset(buf, 0, sizeof(buf));
	length = himSendStateReq(buf);
	if(hwSendControl(Adapter , buf, length))
		return -ENOMEM;

	// get power control mode
	memset(buf, 0, sizeof(buf));
	length = ctGetTxPwrControl(buf);
	if(hwSendControl(Adapter , buf, length))
		return -ENOMEM;
    
	// get power information (maximum power limit)
	memset(buf, 0, sizeof(buf));
	length = ctGetTxPwrLimit(buf);
	if(hwSendControl(Adapter , buf, length))
		return -ENOMEM;
    
	// set default value
	Adapter->Mgt.Antennas = 1;
    
	// get antennas
	memset(buf, 0, sizeof(buf));
	length = ctGetAntennasNumber(buf);
	if(hwSendControl(Adapter , buf, length))
		return -ENOMEM;
    
	// get frequency list
	Adapter->Mgt.FreqList->ArrCnt = CT_FREQ_MAX_NUMBER;
	for (ix = 0; ix < CT_FREQ_MAX_NUMBER; ix++) {
		// init value
		Adapter->Mgt.FreqList->Frequency[ix].FreqIndex = ix;
		Adapter->Mgt.FreqList->Frequency[ix].Frequency = 0;
		// send request
		memset(buf, 0, sizeof(buf));
		length = ctGetFrequency(buf, ix);
		if(hwSendControl(Adapter , buf, length))
			return -ENOMEM;
	}
    
    	LEAVE;
	return 0;//success??
}

void mgtRemove(MINIPORT_ADAPTER *Adapter )
{
    ENTER;
	if (!Adapter->Mgt.Enabled) 
		return;

    if (!Adapter->Mgt.isInited) 
		return;

    // free memory
    if (Adapter->Mgt.FreqList != NULL) {
		kfree(Adapter->Mgt.FreqList);
		Adapter->Mgt.FreqList = NULL;
    }

    // done
    Adapter->Mgt.isInited = FALSE;
    Adapter->Mgt.Enabled = FALSE;
//	sangam:dbg del_timer(&Adapter->RetryAutoConnect);

	LEAVE;
}
#if 0
static void mgtRetryAutoConnection(unsigned long ptr)
{
    UCHAR  buf[512];
    ULONG  length;
	MINIPORT_ADAPTER *Adapter=(struct MINIPORT_ADAPTER *)ptr;
	
	ENTER;
	if (Adapter->MediaState == MEDIA_DISCONNECTED)
	{
		DumpDebug(CONN_MSG, "Not Connected State");
		// get SS information
	    memset(buf, 0, sizeof(buf));
		length = himSendSSInfoReq(buf);
	    hwSendControl(Adapter , buf, length);
    
		// get IMEI
	    memset(buf, 0, sizeof(buf));
		length = himSendImeiReq(buf);
	    if(hwSendControl(Adapter , buf, length))
			return ;
	    // get auth mode TODO
   
	    memset(buf, 0, sizeof(buf));
 	   length = himSendConnect(buf);
 	   if(hwSendControl(Adapter , buf, length))
			return ;
	
		// get state
	    memset(buf, 0, sizeof(buf));
	    length = himSendStateReq(buf);
	    if(hwSendControl(Adapter , buf, length))
			return ;
		mod_timer(&Adapter->RetryAutoConnect, jiffies + HZ * 20);
	}
	else
	{
		DumpDebug(CONN_MSG, "Timer in Connected State");
		mod_timer(&Adapter->RetryAutoConnect, jiffies + HZ * 100);
	}

	LEAVE;
}
#endif

void mgtReceive(MINIPORT_ADAPTER *Adapter , PVOID Buffer, ULONG Length)
{

	ENTER;
    // check flags
    if (Adapter->Mgt.Enabled && Adapter->Mgt.isInited) {
			him_pkt_hdr_t* h = (him_pkt_hdr_t*)Buffer;
		// check ethernet type
		switch (h->eth_type) {
		    case ETHER_TYPE_HIM:
			{
			    BOOLEAN req = FALSE;
	
			    DumpDebug(CONN_MSG," HIM packet");
	
			    // check length
			    if (Length < sizeof(him_pkt_hdr_t)) {
					DumpDebug(CONN_MSG," Buffer is too short to contain HIM packet (%lu)", Length);
					return;
			    }
			    // check HIM type
			    switch (h->him_type) {
					case HIM_TYPE_LINK   :
					    DumpDebug(CONN_MSG," LINK packet");
					    req = mgtProcessLinkResp(Adapter , Buffer, Length);
					    break;
	
					case HIM_TYPE_POWER  :
					    DumpDebug(CONN_MSG," POWER packet");
					    req = mgtProcessPowerResp(Adapter , Buffer, Length);
			  			break;

					case HIM_TYPE_SS_INFO:
					    DumpDebug(CONN_MSG," SS INFO packet");
					    req = mgtProcessSSInfResp(Adapter , Buffer, Length);
					    break;

					case HIM_TYPE_AUTH   :
					    DumpDebug(CONN_MSG," AUTH packet");
					    req = mgtProcessAuthResp(Adapter , Buffer, Length);
					    break;

					case HIM_TYPE_HANDOVER:
					    DumpDebug(CONN_MSG," HANDOVER packet");
					    req = mgtProcessHandoverResp(Adapter , Buffer, Length);
					    break;

					default:
					    // do nothing
					    DumpDebug(CONN_MSG," Other HIM packet type (%x)", h->him_type);
			    }
			    // exit, we intercepted packet
			    return;
			}
		    case ETHER_TYPE_CT:
			{
			    BOOLEAN req = FALSE;

			    DumpDebug(CONN_MSG," CT packet");
			    req = mgtProcessCtPacket(Adapter , Buffer, Length);
			    // exit, we intercepted packet
			    return;
			}
		    case ETHER_TYPE_DM:
			{
			    BOOLEAN req = FALSE;

			    DumpDebug(CONN_MSG," DM packet");
			    req = mgtProcessDmPacket(Adapter , Buffer, Length);
			    // exit, we intercepted packet
			    return;
			}
		    default:
			// do nothing
			DumpDebug(CONN_MSG," Other Ethernet packet type (%x)", h->eth_type); 
		}
    }
	LEAVE;
}

static BOOLEAN mgtProcessLinkResp(MINIPORT_ADAPTER *Adapter , PUCHAR Buffer, ULONG Length)
{
    UCHAR buf[512];
    ULONG length = 0;
    him_pkt_hdr_t* h = (him_pkt_hdr_t*)Buffer;

	ENTER;
    switch (h->subtype) {
		case HIM_LINK_UP_ACK	 :
			DumpDebug(CONN_MSG," LINK UP ACK");
			// check state
	
			switch (Adapter->Mgt.State) {
				case Wibro802_16State_Null:
				    	DumpDebug(CONN_MSG," SYNK UP OK");
		    
					// send link info req
					length = himSendStartLinkInfo(buf);
					hwSendControl(Adapter , buf, length);
				
					// get other information (start DM)
					length = dmSendReportPeriodReq(buf, &Adapter->Mgt.DmSeqNum);
					hwSendControl(Adapter , buf, length);
				    
					length = dmSendStartReq(buf, &Adapter->Mgt.DmSeqNum);
					hwSendControl(Adapter , buf, length);
			    
					// set state
					Adapter->Mgt.State = Wibro802_16State_Standby;
					break;
		
				case Wibro802_16State_NetEntry	     :
				case Wibro802_16State_NetEntry_Ranging:
				case Wibro802_16State_NetEntry_SBC    :
				case Wibro802_16State_NetEntry_PKM    :
				case Wibro802_16State_NetEntry_REG    :
				case Wibro802_16State_NetEntry_DSX    :
					DumpDebug(CONN_MSG," CONNECT OK");

					// indicate connect
					if (Adapter->MediaState == MEDIA_DISCONNECTED) {
						// set value to structure
						Adapter->MediaState = MEDIA_CONNECTED;
					}
					// set state
					Adapter->Mgt.State = Wibro802_16State_Awake;
					break;

				default:
			    // do nothing
			    DumpDebug(CONN_MSG," Wrong state %d", Adapter->Mgt.State);
			}
	    break;

		case HIM_LINK_UP_NOTIFY	 :
		    {
			him_ntfy_link_up_t* n = (him_ntfy_link_up_t*)(Buffer + sizeof(him_pkt_hdr_t));

			DumpDebug(CONN_MSG," LINK UP NOTIFICATION");
		
			if (be16_to_cpu(h->length) != sizeof(him_ntfy_link_up_t)) 
				return FALSE;

			switch (n->res) { // type
				case HIM_LINK_UP_SYNC:
					DumpDebug(CONN_MSG," SYNC UP");
			
					// send link info req
					length = himSendStartLinkInfo(buf);
					hwSendControl(Adapter , buf, length);
		
					// get other information (start DM)
					length = dmSendReportPeriodReq(buf, &Adapter->Mgt.DmSeqNum);
					hwSendControl(Adapter , buf, length);
		
					length = dmSendStartReq(buf, &Adapter->Mgt.DmSeqNum);
					hwSendControl(Adapter , buf, length);
		
					// set state
					Adapter->Mgt.State   = Wibro802_16State_Standby;
					// wake up
					Adapter->Mgt.PwrMode = Wibro802_16PM_Normal;
					break;

				case HIM_LINK_UP_NET :
					DumpDebug(CONN_MSG," NET CONNECT");
					// indicate connect
					if (Adapter->MediaState == MEDIA_DISCONNECTED) {
					    // set value to structure
					    Adapter->MediaState = MEDIA_CONNECTED;
					}
					// set state
					Adapter->Mgt.State = Wibro802_16State_Awake;
					// wake up
					Adapter->Mgt.PwrMode = Wibro802_16PM_Normal;
					break;

		    	default:
					// do nothing
					DumpDebug(CONN_MSG," Unknown link type = %d", n->res);
			}
			break;
	    }

		case HIM_LINK_DOWN_ACK	 :
			DumpDebug(CONN_MSG," LINK DOWN ACK");
		    // check state
	
			switch (Adapter->Mgt.State) {
				case Wibro802_16State_Standby  :
				    DumpDebug(CONN_MSG," DISCONNECT OK");
				    // set state
				    Adapter->Mgt.State = Wibro802_16State_OutOfZone;
				    break;

				case Wibro802_16State_OutOfZone:
				    DumpDebug(CONN_MSG," SYNC DOWN OK");
				    // set state
				    Adapter->Mgt.State = Wibro802_16State_Null;
				    break;

				default:
				    // do nothing
					DumpDebug(CONN_MSG," Wrong state %d", Adapter->Mgt.State);
	    	}
	    	break;

		case HIM_LINK_DOWN_NOTIFY:
		{
			him_ntfy_link_down_t* n = (him_ntfy_link_down_t*)(Buffer + sizeof(him_pkt_hdr_t));

			DumpDebug(CONN_MSG," LINK DOWN NOTIFICATION");
			if (be16_to_cpu(h->length) != sizeof(him_ntfy_link_down_t)) 
				return FALSE;
		
			// check type
			switch (n->res) { // type
				case HIM_LINK_DOWN_SYNC:
					DumpDebug(CONN_MSG," SYNC DOWN");
		
					// stop link info
					length = himSendStopLinkInfo(buf);
					hwSendControl(Adapter , buf, length);
	
					// stop DM
					length = dmSendStopReq(buf, &Adapter->Mgt.DmSeqNum);
					hwSendControl(Adapter , buf, length);
		
					// set state
					Adapter->Mgt.State = Wibro802_16State_Null;
					break;

				case HIM_LINK_DOWN_NET :
					DumpDebug(CONN_MSG," NETWORK DEREG");
		
					// indicate disconnect
					if (Adapter->MediaState == MEDIA_CONNECTED) {
					    // set value to structure
					    Adapter->MediaState = MEDIA_DISCONNECTED;
					}
					// set state
					Adapter->Mgt.State = Wibro802_16State_OutOfZone;
					break;

			    default:
					// do nothing
					DumpDebug(CONN_MSG," Unknown link type = %d", n->res);
			}
			break;
	    }

		case HIM_LINK_UP_NAK	 :
		{
#ifndef WIBRO_USB_WHQL
			him_res_link_nak_t* n = (him_res_link_nak_t*)(Buffer + sizeof(him_pkt_hdr_t));
#endif
			DumpDebug(CONN_MSG," LINK UP NAK");
			if (be16_to_cpu(h->length) != sizeof(him_res_link_nak_t)) 
				return FALSE;

			// check state
			if (Adapter->Mgt.State > Wibro802_16State_Awake) {
			    // revert state back
			    Adapter->Mgt.State = Wibro802_16State_OutOfZone;
			}
#ifndef WIBRO_USB_WHQL
			switch (n->res) { // code
			    case HIM_LINK_FAIL_PHY_SYNC	: DumpDebug(CONN_MSG, " PHY SYNC fail"); break;
			    case HIM_LINK_FAIL_MAC_SYNC	: DumpDebug(CONN_MSG, " MAC SYNC fail"); break;
			    case HIM_LINK_FAIL_RNG	: DumpDebug(CONN_MSG, " RNG fail"); break;
			    case HIM_LINK_FAIL_SBC	: DumpDebug(CONN_MSG, " SBC fail"); break;
			    case HIM_LINK_FAIL_PKM	: DumpDebug(CONN_MSG, " PKM fail"); break;
			    case HIM_LINK_FAIL_REG	: DumpDebug(CONN_MSG, " REG fail"); break;
			    case HIM_LINK_FAIL_DSX	: DumpDebug(CONN_MSG, " DSX fail"); break;
			    case HIM_LINK_FAIL_TIMEOUT	: DumpDebug(CONN_MSG, " Timeout"); break;
			    default: DumpDebug(CONN_MSG," Other code = %x", n->res);
			}
#endif
			// indicate disconnect
			if (Adapter->MediaState == MEDIA_CONNECTED) {
		  		// set value to structure
		    	Adapter->MediaState = MEDIA_DISCONNECTED;
				netif_stop_queue(Adapter->net);
			}
			break;
	    }

		case HIM_LINK_DOWN_NAK	 :
	    {
			him_res_link_nak_t* n = (him_res_link_nak_t*)(Buffer + sizeof(him_pkt_hdr_t));

			DumpDebug(CONN_MSG," LINK DOWN NAK, code = %x", n->res);

			// check state
			if (Adapter->Mgt.State == Wibro802_16State_Standby) {
			    // return previous state
			    Adapter->Mgt.State = Wibro802_16State_Awake;
			}
			break;
	    }

		case HIM_LINK_INFO_RESP	 :
	    {
			ULONG j;
		//	float r_avg, c_avg, r_dev, c_dev;	//sangam: need library??
			long r_avg, c_avg, r_dev, c_dev;
			him_res_link_info_t* i = (him_res_link_info_t*)(Buffer + sizeof(him_pkt_hdr_t));

			DumpDebug(CONN_MSG," LINK INFO");
			if (be16_to_cpu(h->length) != sizeof(him_res_link_info_t)) 
				return FALSE;

			// save power level
			Adapter->Mgt.TxPwrInfo.TxPower = mgtSwapShort2Long(i->TxPower);
			Adapter->Mgt.TxPwrInfo.TxPowerHeadroom = Adapter->Mgt.TxPwrInfo.TxPowerMaximum - Adapter->Mgt.TxPwrInfo.TxPower;
	
			// save RSSI
			Adapter->Mgt.RfInfo.PrimaryRssi = mgtSwapShort2Long(i->Rssi);
			Adapter->Mgt.Rssi[Adapter->Mgt.InfIx] = Adapter->Mgt.RfInfo.PrimaryRssi;
	
			// save CINR
			Adapter->Mgt.RfInfo.PrimaryCinr = mgtSwapShort2Long(i->Cinr) / 8;
			Adapter->Mgt.Cinr[Adapter->Mgt.InfIx] = Adapter->Mgt.RfInfo.PrimaryCinr;
	
			// calculate RSSI deviation
			r_avg = c_avg = r_dev = c_dev = 0;
			for (j = 0; j < WIBRO_USB_DEV_CALC_ARRAY_SZ; j++) {
			    // average RSSI
			    r_avg += Adapter->Mgt.Rssi[j];
			    // deviation RSSI
			    r_dev += (Adapter->Mgt.Rssi[j] * Adapter->Mgt.Rssi[j]);
			    // average CINR
			    c_avg += Adapter->Mgt.Cinr[j];
			    // deviation CINR
			    c_dev += (Adapter->Mgt.Cinr[j] * Adapter->Mgt.Cinr[j]);
			}
		
			// calc averages
			r_avg = r_avg / WIBRO_USB_DEV_CALC_ARRAY_SZ;
			c_avg = c_avg / WIBRO_USB_DEV_CALC_ARRAY_SZ;

			// calc deviation
			r_dev = (r_dev / WIBRO_USB_DEV_CALC_ARRAY_SZ) - (r_avg * r_avg);
//			r_dev = (float)sqrt(r_dev);
			r_dev = int_sqrt(r_dev);	//check for float sqare :sangam
			c_dev = (c_dev / WIBRO_USB_DEV_CALC_ARRAY_SZ) - (c_avg * c_avg);
//			c_dev = (float)sqrt(c_dev);
			c_dev = int_sqrt(c_dev);	//sangam: check later for the float sqaue
	
			// save deviations
			memcpy(&Adapter->Mgt.RfInfo.PrimaryRssiDeviation, &r_dev, 4);
			memcpy(&Adapter->Mgt.RfInfo.PrimaryCinrDeviation, &c_dev, 4);
		
			// increase index
			Adapter->Mgt.InfIx = (Adapter->Mgt.InfIx + 1) % WIBRO_USB_DEV_CALC_ARRAY_SZ;
			// save preamble index
	
			if (Adapter->Mgt.BsInfo.CurrentPreambleIndex != be16_to_cpu(i->PreIndex)) {
		   		Adapter->Mgt.BsInfo.PreviousPreambleIndex = Adapter->Mgt.BsInfo.CurrentPreambleIndex;
		    	Adapter->Mgt.BsInfo.CurrentPreambleIndex  = be16_to_cpu(i->PreIndex);
			}
		
			// save BSID
			memcpy(Adapter->Mgt.BsInfo.BSID, i->Bsid, 6);
			DumpDebug(CONN_MSG," BSID: %02x:%02x:%02x:%02x:%02x:%02x, RSSI: %lu, CINR: %lu, TX POWER: %lu", 
			    Adapter->Mgt.BsInfo.BSID[0], Adapter->Mgt.BsInfo.BSID[1], Adapter->Mgt.BsInfo.BSID[2], 
			    Adapter->Mgt.BsInfo.BSID[3], Adapter->Mgt.BsInfo.BSID[4], Adapter->Mgt.BsInfo.BSID[5],
			    Adapter->Mgt.RfInfo.PrimaryRssi, Adapter->Mgt.RfInfo.PrimaryCinr, Adapter->Mgt.TxPwrInfo.TxPower);
			break;
	    }

		case HIM_LINK_STATE_RESP :
	    {
			him_res_link_state_t* s = (him_res_link_state_t*)(Buffer + sizeof(him_pkt_hdr_t));

			DumpDebug(CONN_MSG," STATE");
			if (be16_to_cpu(h->length) != sizeof(him_res_link_state_t)) 
				return FALSE;

			switch (s->res) { // state
			    case HIM_LINK_STATE_INIT: 
					DumpDebug(CONN_MSG," INIT");
					Adapter->Mgt.State   = Wibro802_16State_Null; 
					Adapter->Mgt.PwrMode = Wibro802_16PM_Normal;
					break;

			    case HIM_LINK_STATE_SYNC: 
					DumpDebug(CONN_MSG," SYNC");
					Adapter->Mgt.State   = Wibro802_16State_Null; 
					Adapter->Mgt.PwrMode = Wibro802_16PM_Normal;
					break;

			    case HIM_LINK_STATE_NORMAL: 
					DumpDebug(CONN_MSG," NORMAL");
					Adapter->Mgt.State   = Wibro802_16State_Awake;
					Adapter->Mgt.PwrMode = Wibro802_16PM_Normal;
					break;

			    case HIM_LINK_STATE_IDLE: 
					DumpDebug(CONN_MSG," IDLE");
					Adapter->Mgt.PwrMode = Wibro802_16PM_Idle; 
					break;

			    case HIM_LINK_STATE_SLEEP: 
					DumpDebug(CONN_MSG," SLEEP");
					Adapter->Mgt.PwrMode = Wibro802_16PM_Sleep; 
					break;

			    case HIM_LINK_STATE_ENTRY: 
					DumpDebug(CONN_MSG," NETWORK ENTRY");
					Adapter->Mgt.State   = Wibro802_16State_NetEntry;
					Adapter->Mgt.PwrMode = Wibro802_16PM_Normal;
					break;
		    
			    default:
					// do nothing
					DumpDebug(CONN_MSG," Unknown state = %x", s->res);
			}
			break;
	    }

		case HIM_LINK_STATE_NTFY :
	    {
			him_ntfy_link_state_t* s = (him_ntfy_link_state_t*)(Buffer + sizeof(him_pkt_hdr_t));

			DumpDebug(CONN_MSG," STATE NOTIFICATION");
			if (be16_to_cpu(h->length) != sizeof(him_ntfy_link_state_t)) 
				return FALSE;

			switch (s->state) {
			    case HIM_LINK_ENTRY_RANGING	:
					DumpDebug(CONN_MSG," RANGING");
					Adapter->Mgt.State   = Wibro802_16State_NetEntry_Ranging;
					Adapter->Mgt.PwrMode = Wibro802_16PM_Normal;
					break;

			    case HIM_LINK_ENTRY_SBC	:
					DumpDebug(CONN_MSG," SBC");
					Adapter->Mgt.State   = Wibro802_16State_NetEntry_SBC;
					Adapter->Mgt.PwrMode = Wibro802_16PM_Normal;
					break;

			    case HIM_LINK_ENTRY_PKM	:
					DumpDebug(CONN_MSG," PKM");
					Adapter->Mgt.State   = Wibro802_16State_NetEntry_PKM;
					Adapter->Mgt.PwrMode = Wibro802_16PM_Normal;
					break;

			    case HIM_LINK_ENTRY_REG	:
					DumpDebug(CONN_MSG," REG");
					Adapter->Mgt.State   = Wibro802_16State_NetEntry_REG;
					Adapter->Mgt.PwrMode = Wibro802_16PM_Normal;
					break;

			    case HIM_LINK_ENTRY_DSX	:
					DumpDebug(CONN_MSG," DSX");
					Adapter->Mgt.State   = Wibro802_16State_NetEntry_DSX;
					Adapter->Mgt.PwrMode = Wibro802_16PM_Normal;
					break;

			    default:
					// do nothing
					DumpDebug(CONN_MSG," Unknown state = %x", s->state);
			}
			break;
	    }

		default:
		    // do nothing
		    DumpDebug(CONN_MSG," Other subtype (%x)", h->subtype);
	}

	LEAVE;
    return (length ? TRUE : FALSE);
}

static BOOLEAN mgtProcessPowerResp(MINIPORT_ADAPTER *Adapter , PUCHAR Buffer, ULONG Length)
{
    WIBRO_802_16_POWER_MODE_INDICATION i;
    him_pkt_hdr_t* h = (him_pkt_hdr_t*)Buffer;

	ENTER;
    i.Type = Wibro802_16StatusType_PowerMode;

    switch (h->subtype) {
		case HIM_POWER_SLEEP_NTFY:
		    DumpDebug(CONN_MSG," SLEEP notification");
		    // set sleep mode
		    Adapter->Mgt.PwrMode = i.PowerMode = Wibro802_16PM_Sleep;
		    break;

		case HIM_POWER_IDLE_NTFY :
		    DumpDebug(CONN_MSG," IDLE notification");
		    // set idle mode
		    Adapter->Mgt.PwrMode = i.PowerMode = Wibro802_16PM_Idle;
		    break;

		default:
		    // do nothing
		    DumpDebug(CONN_MSG," Other subtype (%x)", h->subtype);
		    return FALSE;
	}
	LEAVE;
    return FALSE;
}

static BOOLEAN mgtProcessSSInfResp(MINIPORT_ADAPTER *Adapter , PUCHAR Buffer, ULONG Length)
{
    him_pkt_hdr_t* h = (him_pkt_hdr_t*)Buffer;

	ENTER;
    
	if (h->subtype == HIM_SS_INFO_RES) {
		him_res_ss_info_t* i = (him_res_ss_info_t*)(Buffer + sizeof(him_pkt_hdr_t));
		DumpDebug(CONN_MSG," SS INFO response");
	
		if (be16_to_cpu(h->length) == sizeof(him_res_ss_info_t)) {
		    DumpDebug(CONN_MSG," PHY = %s, MAC = %s", i->PhyVer, i->MacVer);
		    // copy to internal storage
		    strncpy(Adapter->Mgt.VendorInfo.PhyVer, i->PhyVer, 32);
		    strncpy(Adapter->Mgt.VendorInfo.MacVer, i->MacVer, 32);
		}
    }

	LEAVE; 
    return FALSE;
}

static BOOLEAN mgtProcessAuthResp(MINIPORT_ADAPTER *Adapter , PUCHAR Buffer, ULONG Length)
{
    UCHAR buf[512];
    ULONG length = 0;
    him_pkt_hdr_t* h = (him_pkt_hdr_t*)Buffer;

	ENTER;
    switch (h->subtype) {
		case HIM_AUTH_REQ :
		    DumpDebug(CONN_MSG," AUTH REQ");
	
			if (Adapter->Mgt.MskReady) { 
				// send response
				length = himSendAuthResp(buf, Adapter->Mgt.EapMsk, sizeof(WIBRO_802_16_AUTH_EAP_MSK));
		    }
		    else { 
				DumpDebug(CONN_MSG," MSK not ready, wait");
				Adapter->Mgt.KeyRequested = TRUE;
		    }
		    break;

		case HIM_AUTH_KEY_RESP :
	    {
			him_res_auth_rdwr_t* r = (him_res_auth_rdwr_t*)(Buffer + sizeof(him_pkt_hdr_t));

			DumpDebug(CONN_MSG," KEY RESPONSE");
			if (be16_to_cpu(h->length) != (sizeof(him_res_auth_rdwr_t) + sizeof(WIBRO_802_16_IMEI))) { 
  		  		DumpDebug(CONN_MSG,"Length mismatch");
				break;
			}

			// get IMEI
			if ((r->oper == HIM_AUTH_READ) && (r->type == HIM_AUTH_IMEI)) {
				DumpDebug(CONN_MSG," KEY copied");
			    memmove(Adapter->Mgt.Imei, Buffer + sizeof(him_pkt_hdr_t) + sizeof(him_res_auth_rdwr_t), sizeof(WIBRO_802_16_IMEI));
			}
			break;
	    }

		default :
		    // do nothing
		    DumpDebug(CONN_MSG," Other subtype (%x)", h->subtype);
	}

	DumpDebug(CONN_MSG,"--");
	return (length ? TRUE: FALSE);
}

static BOOLEAN mgtProcessHandoverResp(MINIPORT_ADAPTER *Adapter , PUCHAR Buffer, ULONG Length)
{
    WIBRO_802_16_HAND_OVER_INDICATION i;
    him_pkt_hdr_t* h = (him_pkt_hdr_t*)Buffer;

	ENTER;
    if (h->subtype == HIM_HANDOVER_NTFY) {
		DumpDebug(CONN_MSG," HANDOVER NOTIFICATION");
		// save prev. preamble index
		Adapter->Mgt.BsInfo.PreviousPreambleIndex = Adapter->Mgt.BsInfo.CurrentPreambleIndex;
		// fill out structure
		i.Type        = Wibro802_16StatusType_Handover;
		i.DhcpRequest = FALSE;
		memmove(i.BSID, Adapter->Mgt.BsInfo.BSID, sizeof(WIBRO_802_16_MAC_ADDRESS));
    }

	LEAVE;
    return FALSE;
}

static BOOLEAN mgtProcessCtPacket(MINIPORT_ADAPTER *Adapter , PUCHAR Buffer, ULONG Length)
{
    ct_pkt_hdr_t* h = (ct_pkt_hdr_t*)Buffer;

	ENTER;
    
	// check length
    if (Length < sizeof(ct_pkt_hdr_t)) {
		DumpDebug(CONN_MSG," Buffer is too short to contain CT packet (%lu)", Length);
		return FALSE;
    }
    
	// check code
    if (h->code != CT_CODE_DATA) {
		DumpDebug(CONN_MSG, " Not data packet (%02x)", h->code);
		return FALSE;
    }
    
	// check antennas
    if ((h->g_id == CT_GID_RF_CFG) && (h->m_id == CT_MID_ANTENNA)) {
		DumpDebug(CONN_MSG," CT_MID_ANTENNA");
	
		if (h->length == 0x0100) { // 1 byte
			PUCHAR b = (PUCHAR)(Buffer + sizeof(ct_pkt_hdr_t));
			// save 
			Adapter->Mgt.Antennas = *b;
			DumpDebug(CONN_MSG," Antennas = %d", *b);
		}
#ifndef WIBRO_USB_WHQL
		else {
			DumpDebug(CONN_MSG," Wrong length (%d)", h->length);
		}
#endif
		return FALSE;
	}

	// check G_ID - only MAC_CFG is supported
	if (h->g_id != CT_GID_MAC_CFG) {
		DumpDebug(CONN_MSG," Not correct G_ID (%02x)", h->g_id);
		return FALSE;
	}
	
	// check M_ID
	switch (h->m_id) {
		case CT_MID_TX_PWR_CONTROL:
			DumpDebug(CONN_MSG," CT_MID_TX_PWR_CONTROL");
			if (h->length == 0x0400) { // 4 bytes
				PULONG u = (PULONG)(Buffer + sizeof(ct_pkt_hdr_t));
				// save
				if (*u) {
					Adapter->Mgt.TxPwrControl = Wibro802_16TxPowerControl_Auto;
					DumpDebug(CONN_MSG," Auto Power Control");
				}
				else {
					Adapter->Mgt.TxPwrControl = Wibro802_16TxPowerControl_Fixed;
					DumpDebug(CONN_MSG," Fixed Power Control");
				}
			}
#ifndef WIBRO_USB_WHQL
			else {
				DumpDebug(CONN_MSG," Wrong length (%d)", h->length);
			}
#endif
			break;

			case CT_MID_TX_PWR_LIMIT:
				DumpDebug(CONN_MSG," CT_MID_TX_PWR_LIMIT");
				if (h->length == 0x0100) { // 1 byte
					PUCHAR b = (PUCHAR)(Buffer + sizeof(ct_pkt_hdr_t));
					// save 
					Adapter->Mgt.TxPwrInfo.TxPowerMaximum = *b;
					DumpDebug(CONN_MSG," TxMaxPower = %d", *b);
				}
#ifndef WIBRO_USB_WHQL
				else {
					DumpDebug(CONN_MSG," Wrong length (%d)", h->length);
				}
#endif
				break;

			case CT_MID_FREQ_0:
				DumpDebug(CONN_MSG," CT_MID_FREQ_0");
				if (h->length == 0x0400) { // 4 bytes
					PULONG u = (PULONG)(Buffer + sizeof(ct_pkt_hdr_t));
					// save
					Adapter->Mgt.FreqList->Frequency[0].Frequency = be32_to_cpu(*u);
					DumpDebug(CONN_MSG," Frequency 0 = %u MHz", be32_to_cpu(*u));
				}
#ifndef WIBRO_USB_WHQL
			    else {
					DumpDebug(CONN_MSG," Wrong length (%d)", h->length);
			    }
#endif
			    break;

		case CT_MID_FREQ_1:
		    DumpDebug(CONN_MSG, " CT_MID_FREQ_1");
		    if (h->length == 0x0400) { // 4 bytes
				PULONG u = (PULONG)(Buffer + sizeof(ct_pkt_hdr_t));
				// save
				Adapter->Mgt.FreqList->Frequency[1].Frequency = be32_to_cpu(*u);
				DumpDebug(CONN_MSG, " Frequency 1 = %u MHz", be32_to_cpu(*u));
		    }
#ifndef WIBRO_USB_WHQL
		    else {
				DumpDebug(CONN_MSG, " Wrong length (%d)", h->length);
			}
#endif
			break;

		case CT_MID_FREQ_2:
			    DumpDebug(CONN_MSG, " CT_MID_FREQ_2");
			    if (h->length == 0x0400) { // 4 bytes
					PULONG u = (PULONG)(Buffer + sizeof(ct_pkt_hdr_t));
					// save
					Adapter->Mgt.FreqList->Frequency[2].Frequency = be32_to_cpu(*u);
					DumpDebug(CONN_MSG, " Frequency 2 = %u MHz", be32_to_cpu(*u));
			    }
#ifndef WIBRO_USB_WHQL
			    else {
					DumpDebug(CONN_MSG, " Wrong length (%d)", h->length);
			    }
#endif
			    break;

		case CT_MID_FREQ_3:
		    DumpDebug(CONN_MSG, " CT_MID_FREQ_3");
		    if (h->length == 0x0400) { // 4 bytes
				PULONG u = (PULONG)(Buffer + sizeof(ct_pkt_hdr_t));
				// save
				Adapter->Mgt.FreqList->Frequency[3].Frequency = be32_to_cpu(*u);
				DumpDebug(CONN_MSG, " Frequency 3 = %u MHz", be32_to_cpu(*u));
			}
#ifndef WIBRO_USB_WHQL
			else {
				DumpDebug(CONN_MSG, " Wrong length (%d)", h->length);
			}
#endif
			break;

		case CT_MID_FREQ_4:
			DumpDebug(CONN_MSG, " CT_MID_FREQ_4");
			if (h->length == 0x0400) { // 4 bytes
				PULONG u = (PULONG)(Buffer + sizeof(ct_pkt_hdr_t));
				// save
				Adapter->Mgt.FreqList->Frequency[4].Frequency = be32_to_cpu(*u);
				DumpDebug(CONN_MSG, " Frequency 4 = %u MHz", be32_to_cpu(*u));
			}
#ifndef WIBRO_USB_WHQL
			else {
				DumpDebug(CONN_MSG, " Wrong length (%d)", h->length);
			}
#endif
			break;

		case CT_MID_FREQ_5:
			DumpDebug(CONN_MSG, " CT_MID_FREQ_5");
			if (h->length == 0x0400) { // 4 bytes
				PULONG u = (PULONG)(Buffer + sizeof(ct_pkt_hdr_t));
				// save
				Adapter->Mgt.FreqList->Frequency[5].Frequency = be32_to_cpu(*u);
				DumpDebug(CONN_MSG, " Frequency 5 = %u MHz", be32_to_cpu(*u));
			}
#ifndef WIBRO_USB_WHQL
			else {
				DumpDebug(CONN_MSG, " Wrong length (%d)", h->length);
			}
#endif
			break;

		case CT_MID_FREQ_6:
			DumpDebug(CONN_MSG," CT_MID_FREQ_6");
			if (h->length == 0x0400) { // 4 bytes
				PULONG u = (PULONG)(Buffer + sizeof(ct_pkt_hdr_t));
				// save
				Adapter->Mgt.FreqList->Frequency[6].Frequency = be32_to_cpu(*u);
				DumpDebug(CONN_MSG," Frequency 6 = %u MHz", be32_to_cpu(*u));
			}
#ifndef WIBRO_USB_WHQL
			else {
				DumpDebug(CONN_MSG," Wrong length (%d)", h->length);
			}
#endif
			break;

		case CT_MID_FREQ_7:
			DumpDebug(CONN_MSG," CT_MID_FREQ_7");
			if (h->length == 0x0400) { // 4 bytes
				PULONG u = (PULONG)(Buffer + sizeof(ct_pkt_hdr_t));
				// save
				Adapter->Mgt.FreqList->Frequency[7].Frequency = be32_to_cpu(*u);
				DumpDebug(CONN_MSG," Frequency 7 = %u MHz", be32_to_cpu(*u));
			}
#ifndef WIBRO_USB_WHQL
			else {
				DumpDebug(CONN_MSG, " Wrong length (%d)", h->length);
			}
#endif
		    break;

		case CT_MID_FREQ_8:
		    DumpDebug(CONN_MSG," CT_MID_FREQ_8");
		    if (h->length == 0x0400) { // 4 bytes
				PULONG u = (PULONG)(Buffer + sizeof(ct_pkt_hdr_t));
				// save
				Adapter->Mgt.FreqList->Frequency[8].Frequency = be32_to_cpu(*u);
				DumpDebug(CONN_MSG," Frequency 8 = %u MHz", be32_to_cpu(*u));
			}
#ifndef WIBRO_USB_WHQL
			else {
				DumpDebug(CONN_MSG, " Wrong length (%d)", h->length);
			}
#endif
	    break;

		default:
			DumpDebug(CONN_MSG," Other M_ID = %04x", h->m_id);
	}

	LEAVE;
    return FALSE;
}

static BOOLEAN mgtProcessDmPacket(MINIPORT_ADAPTER *Adapter , PUCHAR Buffer, ULONG Length)
{
    dm_pkt_hdr_t* h = (dm_pkt_hdr_t*)Buffer;

	ENTER;
    // check length
	if (Length < sizeof(dm_pkt_hdr_t)) {
		DumpDebug(CONN_MSG," Buffer is too short to contain DM packet (%lu)", Length);
		return FALSE;
	}

    // check command
	switch (h->cmd) {
		case DM_CMD_START_CNF	 :
		case DM_CMD_STOP_CNF	 :
		case DM_CMD_TFREQ_CNG_CNF:
		    DumpDebug(CONN_MSG," Command (%02x) confirmed", h->cmd);
		    break;

		case DM_CMD_INFO_HO:
		{
			PULONG lt = (PULONG)(Buffer + sizeof(dm_pkt_hdr_t));
	
			DumpDebug(CONN_MSG," HO information (latency = %lu)", *lt);
			// save
			Adapter->Mgt.BsInfo.HOSignalLatency = be32_to_cpu(*lt);
			break;
		}
		case DM_CMD_INFO_STATUS:
		{
			ULONG		 ix;
			dm_basic_info_t* inf = (dm_basic_info_t*)(Buffer + sizeof(dm_pkt_hdr_t));

			DumpDebug(CONN_MSG," Basic information (%lu, %d)", Length, sizeof(dm_basic_info_t));
			// save stats
			Adapter->Mgt.Stats.HandoverFailCount	+= be32_to_cpu(inf->HOCntFail) + be32_to_cpu(inf->PingPongCnt);
			Adapter->Mgt.Stats.HandoverCount	+= be32_to_cpu(inf->HOCntTry) - Adapter->Mgt.Stats.HandoverFailCount;
			Adapter->Mgt.Stats.TotalRxPacketCount	+= be32_to_cpu(inf->DL_NumPDU);
			Adapter->Mgt.Stats.TotalRxPacketLength	= Adapter->stats.rx.Bytes;
			Adapter->Mgt.Stats.TotalTxPacketCount	+= be32_to_cpu(inf->UL_NumPDU);
			Adapter->Mgt.Stats.TotalTxPacketLength	= Adapter->stats.tx.Bytes;
			Adapter->Mgt.Stats.TxRetryCount		= 0;
			Adapter->Mgt.Stats.RxErrorCount		= 0;
			Adapter->Mgt.Stats.RxMissingCount	+= inf->Discarded;
			Adapter->Mgt.Stats.ResyncCount		= 0;		
			// save BS information
			Adapter->Mgt.BsInfo.ULPermBase = inf->UL_PermBase;
			Adapter->Mgt.BsInfo.DLPermBase = inf->PreambleIndex % 32;
			// save RF information
			Adapter->Mgt.RfInfo.DiversityRssi	    = 0;
			Adapter->Mgt.RfInfo.DiversityRssiDeviation  = 0;
			Adapter->Mgt.RfInfo.DiversityCinr	    = 0;
			Adapter->Mgt.RfInfo.DiversityCinrDeviation  = 0;
	
			switch (inf->PwrCtlMode) {
			    case DM_PWR_CTL_MODE_CLOSED	: Adapter->Mgt.RfInfo.PowerControlMode = Wibro802_16PowerControlMode_ClosedLoop; break;
			    case DM_PWR_CTL_MODE_OPEN_RS:
			    case DM_PWR_CTL_MODE_OPEN_RT: Adapter->Mgt.RfInfo.PowerControlMode = Wibro802_16PowerControlMode_OpenLoopPassive; break;
			    case DM_PWR_CTL_MODE_ACTIVE	: Adapter->Mgt.RfInfo.PowerControlMode = Wibro802_16PowerControlMode_OpenLoopActive; break;
			    default: Adapter->Mgt.RfInfo.PowerControlMode = Wibro802_16PowerControlMode_Reserved;
			}
	
			Adapter->Mgt.RfInfo.PERReceiveCount += be32_to_cpu(inf->ErrCRC) + be32_to_cpu(inf->DL_NumPDU);
			Adapter->Mgt.RfInfo.PERErrorCount   += be32_to_cpu(inf->ErrCRC);
			// DL burst scheme
			for (ix = 0; ix < 13; ix++) {
			    if (inf->DL_FECType[ix] != 0xff) {
					Adapter->Mgt.RfInfo.DLBurstDataFECScheme = inf->DL_FECType[ix];
					break;
				}
			}
	
			// UL burst scheme
			for (ix = 0; ix < 10; ix++) {
			    if (inf->UL_FECType[ix] != 0xff) {
					Adapter->Mgt.RfInfo.ULBurstDataFECScheme = inf->UL_FECType[ix];
					break;
			    }
			}
	
			Adapter->Mgt.RfInfo.ULBurstDataUIUC = 0;
			Adapter->Mgt.RfInfo.DLBurstDataUIUC = 0;
			
			// save current frequency index
			for (ix = 0; ix < CT_FREQ_MAX_NUMBER; ix++) {
			    if (Adapter->Mgt.FreqList->Frequency[ix].Frequency == be32_to_cpu(inf->CurFreq)) {
					// found
					Adapter->Mgt.FreqSelected = ix;
					break;
			    }
			}
			break;
		}
		default  :
		    DumpDebug(CONN_MSG," Other command = %02x", h->cmd);
	}

	LEAVE;
    return FALSE;
}

static LONG mgtSwapShort2Long(SHORT Value)
{
    LONG  l = 0;
    SHORT s = be16_to_cpu(Value);

    ENTER; 
    // check value
    if (s < 0) {
		l  = -1;
		l &= 0xffff0000;
		l |= s;
	}
    else { // positive or nul
		l = s;
    }

    LEAVE;
    return l;
}
#endif
