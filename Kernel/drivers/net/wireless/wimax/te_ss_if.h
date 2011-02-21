#ifdef WIBRO_SDIO_WMI
#ifndef _WIBRO_TE_SS_IF_H
#define _WIBRO_TE_SS_IF_H

/* 
	Defines
*/
#define ETHER_MAC_ADDR_LENGTH	6
#define HIM_LINK_INFO_PERIOD	0x0a00	    // 1 second
#define CT_FREQ_MAX_NUMBER	9
#define DM_STATUS_INFO_PERIOD	1000	    // 1 second

// ethernet type used by management entity
#define ETHER_TYPE_HIM		0x0015
#define ETHER_TYPE_DM		0x0215
#define ETHER_TYPE_CT		0x0315
// HIM types
#define HIM_TYPE_LINK		0x0100
#define HIM_TYPE_MAC_ADDRESS	0x0300
#define HIM_TYPE_HANDOVER	0x0500
#define HIM_TYPE_POWER		0x0600
#define HIM_TYPE_RESET		0x0700
#define HIM_TYPE_SS_INFO	0x0800
#define HIM_TYPE_DSX		0x0900
#define HIM_TYPE_CONTROL	0x0a00
#define HIM_TYPE_FREQ		0x0b00
#define HIM_TYPE_AUTH		0x2000
#define HIM_TYPE_FW_UPDATE	0x2100
#define HIM_TYPE_IF_MGT		0x2200
#define HIM_TYPE_PWR_MGT	0x2300
#define HIM_TYPE_TEST		0x0001
// link subtype messages
#define HIM_LINK_UP_REQ		0x0100
#define HIM_LINK_UP_ACK		0x0200
#define HIM_LINK_UP_NAK		0x0300
#define HIM_LINK_DOWN_REQ	0x0400
#define HIM_LINK_DOWN_ACK	0x0500
#define HIM_LINK_DOWN_NAK	0x0600
#define HIM_LINK_UP_NOTIFY	0x0700
#define HIM_LINK_DOWN_NOTIFY	0x0800
#define HIM_LINK_INFO_REQ	0x0900
#define HIM_LINK_INFO_RESP	0x0a00
#define HIM_LINK_STATE_REQ	0x0b00
#define HIM_LINK_STATE_RESP	0x0c00
#define HIM_LINK_STATE_NTFY	0x0d00
// link up/down requests
#define HIM_LINK_REQ_SYNC	0x0100
#define HIM_LINK_REQ_NET	0x0200
#define HIM_LINK_STOP_NET	0x0300
// link states
#define HIM_LINK_STATE_INIT	0x0100
#define HIM_LINK_STATE_SYNC	0x0200
#define HIM_LINK_STATE_NORMAL	0x0300
#define HIM_LINK_STATE_IDLE	0x0400
#define HIM_LINK_STATE_SLEEP	0x0500
#define HIM_LINK_STATE_ENTRY	0x0600
// link substates for network entry
#define HIM_LINK_ENTRY_RANGING	0x0100
#define HIM_LINK_ENTRY_SBC	0x0200
#define HIM_LINK_ENTRY_PKM	0x0300
#define HIM_LINK_ENTRY_REG	0x0400
#define HIM_LINK_ENTRY_DSX	0x0500
// link error codes
#define HIM_LINK_FAIL_PHY_SYNC	0x0100
#define HIM_LINK_FAIL_MAC_SYNC	0x0200
#define HIM_LINK_FAIL_RNG	0x0B00
#define HIM_LINK_FAIL_SBC	0x0C00
#define HIM_LINK_FAIL_PKM	0x0D00
#define HIM_LINK_FAIL_REG	0x0F00
#define HIM_LINK_FAIL_DSX	0x1000
#define HIM_LINK_FAIL_TIMEOUT	0x9900
// link down types
#define HIM_LINK_DOWN_SYNC	0x0100
#define HIM_LINK_DOWN_NET	0x0200
// link up types
#define HIM_LINK_UP_SYNC	0x0100
#define HIM_LINK_UP_NET		0x0200
// handover subtypes
#define HIM_HANDOVER_NTFY	0x0100
// power saving subtypes
#define HIM_POWER_SLEEP_REQ	0x0100
#define HIM_POWER_SLEEP_NTFY	0x0200
#define HIM_POWER_IDLE_REQ	0x0300
#define HIM_POWER_IDLE_NTFY	0x0400
#define HIM_POWER_DOWN_REQ	0x0500
#define HIM_POWER_DOWN_NAK	0x0600
#define HIM_POWER_DOWN_NTFY	0x0700
// SS info subtypes
#define HIM_SS_INFO_REQ		0x0100
#define HIM_SS_INFO_RES		0x0200
// auth subtypes
#define HIM_AUTH_REQ		0x0100
#define HIM_AUTH_RESP		0x0200
#define HIM_AUTH_COMPLETE	0x0300
#define HIM_AUTH_KEY_REQ	0x0500
#define HIM_AUTH_KEY_RESP	0x0600
#define HIM_AUTH_RESET		0x0700
// auth key request operations
#define HIM_AUTH_READ		0x0100
#define HIM_AUTH_WRITE		0x0200
// auth types
#define HIM_AUTH_EAPAKA_METHOD	0x0100
#define HIM_AUTH_IMEI		0x0200
#define HIM_AUTH_ATR_LOCK	0x0300
// auth response types
#define HIM_AUTH_RES_MSK	0x0100
#define HIM_AUTH_RES_DATA	0x0200
// Power management subtypes
#define HIM_PM_INCREMENT	0x0100
#define HIM_PM_DECREMENT	0x0200

