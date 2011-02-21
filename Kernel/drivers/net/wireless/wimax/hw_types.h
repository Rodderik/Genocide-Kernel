/**
* File name	    : hw_types.h
* File description  : Hardware types and definitions
*/
#ifndef _WIBRO_HW_TYPES_H
#define _WIBRO_HW_TYPES_H
// misc defines
#define HARDWARE_BUFFER_SIZE			(MINIPORT_MAX_TOTAL_SIZE + sizeof(HW_PACKET_HEADER) + 2)
#define HARDWARE_SEND_RESTART			5			// send restart
#define HARDWARE_MAX_MAC_RESPONSES		10			// maximum wait time on getting MAC address
#define HARDWARE_MAC_REQ_TIMEOUT		(1000 * 1000 * 10)	// 1 second timeout
#define HARDWARE_START_TIMEOUT			200			// 200 ms
#define HARDWARE_DISCONNECT_TIMEOUT		300			// 300 ms
// retransmission timeout
#define HARDWARE_RETRY_TIMEOUT			10			// 10 ms
#define HARDWARE_RETRY_MAX_COUNTER		100			// 100 times
// wake-up defines
#define HARDWARE_WAKE_MAX_COUNTER		10
#define HARDWARE_WAKEUP_TIMEOUT			10000			// 10 ms
// read counter
#define HARDWARE_READ_MAX_COUNTER		10			// 10 times
// private protocol defines
#define HW_PROT_VALUE_LINK_DOWN			0x00
#define HW_PROT_VALUE_LINK_UP			0xff
// SDIO general defines
#define SDIO_MAX_BLOCK_SIZE			2048			// maximum block size (SDIO)
#define SDIO_MAX_BYTE_SIZE				511			// maximum size in byte mode
//#define SDIO_MAX_BYTE_SIZE				127 		// sangam for 128 cur block size setting

//#define SDIO_BUFFER_SIZE			(SDIO_MAX_BLOCK_SIZE * 32)
#define SDIO_BUFFER_SIZE			(SDIO_MAX_BLOCK_SIZE * 4)	//cky 20100315

#define SDIO_ENABLE_INTERRUPT			0x01
#define SDIO_DISABLE_INTERRUPT			0x00
// SDIO function addresses 
#define SDIO_DATA_PORT_REG			0x00
#define SDIO_INT_STATUS_REG			0x04
#define SDIO_TRANSFER_COUNT_REG		0x1c
#define SDIO_CLK_WAKEUP_REG			0x38
// SDIO function registers
#define SDIO_INT_DATA_READY			0x01
#define SDIO_INT_ERROR				0x02
#define SDIO_COUNT_MASK				0x000fffff
#define SDIO_CLK_AUTO				0x01
#define SDIO_CLK_MANUAL				0x02

#ifdef DISCONNECT_TIMER
#define HARDWARE_DISCONNECT_TIMEOUT		300			// 300 ms
#endif
#define WAKEUP_MAX_TRY   				3
#define WAKEUP_TIMEOUT                            200

/*
   Hardware Types
*/
// packet types
enum {
    HwPktTypeNone = 0xff00,
    HwPktTypePrivate,
    HwPktTypeControl,
    HwPktTypeData,
    HwPktTypeTimeout
};

// private packet opcodes
enum {
    HwCodeMacRequest = 0x01,
    HwCodeMacResponse,
    HwCodeLinkIndication,
    HwCodeRxReadyIndication,
    HwCodeHaltedIndication,
    HwCodeIdleNtfy,
    HwCodeWakeUpNtfy
};

enum {
    SDIO_MODE = 0,
    WTM_MODE,
    MAC_IMEI_WRITE_MODE,
    USIM_RELAY_MODE,
    DM_MODE,
    USB_MODE,
    AUTH_MODE
};

#pragma pack(1)
typedef struct {		// header for packet,
    CHAR            	Id0;	// packet ID
    CHAR            	Id1;        
    USHORT          	Length;	// packet length
} HW_PACKET_HEADER, *PHW_PACKET_HEADER;

// private command
typedef struct {  
    CHAR            	Id0;			// packet ID
    CHAR            	Id1;    
    UCHAR				Code;			// command code
    UCHAR				Value;			// command value
} HW_PRIVATE_PACKET, *PHW_PRIVATE_PACKET;

typedef struct{
	USHORT MsgType;
	USHORT MsgId;
	UINT Length;
	//USHORT Length;
}HW_DOWNLOAD_PACKET, *PHW_DOWNLOAD_PACKET;
#pragma pack()

typedef struct {
//	PVOID           		ReceiveBuffer;		// receive data buffer	//sumanth: no longer necessary
	PVOID			ReceiveTempBuffer;

	BOOLEAN			ReceiveRequested;	// receive interrupt occured
	BOOLEAN			SendDisabled;		// send disabled
	ULONG			RestartDevice;		// device restarted due to system sleep

	UCHAR			EthHeader[MINIPORT_LENGTH_OF_ADDRESS * 2]; // static ethernet header

	BOOLEAN			RetryRequested;		// retransmission requested
	BOOLEAN			RetryTimerActivated;	// timer activated flag
	ULONG			RetryCounter;		// retry counter

	FREE_QUEUE_INFO		Q_Free;			// free queue
	USED_QUEUE_INFO	Q_Send;			// send pending queue
} HARDWARE_INFO, *PHARDWARE_INFO;

#endif
