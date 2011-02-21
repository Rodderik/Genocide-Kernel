#ifndef _WIBRO_DOWNLOAD_H__
#define _WIBRO__DOWNLOAD_H__

#define WIMAX_IMAGE_PATH "/system/etc/wimaxfw.bin"
#define WIMAX_LOADER_PATH "/system/etc/wimaxloader.bin"

#define CMC730_RAM_START			0x20000000
#define CMC730_WIBRO_ADDRESS		CMC730_RAM_START

#define CMD_MSG_TOTAL_LENGTH	12
#define IMAGE_INFO_MSG_TOTAL_LENGTH 	28

#define CMD_MSG_LENGTH			0
#define IMAGE_INFO_MSG_LENGTH 	16

#define MAX_IMAGE_DATA_LENGTH	3564
//#define MAX_IMAGE_DATA_LENGTH	3948 //sangam for 128 byte cur blk size
#define MAX_IMAGE_DATA_MSG_LENGTH 4096

#define FWDOWNLOAD_TIMEOUT 6000
//#define FWDOWNLOAD_TIMEOUT 1500000
#define MAX_WIMAXFW_SIZE 2100000

typedef enum
{
	DL_CODE_UNKNOWN,	// 0
	DL_CODE_READ,		// 1
	DL_CODE_DATA,		// 2
	DL_CODE_WRITE,	// 3
	DL_CODE_ACK,		// 4
	DL_CODE_CMD,		// 5
	DL_CODE_DONE,		// 6
	NUM_OF_DL_CODE
} DL_MESSAGE_CODE_LIST;

enum
{
	MSG_DRIVER_OK_REQ      = 0x5010,
	MSG_DRIVER_OK_RESP    = 0x6010,
	MSG_IMAGE_INFO_REQ    = 0x3021,
	MSG_IMAGE_INFO_RESP  = 0x4021,
	MSG_IMAGE_DATA_REQ   = 0x3022,
	MSG_IMAGE_DATA_RESP = 0x4022,
	MSG_RUN_REQ 		      = 0x5014,
	MSG_RUN_RESP                = 0x6014
};

typedef struct _IMAGE_DATA_Tag {	
	unsigned int    uiSize;
	unsigned int    uiWorkAddress;
	unsigned int 	uiOffset;
	unsigned long	buforder;	// For freeing the memory
	unsigned char * pImage;
} IMAGE_DATA, *PIMAGE_DATA;

typedef struct _IMAGE_DATA_Payload{
	unsigned int uiOffset;
	unsigned int uiLength;
	unsigned char ucImageData[MAX_IMAGE_DATA_LENGTH];
}IMAGE_DATA_PAYLOAD, *PIMAGE_DATA_PAYLOAD;

typedef struct WIMAX_MESSAGE_HEADER_STRUCT {
	unsigned short MsgType;
	unsigned short MsgID;
	unsigned int MsgLength;
	//unsigned short MsgLength;
} WIMAX_MESSAGE_HEADER, *PWIMAX_MESSAGE_HEADER;

extern unsigned int sd_send(MINIPORT_ADAPTER *Adapter, UCHAR* pBuffer, UINT cbBuffer);

#endif 