// CT codes
#define CT_CODE_READ		0x00
#define CT_CODE_DATA		0x01
#define CT_CODE_WRITE		0x02
// CT G_ID
#define CT_GID_SYS_CFG		0x00
#define CT_GID_MAC_CFG		0x01
#define CT_GID_RF_CFG		0x02
#define CT_GID_SYS_INFO		0x03
// CT MAC CFG M_ID
#define CT_MID_FREQ_0		0x0000
#define CT_MID_FREQ_1		0x0100
#define CT_MID_FREQ_2		0x0200
#define CT_MID_FREQ_3		0x0300
#define CT_MID_FREQ_4		0x0400
#define CT_MID_FREQ_5		0x0500
#define CT_MID_TX_PWR_CONTROL	0x0c00
#define CT_MID_TX_POWER		0x0e00
#define CT_MID_FREQ_6		0x2200
#define CT_MID_FREQ_7		0x2300
#define CT_MID_FREQ_8		0x2400
#define CT_MID_TX_PWR_LIMIT	0x3900
// CT RF CFG M_ID
#define CT_MID_ANTENNA		0x0000

// DM flags
#define DM_FLAG_START		0x7f
#define DM_FLAG_END		0x7e
// DM commands
#define DM_CMD_TEST		0x00
#define DM_CMD_SEND		0x01
#define DM_CMD_START_REQ	0x02
#define DM_CMD_START_CNF	0x03
#define DM_CMD_STOP_REQ		0x04
#define DM_CMD_STOP_CNF		0x05
#define DM_CMD_NVRAM_UPDATE	0x07
#define DM_CMD_INFO_STATUS	0x0c
#define DM_CMD_INFO_HO		0x19
#define DM_CMD_TFREQ_CNG_REQ	0x1a
#define DM_CMD_TFREQ_CNG_CNF	0x1b
// DM Type code
#define DM_TYPE_CODE_HIDM	0x01
#define DM_TYPE_CODE_DEBUG	0x02
#define DM_TYPE_CODE_INNDM	0x03
#define DM_TYPE_CODE_TM		0x04
// power control modes
#define DM_PWR_CTL_MODE_CLOSED	0x00
#define DM_PWR_CTL_MODE_OPEN_RT	0x01
#define DM_PWR_CTL_MODE_OPEN_RS	0x02
#define DM_PWR_CTL_MODE_ACTIVE	0x03
#define DM_PWR_CTL_MODE_OFF	0x04

/* 
	Types
*/

#pragma pack(1)
// HIM header
typedef struct {
    UCHAR	dst[ETHER_MAC_ADDR_LENGTH];
    UCHAR	src[ETHER_MAC_ADDR_LENGTH];
    USHORT	eth_type;
    USHORT	him_type;
    USHORT	subtype;
    USHORT	length;
} him_pkt_hdr_t;

