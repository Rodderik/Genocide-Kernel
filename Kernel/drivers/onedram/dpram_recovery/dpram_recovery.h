#ifndef __DPRAM_RECOVERY_H__
#define __DPRAM_RECOVERY_H__

// dpram layout
#define DPRAM_BOOT_MAGIC_ADDR 0x0
#define DPRAM_BOOT_TYPE_ADDR 0x4
#define DPRAM_MODEM_STATUS_ADDR 0x8
#define DPRAM_AP_STATUS_ADDR 0xC
#define DPRAM_FIRMWARE_SIZE_ADDR 0x10
#define DPRAM_MODEM_STRING_MSG_ADDR 0x100
#define DPRAM_FIRMWARE_ADDR 0x1000

#define DPRAM_MODEM_MSG_SIZE 0x100

#define DPRAM_BOOT_MAGIC_RECOVERY_FOTA 0x56434552
#define DPRAM_BOOT_TYPE_DPRAM_DELTA 0x41544c44

// ioctl commands
#define IOCTL_ST_FW_UPDATE _IO('D',0x1)
#define IOCTL_CHK_STAT _IO('D',0x2)
#define IOCTL_MOD_PWROFF _IO('D',0x3)

// All status values are kept through out the process.
// So the final status value for a successful job will be 0xB60x1164
// This means that magic code is 	B6xxxxxx
// Job was started 					xxxxx1xx
// Job is done 100%					xxxxxx64 (0x64 is 100 in hex)
// Job is completed 				xxxx1xxx
// This way we can just check the final value and know the status of the job.
#define STATUS_JOB_MAGIC_CODE		0xB6000000
#define STATUS_JOB_MAGIC_M			0xFF000000
#define STATUS_JOB_STARTED_M		    0x00000100
#define STATUS_JOB_PROGRESS_M		0x000000FF 
#define STATUS_JOB_COMPLETE_M		0x00001000
#define STATUS_JOB_DEBUG_M			0x000F0000
#define STATUS_JOB_ERROR_M			0x00F00000
#define STATUS_JOB_ENDED_M			(STATUS_JOB_ERROR_M|STATUS_JOB_COMPLETE_M)


struct dpram_firmware {
    char *firmware;
    int size;
};

struct stat_info {
	int pct;
	char msg[DPRAM_MODEM_MSG_SIZE];
};

#endif	/* __DPRAM_RECOVERY_H__ */


