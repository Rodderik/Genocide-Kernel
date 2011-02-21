#ifndef _WIBRO_SDIO_H
#define _WIBRO_SDIO_H

// sangam: power msg is a control message now and handled by wcm
//#define WIBRO_USB_SELECTIVE_SUSPEND		 		

// WHQL tests
#define WIBRO_SDIO_WHQL

// WCM link indication
#define CONTROl_USE_WCM_LINK_INDICATION
                                     
#define	RX_SKBS			4
#define WIBRO_USB_DEV_CALC_ARRAY_SZ	60

// OID Constants
#define MINIPORT_MAX_LOOKAHEAD           	256
#define MINIPORT_MTU_SIZE				1400
#define MINIPORT_MAX_FRAMESIZE           	1500
#define MINIPORT_HEADER_SIZE             	14
#define MINIPORT_MAX_TOTAL_SIZE          	(MINIPORT_MAX_FRAMESIZE + MINIPORT_HEADER_SIZE)
#define BUFFER_DATA_SIZE				1600	// maximum allocated data size,  mtu 1400 so 3 blocks max 1536 : sangam 
#define MINIPORT_VENDOR_DESCRIPTION      	"Samsung Electronics"
#define MINIPORT_DRIVER_VERSION	  		0x00010000
// Misc defines
#define MINIPORT_DRIVER_VERSION_STRING		"SDIO-100"	// for control
#define MINIPORT_LENGTH_OF_ADDRESS       	6
#define MINIPORT_MCAST_LIST_LENGTH		32
#define MINIPORT_PRINT_NAME			"C730"MINIPORT_DRIVER_VERSION_STRING
// configurable values
#define MINIPORT_MAX_SEND_PACKETS_DEFAULT      	1750
#define MINIPORT_MAX_RECV_PACKETS_DEFAULT      	1750  		// 12 Mbit / (1514 * 8) = about 1000
#define MINIPORT_MAX_LINKSPEED_DEFAULT         	1000000L	// 100 Mbit/s
#define MINIPORT_STATISTIC_INTERVAL_DEFAULT	10		// 10 sec
#define MINIPORT_WAKEUP_TIMEOUT_DEFAULT		100000		// 100 ms

#define MEDIA_DISCONNECTED 			0
#define MEDIA_CONNECTED 			1

//sangam : porting from windows
#define MINIPORT_ADAPTERX_TIMEOUT	HZ*10

#include "hw_types.h"
#include "ctl_types.h"
		
#ifdef WIBRO_USB_SELECTIVE_SUSPEND		//sangam:needed? 
enum {					//sangam:
	PowerDeviceD0, 		// Normal state
	PowerDeviceD2,		// Idle state
	PowerDeviceD3		// Suspend state
};
#endif

#define ADAPTER_UNPLUG 			0x00000040
#define Adapter_RX_URB_FAIL 	0x00000080

#define	WIBRO_STATE_NOT_READY		0
#define	WIBRO_STATE_READY 			1
#define	WIBRO_STATE_VIRTUAL_IDLE		2
#define	WIBRO_STATE_NORMAL			3
#define	WIBRO_STATE_IDLE				4
#define	WIBRO_STATE_RESET_REQUESTED 5
#define	WIBRO_STATE_RESET_ACKED 	6
#define	WIBRO_STATE_AWAKE_REQUESTED 7

/*
    Types
*/
// For checking the length of the data from the application
typedef struct {
	int length;
	unsigned char bytePacket[MINIPORT_MAX_TOTAL_SIZE];
} SAMSUNG_ETHER_HDR;

// Power definitions
#ifdef WIBRO_USB_SELECTIVE_SUSPEND
/*
     Types
*/
typedef struct {
    LONG			SuspendCancelled;	// cancel flag
    LONG			WakeCancelled;		// cancel flag
	USHORT			DevicePowerState;	// device power state
    BOOLEAN			IdleImminent;		// idle is imminent flag
    LONG			NAKRequested;		// NAK message requested because, system can't have suspend
    BOOLEAN			Extended;		// extended wake-up scheme
} WIBRO_POWER_INFO;
#endif