// link up/down command, power management command
typedef struct {
    USHORT	cmd;
} him_cmd_link_up_t, him_cmd_link_down_t, him_cmd_link_info_t, him_cmd_pm_t;

// auth read/write request/response
typedef struct {
    USHORT	oper;
    USHORT	type;
} him_cmd_auth_rdwr_t, him_res_auth_rdwr_t;

// auth response (from host to device)
typedef struct {
    USHORT	type;
    USHORT	length;
} him_res_auth_t;

// link state response, link NAK response, link notification
typedef struct {
    USHORT	res;
} him_res_link_state_t, him_res_link_nak_t, him_ntfy_link_down_t, him_ntfy_link_up_t;

// link state notification
typedef struct {
    USHORT	state;
    USHORT	time;
} him_ntfy_link_state_t;

// link info response
typedef struct {
    SHORT	Rssi;
    SHORT	Cinr;
    UCHAR	Bsid[6];
    USHORT	PreIndex;
    SHORT	TxPower;
} him_res_link_info_t;

// SS info response
typedef struct {
    UCHAR	PhyVer[64];
    UCHAR	MacVer[64];
} him_res_ss_info_t;

// CT packet header
typedef  struct {
    UCHAR	dst[ETHER_MAC_ADDR_LENGTH];
    UCHAR	src[ETHER_MAC_ADDR_LENGTH];
    USHORT	eth_type;
    UCHAR	code;
    UCHAR	g_id;
    USHORT	m_id;
    USHORT	length;
} ct_pkt_hdr_t;

// DM packet header
typedef  struct {
    UCHAR	dst[ETHER_MAC_ADDR_LENGTH];
    UCHAR	src[ETHER_MAC_ADDR_LENGTH];
    USHORT	eth_type;
    UCHAR	start;
    UCHAR	cmd;
    USHORT	seqnum;
    USHORT	length;
} dm_pkt_hdr_t;

// basic information
typedef struct {
    ULONG	HOCntTry;
    ULONG	HOCntFail;
    ULONG	PingPongCnt;
    LONG	ServCinr;
    ULONG	IntraFANbrNum;
    ULONG	IntraFANbrCinr[24];
    UCHAR	IntraFANbrRssi[24];
    UCHAR	IntraFANbrPrIx[24];
    ULONG	InterFANbrNum;
    ULONG	InterFANbrCinr[24];
    UCHAR	InterFANbrRssi[24];
    UCHAR	InterFANbrPrIx[24];
    UCHAR	PRNGOverride;
    UCHAR	Reserved0;
    USHORT	Q2_CtrlNI;
    USHORT	Q2_PUSCNI;
    USHORT	Q2_OPUSCNI;
    USHORT	Q2_AMC;
    USHORT	Q2_AAS;
    USHORT	Q2_PRNGNI;
    UCHAR	PwrCtlMode;
    UCHAR	Reserved1;
    USHORT	PathLoss;
    USHORT	Q2_OffsetSS;
    USHORT	Q2_OffsetBS;
    USHORT	TxPowerSC;
    USHORT	SubCnlNum;
    UCHAR	PreambleIndex;
    UCHAR	DL_ZoneInfPerm[5];
    UCHAR	DL_FECType[13];
    UCHAR	Reserved2;
    USHORT	DL_FECTypeCnt[13][4];
    UCHAR	UL_PermBase;
    UCHAR	UL_ZoneInfPerm[4];
    UCHAR	UL_FECType[10];
    UCHAR	Reserved3;
    USHORT	UL_FECTypeCnt[10][4];
    UCHAR	Reserved4[2];
    ULONG	FrameNum;
    ULONG	UL_DataRate;
    ULONG	DL_DataRate;
    ULONG	UL_NumSDU;
    ULONG	UL_NumPDU;
    ULONG	DL_NumSDU;
    ULONG	DL_NumPDU;
    ULONG	ErrCRC;
    ULONG	ErrHCS;
    ULONG	DL_Burst;
    ULONG	DL_BurstErr;
    ULONG	Discarded;
    ULONG	MACState;
    LONG	Cinr;
    ULONG	DL_MapCRCErr;
    ULONG	CurFreq;
    struct {
	ULONG	ConType;
	ULONG	ConID;
    } DL[12];
    struct {
	ULONG	ConType;
	ULONG	ConID;
    } UL[8];
    SHORT	Rssi;
    SHORT	TxPower;
    UCHAR	PHYSync;
    UCHAR	MACSync;
    UCHAR	DL_FrmRatio;
    UCHAR	UL_FrmRatio;
    UCHAR	RAS[6];
    UCHAR	Reserved5[2];
} dm_basic_info_t;

