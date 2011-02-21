#ifndef _WIBRO_CTL_TYPES_H
#define _WIBRO_CTL_TYPES_H
/**
* File name	    : ctl_types.h
* File description  : Control types and definitions
*/

// Control definitions
/*
    Defines
*/

#ifndef FILE_DEVICE_UNKNOWN
#define FILE_DEVICE_UNKNOWN 0x89	// 0x22
#endif

// Macro definition for defining IOCTL 
//
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)
//
// Define the method codes for how buffers are passed for I/O and FS controls
//
#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3
//
// Define the access check value for any access
//
//
// The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
// ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
// constants *MUST* always be in sync.
//
#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )	// file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )	// file & pipe

#define CONTROL_ETH_TYPE_WCM		0x0015


#define	CONTROL_IOCTL_WRITE_REQUEST		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x820, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_SERIAL_CLOSE_REQUEST	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x825, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define CONTROL_IOCTL_GET_MAC_REQUEST		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x826, METHOD_BUFFERED, FILE_ANY_ACCESS)
#ifdef CONTROL_USB_WCM_LINK_INDICATION 
#define	SAMSUNG_LINK_UP			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x830, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	SAMSUNG_LINK_DOWN		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x832, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
#define	CONTROL_IOCTL_GET_DRIVER_VERSION	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x833, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_GET_NETWORK_STATISTICS	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x834, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_SET_DISCONNECT_TIMEOUT	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x835, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_GET_DEVICE_INFO		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x836, METHOD_BUFFERED, FILE_ANY_ACCESS)
#ifdef WIBRO_USB_SELECTIVE_SUSPEND		//sangam:needed? 
// internal wake-up (resume operations) request
#define SAMSUNG_WAKEUP_REQ		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x840, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif



#define MAX_APPLICATION_LEN		50
/*
    Types
*/
#pragma pack(1)
// network statistics structure for WCM
typedef struct {
    ULONG64		XmitOkBytes;
    ULONG64		RcvOkBytes;
} CONTROL_NETWORK_STATISTICS, *PCONTROL_NETWORK_STATISTICS;

// HIM header
typedef struct {
	UCHAR           	Destination[MINIPORT_LENGTH_OF_ADDRESS];
	UCHAR           	Source[MINIPORT_LENGTH_OF_ADDRESS];
	USHORT          	Type;
} CONTROL_ETHERNET_HEADER, *PCONTROL_ETHERNET_HEADER;
#pragma pack()

typedef struct {
	LIST_ENTRY      	Node;
	wait_queue_head_t		read_wait;
	USHORT          	Type;
	ULONG           	Id;
	UCHAR			name[MAX_APPLICATION_LEN];	// sangam:Find ID for the application in linux
 	USHORT            	Irp;	//sangam : Used for the read thread indication
} CONTROL_PROCESS_DESCRIPTOR, *PCONTROL_PROCESS_DESCRIPTOR;

typedef struct {
//	PDEVICE_OBJECT	DeviceObject;		// we create special device for interconnection with Virtual Serial Port Driver
//	NDIS_HANDLE     	DeviceHandle;
    USHORT				Type;
//	PIRP				Irp;
	NDIS_SPIN_LOCK		Lock;
    BOOLEAN		Inited;
} CONTROL_SERIAL_DESCRIPTOR, *PCONTROL_SERIAL_DESCRIPTOR;

typedef struct {
//	PDEVICE_OBJECT	DeviceObject;
//  NDIS_HANDLE     	DeviceHandle;
    LIST_ENTRY      	ProcessList;		// there could be undefined number of applications
    NDIS_SPIN_LOCK	Lock;
    BOOLEAN		Inited;
} CONTROL_APP_DESCRIPTOR, *PCONTROL_APP_DESCRIPTOR;

typedef struct {
    CONTROL_SERIAL_DESCRIPTOR	Serial;	 		// serial device structure
    CONTROL_APP_DESCRIPTOR		Apps;			// application device structure
    USED_QUEUE_INFO    			Q_Received;	    // pending queue
// 	struct timer_list			Picker;		    // Irp picker timer
//  ANSI_STRING			Name;		    // device name
//	ANSI_STRING			Key;		    // device key
} CONTROL_INFO, *PCONTROL_INFO;

#endif