#ifdef WIBRO_SDIO_WMI
typedef struct {
	BOOLEAN				Enabled;
    BOOLEAN				isInited;	// initialized?
    WIBRO_802_16_STATE			State;		// device state
    WIBRO_802_16_POWER_MODE		PwrMode;	// device power mode
    WIBRO_802_16_VENDOR_INFORMATION	VendorInfo;	// vendor information
    WIBRO_802_16_TX_POWER_INFORMATION	TxPwrInfo;	// TX power information
    WIBRO_802_16_TX_POWER_CONTROL	TxPwrControl;	// TX power control mode
    WIBRO_802_16_BS_INFORMATION		BsInfo;		// base station information
    WIBRO_802_16_RF_INFORMATION		RfInfo;		// RF information
    WIBRO_802_16_AUTH_EAP_MSK		EapMsk;		// EAP MSK
    BOOLEAN				MskReady;	// is MSK ready?
    WIBRO_802_16_AUTH_MODE		AuthMode;	// auth mode
    BOOLEAN				KeyRequested;	// was key requested?
    WIBRO_802_16_IMEI			Imei;		// IMEI
    PWIBRO_802_16_FREQUENCY_LIST		FreqList;	// frequency list
    ULONG				FreqSelected;	// frequency index
    WIBRO_802_16_STATISTICS		Stats;		// device statistics
    USHORT				DmSeqNum;	// DM protocol sequence number
    ULONG				Antennas;	// number of antennas
    ULONG				InfIx;		// received information index
    LONG				Rssi[WIBRO_USB_DEV_CALC_ARRAY_SZ];
    LONG				Cinr[WIBRO_USB_DEV_CALC_ARRAY_SZ];
} WIBRO_MGT_INFO, *PWIBRO_MGT_INFO;
#endif

typedef struct MINIPORT_ADAPTER {
	struct sdio_func		*func;
	struct net_device		*net;
	struct net_device_stats	netstats;
	ULONG					AdapterIndex;
	unsigned long			ioport;
    // configurable values
    ULONG					Linkspeed;			// linkspeed value
    ULONG					SndQLimit;			// send queue limit
    ULONG					RcvQLimit;			// receive queue limit
	ULONG					FullQLimit;
    ULONG					WakeupTimeout;			// S3/S4 wakeup timeout
    ULONG					RWU;				// remote wake-up

    NDIS_MEDIA_STATE		MediaState;
	UCHAR					WibroStatus;
	UCHAR					Previous_WibroStatus;
	UCHAR					IPRefreshing;

	struct	work_struct		work;
	unsigned				flags;
	u32						msg_enable;
	struct urb				*rx_urb, *tx_urb, *intr_urb;
	struct sk_buff			*rx_pool[RX_SKBS];
	struct sk_buff			*rx_skb;
	unsigned char			*rx_buff;
	unsigned char			*ctrl_buf; 
	spinlock_t				rx_pool_lock;

	BOOLEAN 				acquired_mac_address;
	BOOLEAN					bHaltPending;	// miniport halt pending flag
	BOOLEAN					InitDone; // probe and mac completion
	BOOLEAN					DownloadMode;
	BOOLEAN					SurpriseRemoval;

    UCHAR                   CurrentAddress[MINIPORT_LENGTH_OF_ADDRESS];	  // currently used hardware address

	// Statistics used by ioctl : sangam 
	struct {
	struct {
    	ULONG					Count;		// number of all packet sent by driver
   		ULONG                 	Data;			// OID_GEN_XMIT_OK
	    ULONG                   Bytes;		// OID_GEN_DIRECTED_BYTES_XMIT
		ULONG					Control;		// Number of control packets sent
	    ULONG					Ack;			// number of HW ACKs
	    ULONG					Retry;			// number of HW retransmissions	
	}tx;
	struct {
		ULONG					Ints; 		// Number of received interrupts
		ULONG           		Count;         // number of all Rcv packets incoming into driver
		ULONG        			Data;			// OID_GEN_RCV_OK
	    ULONG              		Bytes;		// OID_GEN_DIRECTED_BYTES_RCV
		ULONG           		Control;			// control packets 
	}rx;
	struct {
		ULONG					G;
		ULONG					Count;
	}arp;
	}stats;

	ULONG					XmitErr; // packet send fails : sangam

	HARDWARE_INFO			hw;
    CONTROL_INFO 			ctl;	    	

	struct completion		hFWInitCompleteEvent;
	struct completion		hAwakeAckEvent;
	wait_queue_head_t		hFWDNEndEvent;	// firmware download
	BOOLEAN					bFWDNEndFlag;	

#ifdef DISCONNECT_TIMER
	BOOLEAN					DisconnectTimerActivated;
	struct timer_list		DisconnectTimer;
#endif
#ifdef WIBRO_SDIO_WMI 
	WIBRO_MGT_INFO			Mgt;
	struct timer_list		RetryAutoConnect;
#endif

#ifdef WIBRO_USB_SELECTIVE_SUSPEND
    WIBRO_POWER_INFO		Power;			// power control
#endif

    USHORT					DevFeatures;		// device features flag

    UCHAR      				MulticastList[MINIPORT_LENGTH_OF_ADDRESS * MINIPORT_MCAST_LIST_LENGTH]; 

} MINIPORT_ADAPTER, *PMINIPORT_ADAPTER;

#define hwSdioReadCounter(Adapter, pCounter, Status)\
{\
    *pCounter = 0;\
    *pCounter = sdio_readb(Adapter->func, SDIO_TRANSFER_COUNT_REG, Status);\
    if (*Status == 0)\
    *pCounter |= sdio_readb(Adapter->func, SDIO_TRANSFER_COUNT_REG + 1, Status) << 8;\
    if (*pCounter > SDIO_BUFFER_SIZE) {\
	*pCounter = 0;\
    }\
}
#endif