//#pragma pack()

/*
    Functions
*/
static inline ULONG himSendStateReq(PUCHAR Buffer)
{
    him_pkt_hdr_t* h = (him_pkt_hdr_t*)Buffer;
    h->eth_type = ETHER_TYPE_HIM;
    h->him_type = HIM_TYPE_LINK;
    h->subtype  = HIM_LINK_STATE_REQ;
    h->length   = 0;
    return sizeof(him_pkt_hdr_t);
}

static inline ULONG himSendStartLinkInfo(PUCHAR Buffer)
{
    him_pkt_hdr_t*       h = (him_pkt_hdr_t*)Buffer;
    him_cmd_link_info_t* c = (him_cmd_link_info_t*)(Buffer + sizeof(him_pkt_hdr_t));
    h->eth_type = ETHER_TYPE_HIM;
    h->him_type = HIM_TYPE_LINK;
    h->subtype  = HIM_LINK_INFO_REQ;
    h->length   = be16_to_cpu(sizeof(him_cmd_link_info_t));
 //   h->length   = _byteswap_ushort(sizeof(him_cmd_link_info_t));
    c->cmd	= HIM_LINK_INFO_PERIOD; // time
    return (sizeof(him_pkt_hdr_t) + sizeof(him_cmd_link_info_t));
}

static inline ULONG himSendStopLinkInfo(PUCHAR Buffer)
{
    him_pkt_hdr_t*       h = (him_pkt_hdr_t*)Buffer;
    him_cmd_link_info_t* c = (him_cmd_link_info_t*)(Buffer + sizeof(him_pkt_hdr_t));
    h->eth_type = ETHER_TYPE_HIM;
    h->him_type = HIM_TYPE_LINK;
    h->subtype  = HIM_LINK_INFO_REQ;
    h->length   = be16_to_cpu(sizeof(him_cmd_link_info_t));
    c->cmd	= 0xffff; // time
    return (sizeof(him_pkt_hdr_t) + sizeof(him_cmd_link_info_t));
}

static inline ULONG himSendSyncUp(PUCHAR Buffer)
{
    him_pkt_hdr_t*     h = (him_pkt_hdr_t*)Buffer;
    him_cmd_link_up_t* c = (him_cmd_link_up_t*)(Buffer + sizeof(him_pkt_hdr_t));
    h->eth_type = ETHER_TYPE_HIM;
    h->him_type = HIM_TYPE_LINK;
    h->subtype  = HIM_LINK_UP_REQ;
    h->length   = be16_to_cpu(sizeof(him_cmd_link_up_t));
    c->cmd	= HIM_LINK_REQ_SYNC;
    return (sizeof(him_pkt_hdr_t) + sizeof(him_cmd_link_up_t));
}

static inline ULONG himSendSyncDown(PUCHAR Buffer)
{
    him_pkt_hdr_t*	 h = (him_pkt_hdr_t*)Buffer;
    him_cmd_link_down_t* c = (him_cmd_link_down_t*)(Buffer + sizeof(him_pkt_hdr_t));
    h->eth_type = ETHER_TYPE_HIM;
    h->him_type = HIM_TYPE_LINK;
    h->subtype  = HIM_LINK_DOWN_REQ;
    h->length   = be16_to_cpu(sizeof(him_cmd_link_down_t));
    c->cmd	= HIM_LINK_REQ_SYNC;
    return (sizeof(him_pkt_hdr_t) + sizeof(him_cmd_link_down_t));
}

static inline ULONG himSendConnect(PUCHAR Buffer)
{
    him_pkt_hdr_t*     h = (him_pkt_hdr_t*)Buffer;
    him_cmd_link_up_t* c = (him_cmd_link_up_t*)(Buffer + sizeof(him_pkt_hdr_t));
    h->eth_type = ETHER_TYPE_HIM;
    h->him_type = HIM_TYPE_LINK;
    h->subtype  = HIM_LINK_UP_REQ;
    h->length   = be16_to_cpu(sizeof(him_cmd_link_up_t));
    c->cmd	= HIM_LINK_REQ_NET;
    return (sizeof(him_pkt_hdr_t) + sizeof(him_cmd_link_up_t));
}

