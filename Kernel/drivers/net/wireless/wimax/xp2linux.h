#ifndef _WIBRO_XP2LINUX_H_WIBRO_CPE__
#define _WIBRO_XP2LINUX_H_WIBRO_CPE__



#include <linux/completion.h>


#define HW_ACCESS

#define DIS_STATUS_SEND_DPS	
#define LUNTCH_CONTROLPACKET_CONTROL 
#define LUNTCH_IOCONTROL
#define SEND_PACKET_BY_POLLING
#define SHOW_ARECV_CONTROL_PACKET_DATAS
#define ADD_FOUR_BYTE_DUMMY_IN_WRITE
#define NEED_WORD_SWAP

#define NDIS_STATUS		int

#define STATUS_SUCCESS				((NTSTATUS)0x00000000L)
#define STATUS_PENDING				((NTSTATUS)0x00000103L)	// The operation that was requested is pending completion
#define STATUS_RESOURCES			((NTSTATUS)0x00001003L)
#define STATUS_RESET_IN_PROGRESS	((NTSTATUS)0xc001000dL)
#define STATUS_DEVICE_FAILED		((NTSTATUS)0xc0010008L)
#define STATUS_NOT_ACCEPTED		((NTSTATUS)0x00010003L)
#define STATUS_FAILURE				((NTSTATUS)0xC0000001L)
#define STATUS_UNSUCCESSFUL		((NTSTATUS)0xC0000002L)	// The requested operation was unsuccessful
#define STATUS_CANCELLED			((NTSTATUS)0xC0000003L)


#define IN
#define OUT
#define PIRP				void
#define VOID				void
#define PVOID				void*

#define NDIS_HANDLE		int
#define LONG			long
#define ULONG			unsigned long	
#define ULONG64			unsigned long long	
#define PULONG			unsigned long*
#define BOOLEAN			unsigned char
#define CHAR			char
#define UCHAR			unsigned char
#define PUCHAR			unsigned char*

#define INT				int
#define UINT			unsigned int
#define USHORT			unsigned short
#define PUSHORT			unsigned short*
#define	SHORT			short
#define DWORD			unsigned long

#define NDIS_SPIN_LOCK	spinlock_t

#define	NDIS_STATUS_FAILURE	0;

typedef unsigned int*	PUINT;

typedef unsigned long NTSTATUS;
typedef struct completion	NDIS_EVENT;
typedef struct list_head	LIST_ENTRY, *PLIST_ENTRY;
typedef struct sk_buff 	NDIS_PACKET;

#define PNDIS_PACKET	NDIS_PACKET*
#define PNDIS_BUFFER	PUSHORT
#define NDIS_DEVICE_POWER_STATE   	UCHAR
#define NDIS_MEDIA_STATE            	UCHAR
#define NDIS_MEDIA_STATE            	UCHAR

#define NdisMediaStateConnected		1;
#define NdisMediaStateDisconnected	0;

#ifndef TRUE_FALSE_
#define TRUE_FALSE_
typedef enum BOOL {
	FALSE,
	TRUE
} BOOL;
#endif


#endif
