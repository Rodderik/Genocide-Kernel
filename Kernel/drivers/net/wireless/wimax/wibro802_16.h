#ifndef _WIBRO_WIBRO802_16_H
#define _WIBRO_WIBRO802_16_H

#define OID_802_16_RF_UP		0x10010101
#define OID_802_16_RF_DOWN		0x10010102
#define OID_802_16_NETWORK_ENTRY	0x10010103
#define OID_802_16_NETWORK_DISCONNECT	0x10010104
#define OID_802_16_BS_INFORMATION	0x10010105
#define OID_802_16_RF_INFORMATION	0x10010106
#define OID_802_16_STATISTICS		0x10020107
#define OID_802_16_VENDOR_INFORMATION   0x10010108
#define OID_802_16_POWER_MODE		0x10010109
#define OID_802_16_STATE		0x1001010a
#define OID_802_16_AUTH_EAP_MASK	0x1001010b
#define OID_802_16_IMEI			0x1001010c
#define OID_802_16_FREQUENCY		0x1001010d
#define OID_802_16_AUTH_MODE		0x1001010e
#define OID_802_16_TX_POWER		0x1001010f
#define OID_802_16_VENDOR_SPECIFIC	0x10010210

typedef UCHAR	WIBRO_802_16_MAC_ADDRESS[6];
typedef UCHAR	WIBRO_802_16_AUTH_EAP_MSK[64];
typedef UCHAR	WIBRO_802_16_IMEI[8];

typedef enum _WIBRO_802_16_POWER_MODE {
    Wibro802_16PM_Normal,
    Wibro802_16PM_Idle,
    Wibro802_16PM_Sleep
} WIBRO_802_16_POWER_MODE, *PWIBRO_802_16_POWER_MODE;

typedef enum _WIBRO_802_16_STATE {
    Wibro802_16State_Null,
    Wibro802_16State_Standby,
    Wibro802_16State_OutOfZone,
    Wibro802_16State_Awake,
    Wibro802_16State_NetEntry,
    Wibro802_16State_NetEntry_Ranging,
    Wibro802_16State_NetEntry_SBC,
    Wibro802_16State_NetEntry_PKM,
    Wibro802_16State_NetEntry_REG,
    Wibro802_16State_NetEntry_DSX
} WIBRO_802_16_STATE, *PWIBRO_802_16_STATE;

typedef enum _WIBRO_802_16_STATUS_TYPE {
    Wibro802_16StatusType_PowerMode,
    Wibro802_16StatusType_Handover,
    Wibro802_16StatusType_Obstacle
} WIBRO_802_16_STATUS_TYPE, *PWIBRO_802_16_STATUS_TYPE;

typedef enum _WIBRO_802_16_POWER_CONTROL_MODE {
    Wibro802_16PowerControlMode_ClosedLoop,
    Wibro802_16PowerControlMode_OpenLoopPassive,
    Wibro802_16PowerControlMode_OpenLoopActive,
    Wibro802_16PowerControlMode_Reserved
} WIBRO_802_16_POWER_CONTROL_MODE, *PWIBRO_802_16_POWER_CONTROL_MODE;

typedef enum _WIBRO_802_16_AUTH_MODE {
    Wibro802_16AuthMode_PKM,
    Wibro802_16AuthMode_Open
} WIBRO_802_16_AUTH_MODE, *PWIBRO_802_16_AUTH_MODE;

typedef enum _WIBRO_802_16_TX_POWER_CONTROL {
    Wibro802_16TxPowerControl_Auto,
    Wibro802_16TxPowerControl_Fixed
} WIBRO_802_16_TX_POWER_CONTROL, *PWIBRO_802_16_TX_POWER_CONTROL;

#pragma pack(1)
typedef struct _WIBRO_802_16_BS_INFORMATION {
    WIBRO_802_16_MAC_ADDRESS	    BSID;
    ULONG			    ULPermBase;
    ULONG			    DLPermBase;
    ULONG			    CurrentPreambleIndex;
    ULONG			    PreviousPreambleIndex;
    ULONG			    HOSignalLatency;
} WIBRO_802_16_BS_INFORMATION, *PWIBRO_802_16_BS_INFORMATION;