static inline ULONG himSendDisconnect(PUCHAR Buffer)
{
    him_pkt_hdr_t*	 h = (him_pkt_hdr_t*)Buffer;
    him_cmd_link_down_t* c = (him_cmd_link_down_t*)(Buffer + sizeof(him_pkt_hdr_t));
    h->eth_type = ETHER_TYPE_HIM;
    h->him_type = HIM_TYPE_LINK;
    h->subtype  = HIM_LINK_DOWN_REQ;
    h->length   = be16_to_cpu(sizeof(him_cmd_link_down_t));
    c->cmd	= HIM_LINK_REQ_NET;
    return (sizeof(him_pkt_hdr_t) + sizeof(him_cmd_link_down_t));
}

static inline ULONG himSendPwrIdle(PUCHAR Buffer)
{
    him_pkt_hdr_t* h = (him_pkt_hdr_t*)Buffer;
    h->eth_type = ETHER_TYPE_HIM;
    h->him_type = HIM_TYPE_POWER;
    h->subtype  = HIM_POWER_IDLE_REQ;
    h->length   = 0;
    return sizeof(him_pkt_hdr_t);
}

static inline ULONG himSendPwrSleep(PUCHAR Buffer)
{
    him_pkt_hdr_t* h = (him_pkt_hdr_t*)Buffer;
    h->eth_type = ETHER_TYPE_HIM;
    h->him_type = HIM_TYPE_POWER;
    h->subtype  = HIM_POWER_SLEEP_REQ;
    h->length   = 0;
    return sizeof(him_pkt_hdr_t);
}

static inline ULONG himSendSSInfoReq(PUCHAR Buffer)
{
    him_pkt_hdr_t* h = (him_pkt_hdr_t*)Buffer;
    h->eth_type = ETHER_TYPE_HIM;
    h->him_type = HIM_TYPE_SS_INFO;
    h->subtype  = HIM_SS_INFO_REQ;
    h->length   = 0;
    return sizeof(him_pkt_hdr_t);
}

static inline ULONG himSendImeiReq(PUCHAR Buffer)
{
    him_pkt_hdr_t*	 h = (him_pkt_hdr_t*)Buffer;
    him_cmd_auth_rdwr_t* c = (him_cmd_auth_rdwr_t*)(Buffer + sizeof(him_pkt_hdr_t));
    h->eth_type = ETHER_TYPE_HIM;
    h->him_type = HIM_TYPE_AUTH;
    h->subtype  = HIM_AUTH_KEY_REQ;
    h->length   = be16_to_cpu(sizeof(him_cmd_auth_rdwr_t));
    c->oper	= HIM_AUTH_READ;
    c->type	= HIM_AUTH_IMEI;
    return (sizeof(him_pkt_hdr_t) + sizeof(him_cmd_auth_rdwr_t));
}

static inline ULONG himSendAuthResp(PUCHAR Buffer, PVOID Data, USHORT Length)
{
    him_pkt_hdr_t*  h = (him_pkt_hdr_t*)Buffer;
    him_res_auth_t* r = (him_res_auth_t*)(Buffer + sizeof(him_pkt_hdr_t));
    h->eth_type = ETHER_TYPE_HIM;
    h->him_type = HIM_TYPE_AUTH;
    h->subtype  = HIM_AUTH_RESP;
    h->length   = be16_to_cpu(sizeof(him_res_auth_t) + Length);
    r->type	= HIM_AUTH_RES_MSK;
    r->length	= be16_to_cpu(Length);
    memcpy(Buffer + sizeof(him_pkt_hdr_t) + sizeof(him_res_auth_t), Data, Length);
    return (sizeof(him_pkt_hdr_t) + sizeof(him_res_auth_t) + Length);
}

static inline ULONG himSendPmInc(PUCHAR Buffer, USHORT Level)
{
    him_pkt_hdr_t* h = (him_pkt_hdr_t*)Buffer;
    him_cmd_pm_t*  c = (him_cmd_pm_t*)(Buffer + sizeof(him_pkt_hdr_t));
    h->eth_type = ETHER_TYPE_HIM;
    h->him_type = HIM_TYPE_PWR_MGT;
    h->subtype  = HIM_PM_INCREMENT;
    h->length   = be16_to_cpu(sizeof(him_cmd_pm_t));
    c->cmd	= be16_to_cpu(Level);
    return (sizeof(him_pkt_hdr_t) + sizeof(him_cmd_pm_t));
}

static inline ULONG himSendPmDec(PUCHAR Buffer, USHORT Level)
{
    him_pkt_hdr_t* h = (him_pkt_hdr_t*)Buffer;
    him_cmd_pm_t*  c = (him_cmd_pm_t*)(Buffer + sizeof(him_pkt_hdr_t));
    h->eth_type = ETHER_TYPE_HIM;
    h->him_type = HIM_TYPE_PWR_MGT;
    h->subtype  = HIM_PM_DECREMENT;
    h->length   = be16_to_cpu(sizeof(him_cmd_pm_t));
    c->cmd	= be16_to_cpu(Level);
    return (sizeof(him_pkt_hdr_t) + sizeof(him_cmd_pm_t));
}

static inline ULONG ctGetTxPwrControl(PUCHAR Buffer)
{
    ct_pkt_hdr_t* h = (ct_pkt_hdr_t*)Buffer;
    h->eth_type = ETHER_TYPE_CT;
    h->code	= CT_CODE_READ;
    h->g_id	= CT_GID_MAC_CFG;
    h->m_id	= CT_MID_TX_PWR_CONTROL;
    h->length	= 0x0400;
    return (sizeof(ct_pkt_hdr_t) + 4);
}

static inline ULONG ctSetTxPwrControl(PUCHAR Buffer, ULONG PwrControl)
{
    ct_pkt_hdr_t* h = (ct_pkt_hdr_t*)Buffer;
    PULONG	  u = (PULONG)(Buffer + sizeof(ct_pkt_hdr_t));
    h->eth_type = ETHER_TYPE_CT;
    h->code	= CT_CODE_WRITE;
    h->g_id	= CT_GID_MAC_CFG;
    h->m_id	= CT_MID_TX_PWR_CONTROL;
    h->length	= 0x0400;
    *u		= be32_to_cpu(PwrControl);
    return (sizeof(ct_pkt_hdr_t) + sizeof(ULONG));
}

static inline ULONG ctGetTxPwrLimit(PUCHAR Buffer)
{
    ct_pkt_hdr_t* h = (ct_pkt_hdr_t*)Buffer;
    h->eth_type = ETHER_TYPE_CT;
    h->code	= CT_CODE_READ;
    h->g_id	= CT_GID_MAC_CFG;
    h->m_id	= CT_MID_TX_PWR_LIMIT;
    h->length	= 0x0100;
    return (sizeof(ct_pkt_hdr_t) + 1);
}

static inline ULONG ctSetTxPowerLvl(PUCHAR Buffer, UCHAR Lvl)
{
    ct_pkt_hdr_t* h = (ct_pkt_hdr_t*)Buffer;
    PUCHAR	  b = (PUCHAR)(Buffer + sizeof(ct_pkt_hdr_t));
    h->eth_type = ETHER_TYPE_CT;
    h->code	= CT_CODE_WRITE;
    h->g_id	= CT_GID_MAC_CFG;
    h->m_id	= CT_MID_TX_POWER;
    h->length	= 0x0100;
    *b		= Lvl;
    return (sizeof(ct_pkt_hdr_t) + 1);
}

static inline ULONG ctGetAntennasNumber(PUCHAR Buffer)
{
    ct_pkt_hdr_t* h = (ct_pkt_hdr_t*)Buffer;
    h->eth_type = ETHER_TYPE_CT;
    h->code	= CT_CODE_READ;
    h->g_id	= CT_GID_RF_CFG;
    h->m_id	= CT_MID_ANTENNA;
    h->length	= 0x0100;
    return (sizeof(ct_pkt_hdr_t) + 1);
}