typedef struct _WIBRO_802_16_RF_INFORMATION {
    LONG			    PrimaryRssi;
    LONG			    PrimaryRssiDeviation;
    LONG			    PrimaryCinr;
    LONG			    PrimaryCinrDeviation;
    LONG			    DiversityRssi;
    LONG			    DiversityRssiDeviation;
    LONG			    DiversityCinr;
    LONG			    DiversityCinrDeviation;
    WIBRO_802_16_POWER_CONTROL_MODE  PowerControlMode;
    ULONG			    PERReceiveCount;
    ULONG			    PERErrorCount;
    ULONG			    ULBurstDataFECScheme;
    ULONG			    DLBurstDataFECScheme;
    ULONG			    ULBurstDataUIUC;
    ULONG			    DLBurstDataUIUC;
} WIBRO_802_16_RF_INFORMATION, *PWIBRO_802_16_RF_INFORMATION;

typedef struct _WIBRO_802_16_STATISTICS {
    ULONG64			    TotalRxPacketLength;
    ULONG64			    TotalTxPacketLength;
    ULONG64			    TotalRxPacketCount;
    ULONG64			    TotalTxPacketCount;
    ULONG			    TxRetryCount;
    ULONG			    RxErrorCount;
    ULONG			    RxMissingCount;
    ULONG			    HandoverCount;
    ULONG			    HandoverFailCount;
    ULONG			    ResyncCount;
} WIBRO_802_16_STATISTICS, *PWIBRO_802_16_STATISTICS;

typedef struct _WIBRO_802_16_VENDOR_INFORMATION {
    UCHAR			    PhyVer[32];
    UCHAR			    MacVer[32];
    UCHAR			    Vendor[64];
} WIBRO_802_16_VENDOR_INFORMATION, *PWIBRO_802_16_VENDOR_INFORMATION;

typedef struct _WIBRO_802_16_FREQUENCY {
    ULONG			    FreqIndex;
    ULONG			    Frequency;
} WIBRO_802_16_FREQUENCY, *PWIBRO_802_16_FREQUENCY;

typedef struct _WIBRO_802_16_FREQUENCY_LIST {
    ULONG			    ArrCnt;
    WIBRO_802_16_FREQUENCY	    Frequency[1];
} WIBRO_802_16_FREQUENCY_LIST, *PWIBRO_802_16_FREQUENCY_LIST;

typedef struct _WIBRO_802_16_TX_POWER {
    WIBRO_802_16_TX_POWER_CONTROL    TxPowerControl;
    LONG			    TxPower;
} WIBRO_802_16_TX_POWER, *PWIBRO_802_16_TX_POWER;

typedef struct _WIBRO_802_16_TX_POWER_INFORMATION {
    LONG			    TxPower;
    LONG			    TxPowerMaximum;
    LONG			    TxPowerHeadroom;
} WIBRO_802_16_TX_POWER_INFORMATION, *PWIBRO_802_16_TX_POWER_INFORMATION;

typedef struct _WIBRO_802_16_VENDOR_SPECIFIC {
    ULONG			    Length;
    UCHAR			    VendorSpecific[1];
} WIBRO_802_16_VENDOR_SPECIFIC, *PWIBRO_802_16_VENDOR_SPECIFIC;

typedef struct _WIBRO_802_16_POWER_MODE_INDICATION {
    WIBRO_802_16_STATUS_TYPE	    Type;
    WIBRO_802_16_POWER_MODE	    PowerMode;
} WIBRO_802_16_POWER_MODE_INDICATION, *PWIBRO_802_16_POWER_MODE_INDICATION;

typedef struct _WIBRO_802_16_HAND_OVER_INDICATION {
    WIBRO_802_16_STATUS_TYPE	    Type;
    WIBRO_802_16_MAC_ADDRESS	    BSID;
    BOOLEAN			    DhcpRequest;
} WIBRO_802_16_HAND_OVER_INDICATION, *PWIBRO_802_16_HAND_OVER_INDICATION;

typedef struct _WIBRO_802_16_OBSTACLE_INDICATION {
    WIBRO_802_16_STATUS_TYPE	    Type;
    ULONG			    Length;
    UCHAR			    Error[1];
} WIBRO_802_16_OBSTACLE_INDICATION, *PWIBRO_802_16_OBSTACLE_INDICATION;
#pragma pack()

#endif 