static inline ULONG ctGetFrequency(PUCHAR Buffer, USHORT Index)
{
    ct_pkt_hdr_t* h = (ct_pkt_hdr_t*)Buffer;
    h->eth_type = ETHER_TYPE_CT;
    h->code	= CT_CODE_READ;
    h->g_id	= CT_GID_MAC_CFG;
    
	if (Index < 6) {
		h->m_id	= be16_to_cpu(Index);
    }
    else if ((Index >= 6) && (Index < CT_FREQ_MAX_NUMBER)) {
		h->m_id = be16_to_cpu(Index + 0x1c);
    }
    else {
		return 0;
    }
   
	h->length	= 0x0400;
    return (sizeof(ct_pkt_hdr_t) + 4);
}

static inline ULONG ctSetFrequency(PUCHAR Buffer, USHORT Index, ULONG Value)
{
    ct_pkt_hdr_t* h = (ct_pkt_hdr_t*)Buffer;
    PULONG	  u = (PULONG)(Buffer + sizeof(ct_pkt_hdr_t));
    
	h->eth_type = ETHER_TYPE_CT;
    h->code	= CT_CODE_WRITE;
    h->g_id	= CT_GID_MAC_CFG;

    if (Index < 6) {
		h->m_id	= be16_to_cpu(Index);
    }
    else if ((Index >= 6) && (Index < CT_FREQ_MAX_NUMBER)) {
		h->m_id = be16_to_cpu(Index + 0x1c);
    }
    else {
		return 0;
    }
    
	h->length	= 0x0400;
    *u		= be32_to_cpu(Value);
    
	return (sizeof(ct_pkt_hdr_t) + sizeof(ULONG));
}

static inline ULONG dmSendReportPeriodReq(PUCHAR Buffer, PUSHORT SeqNum)
{
    dm_pkt_hdr_t* h = (dm_pkt_hdr_t*)Buffer;
    PULONG	  u = (PULONG)(Buffer + sizeof(dm_pkt_hdr_t));
    PUCHAR	  b = (PUCHAR)(Buffer + sizeof(dm_pkt_hdr_t) + sizeof(ULONG));
    h->eth_type = ETHER_TYPE_DM;
    h->start	= DM_FLAG_START;
    h->cmd	= DM_CMD_START_REQ;
    h->seqnum	= be16_to_cpu(*SeqNum); (*SeqNum)++;
    h->length	= 0x0400;
    *u		= be32_to_cpu(DM_STATUS_INFO_PERIOD);
    *b		= DM_FLAG_END;
    return (sizeof(dm_pkt_hdr_t) + sizeof(ULONG) + 1);
}

static inline ULONG dmSendStartReq(PUCHAR Buffer, PUSHORT SeqNum)
{
    dm_pkt_hdr_t* h = (dm_pkt_hdr_t*)Buffer;
    PULONG	  u = (PULONG)(Buffer + sizeof(dm_pkt_hdr_t));
    PUCHAR	  b = (PUCHAR)(Buffer + sizeof(dm_pkt_hdr_t) + sizeof(ULONG));
    h->eth_type = ETHER_TYPE_DM;
    h->start	= DM_FLAG_START;
    h->cmd	= DM_CMD_START_REQ;
    h->seqnum	= be16_to_cpu(*SeqNum); (*SeqNum)++;
    h->length	= 0x0400;
    *u		= DM_TYPE_CODE_DEBUG;
    *b		= DM_FLAG_END;
    return (sizeof(dm_pkt_hdr_t) + sizeof(ULONG) + 1);
}

static inline ULONG dmSendStopReq(PUCHAR Buffer, PUSHORT SeqNum)
{
    dm_pkt_hdr_t* h = (dm_pkt_hdr_t*)Buffer;
    PUCHAR	  b = (PUCHAR)(Buffer + sizeof(dm_pkt_hdr_t) + 8);
    CHAR	  s[] = "STOP REQ";
    h->eth_type = ETHER_TYPE_DM;
    h->start	= DM_FLAG_START;
    h->cmd	= DM_CMD_STOP_REQ;
    h->seqnum	= be16_to_cpu(*SeqNum); (*SeqNum)++;
    h->length	= 0x0800;
    memcpy(Buffer + sizeof(dm_pkt_hdr_t), s, 8);
    *b		= DM_FLAG_END;
    return (sizeof(dm_pkt_hdr_t) + 9);
}
#endif

#endif
