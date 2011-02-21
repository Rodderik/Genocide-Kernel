/*
 * This program is free software; you can redistribute it and/or modify      
 * it under the terms of the GNU General Public License version 2 as         
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/irq.h>
#include <linux/poll.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/hardware.h>

#include <linux/cdev.h>
#include "dpram_recovery.h"

#define _DEBUG_

#ifdef _DEBUG_
#define MSGCRIT "\x1b[1;31m"
#define MSGERR "\x1b[1;33m"
#define MSGWARN "\x1b[1;35m"
#define MSGINFO "\x1b[1;32m"
#define MSGDBG "\x1b[1;37m"

#define MSGEND "\x1b[0m \n"
#else
#define MSGCRIT 
#define MSGERR 
#define MSGWARN 
#define MSGINFO 
#define MSGDBG 
#define MSGEND 
#endif

#define DRIVER_NAME "dpram_recovery"
//SHOULD be synchronized with init.c
#define MAJOR_NUM 252

//From dpram_m900 module of garnett for reference
#if 0
#define DPRAM_INTERRUPT_PORT_SIZE				2
#define DPRAM_START_ADDRESS_PHYS 				0x30000000
#define DPRAM_SHARED_BANK						0x5000000
#define DPRAM_SHARED_BANK_SIZE		    		0x1000000
#define MAX_MODEM_IMG_SIZE						0x1000000	//16 * 1024 * 1024
#define MAX_DBL_IMG_SIZE						0x5000		//20 * 1024 
#define DPRAM_SFR								0xFFF800
#define	DPRAM_SMP								DPRAM_SFR				//semaphore
#define DPRAM_MBX_AB							DPRAM_SFR + 0x20		//mailbox a -> b
#define DPRAM_MBX_BA							DPRAM_SFR + 0x40		//mailbox b -> a//
#define DPRAM_CHECK_AB				    		DPRAM_SFR + 0xA0		//check mailbox a -> b read
#define DPRAM_CHECK_BA				    		DPRAM_SFR + 0xC0		//check mailbox b -> a read
#define DPRAM_PDA2PHONE_INTERRUPT_ADDRESS		DPRAM_MBX_BA
#define DPRAM_PHONE2PDA_INTERRUPT_ADDRESS		DPRAM_MBX_AB
#endif

//Form M900
#if 0
#define DPRAM_ST_ADDR_PHYS 0x5D000000
#define DPRAM_SIZE 0x1000000
#define DPRAM_MEMORY_SIZE 0xFFF800
#define DPRAM_SEM_ADDR_OFFSET 0xFFF800
#define DPRAM_MAILBOX_AB_ADDR_OFFSET (DPRAM_SEM_ADDR_OFFSET+0x20)
#define DPRAM_MAILBOX_BA_ADDR_OFFSET (DPRAM_SEM_ADDR_OFFSET+0x40)
#endif

////
#include <mach/gpio.h>
#define GPIO_LEVEL_LOW			0
#define GPIO_LEVEL_HIGH			1

#define GPIO_PHONE_ON				S5PV210_GPJ1(0)
#define GPIO_PHONE_ON_AF			0x1

#define GPIO_PHONE_RST_N			S5PV210_GPH3(7)
#define GPIO_PHONE_RST_N_AF		0x1

#if defined(CONFIG_S5PV210_VICTORY)
#define GPIO_PDA_ACTIVE			S5PV210_MP03(3)
#elif defined(CONFIG_S5PV210_ATLAS)
#define GPIO_PDA_ACTIVE			S5PV210_GPH1(0)//S5PC11X_MP03(3)
#endif

#define GPIO_PDA_ACTIVE_AF		0x1

////
#define GPIO_PHONE_ACTIVE			S5PV210_GPH1(7)
#define GPIO_PHONE_ACTIVE_AF		0xff	

#define GPIO_ONEDRAM_INT_N		S5PV210_GPH1(3)
#define GPIO_ONEDRAM_INT_N_AF	0xff

#define IRQ_ONEDRAM_INT_N	        IRQ_EINT11
#define IRQ_PHONE_ACTIVE	        IRQ_EINT15

////
#define DPRAM_ST_ADDR_PHYS             0x30000000
#define DPRAM_SHARED_BANK				 0x05000000
#define DPRAM_SIZE                      0x1000000
#define DPRAM_MEMORY_SIZE              0xFFF800
#define DPRAM_SEM_ADDR_OFFSET          0xFFF800
#define DPRAM_MAILBOX_AB_ADDR_OFFSET (DPRAM_SEM_ADDR_OFFSET+0x20)
#define DPRAM_MAILBOX_BA_ADDR_OFFSET (DPRAM_SEM_ADDR_OFFSET+0x40)

struct dpram_dev {
    int memsize;
    int dpram_vbase;
    int dpram_sem_vaddr;
    int dpram_mailboxAB_vaddr;
    int dpram_mailboxBA_vaddr;
    struct semaphore sem;
    struct cdev cdev;
};

static struct dpram_dev *dpram;



//Thomas Ryu, from dpram.c
static inline int WRITE_TO_DPRAM_VERIFY(u32 dpram_addr, void *src, int size)
{
	return -1;
}

static inline int READ_FROM_DPRAM_VERIFY(void *dest, u32 dpram_addr, int size)
{
	return -1;
}


static void dpram_memory_barrier(void)
{
	dmb();
	dsb();
}

static volatile u32 
read_semaphore(struct dpram_dev *dev)
{
    printk(KERN_ERR "%s\n", __func__);

	dpram_memory_barrier();
	
    return *(u32*)dev->dpram_sem_vaddr;	
}

static void 
return_semaphore(struct dpram_dev *dev)
{
    printk(KERN_ERR "%s\n", __func__);
    
	if(*(u32*)dev->dpram_sem_vaddr == 1)
		*(u32*)dev->dpram_sem_vaddr = 0;

	dpram_memory_barrier();
}

static volatile u32 
get_mailboxAB(struct dpram_dev *dev)
{
    printk(KERN_ERR "%s\n", __func__);
    
	dpram_memory_barrier();
	
    return *(u32*)dev->dpram_mailboxAB_vaddr;
}

static volatile u32 
get_mailboxAB_Once(struct dpram_dev *dev)
{
	dpram_memory_barrier();
	
    return *(u32*)dev->dpram_mailboxAB_vaddr;
}


static void 
set_mailboxBA(struct dpram_dev *dev, u32 data)
{
    printk(KERN_ERR "%s\n", __func__);
    
    *(u32*)dev->dpram_mailboxBA_vaddr = data;

	dpram_memory_barrier();
}

// Below __(functions) write and reads from dpram. The paramter [addr] is
// the relative address from the dpram_vbase address.
// So addr=0 means the base of the dpram.
static void __inline __writel(struct dpram_dev *dev, int addr, int data)
{
    if(*(int*)dev->dpram_sem_vaddr != 1) {
        printk(KERN_ERR "Semaphore is in modem!!");
        return;
    }

    *(int*)(dev->dpram_vbase+addr) = data;

    printk(KERN_ERR "\n %s dpram=0x%08x, Written Data=0x%08x\n", __func__, (int)(dev->dpram_vbase+addr), data);

	dpram_memory_barrier();
}

static int __inline __writel_once(struct dpram_dev *dev, int addr, int data)
{
    if(*(int*)dev->dpram_sem_vaddr != 1) {
        printk(KERN_ERR "Semaphore is in modem!!");
        return 1;
    }

    *(int*)(dev->dpram_vbase+addr) = data;

	dpram_memory_barrier();

	return 0;
}


static void __inline __write(struct dpram_dev *dev, int addr, char *data, 
    size_t size)
{
    if(*(int*)dev->dpram_sem_vaddr != 1) {
        printk(KERN_ERR "Semaphore is in modem!!");
        return;
    }
    
    memcpy((void*)(dev->dpram_vbase+addr),data,size);

    {
      int i;
      printk(KERN_ERR "\n %s dpram=0x%08x, Written Data=%d,  ", __func__, (int)(dev->dpram_vbase+addr), size);
	  for (i = 0; i < size; i++)	
		  printk(KERN_ERR "%02x ", *((unsigned char *)data + i));
	  printk(KERN_ERR "\n");
    }


	dpram_memory_barrier();
}
static void __write_from_user(struct dpram_dev *dev, int addr, 
    char __user *data, size_t size)
{
    if(*(int*)dev->dpram_sem_vaddr != 1) {
        printk(KERN_ERR "Semaphore is in modem!!");
        return;
    }

    if(copy_from_user((void*)(dev->dpram_vbase+addr),data,size)<0) {
		printk(KERN_ERR "[%s:%d] Copy from user failed\n", __func__, __LINE__);
    }

    {
      int i;
      printk(KERN_ERR "\n %s dpram=0x%08x, Written Data=%d,  ", __func__, (int)(dev->dpram_vbase+addr), size);
      
	  //for (i = 0; i < size; i++)	
	  //	  printk(KERN_ERR "%02x ", *((unsigned char *)data + i));
	  //printk(KERN_ERR "\n");
    }


	dpram_memory_barrier();
}

static int __inline __readl(struct dpram_dev *dev, int addr)
{
	dpram_memory_barrier();
	
    if(*(int*)dev->dpram_sem_vaddr != 1) {
        printk(KERN_ERR "Semaphore is in modem!!");
        return -1;
    }

    printk(KERN_ERR "\n %s dpram=0x%08x, Read Data=0x08x%\n", __func__, (int)(dev->dpram_vbase+addr), *(int*)(dev->dpram_vbase+addr));
    
    return *(int*)(dev->dpram_vbase+addr);
}

static void __inline __read(struct dpram_dev *dev, int addr, char *data, 
    size_t size)
{
    if(data == NULL) return;

	dpram_memory_barrier();
	
    if(*(int*)dev->dpram_sem_vaddr != 1) {
        printk(KERN_ERR "Semaphore is in modem!!");
        return;
    }
    
    memcpy(data, (void*)(dev->dpram_vbase+addr), size);

    {
      int i;
      printk(KERN_ERR "\n %s dpram=0x%08x, Read Data=%d,  ", __func__, (int)(dev->dpram_vbase+addr), size);
	  for (i = 0; i < size; i++)	
		  printk(KERN_ERR "%02x ", *((unsigned char *)data + i));
	  printk(KERN_ERR "\n");
    }
    
}
static void __read_to_user(struct dpram_dev *dev, int addr, 
    char __user *data, size_t size)
{
    if(data == NULL) return;

	dpram_memory_barrier();
	
    if(*(int*)dev->dpram_sem_vaddr != 1) {
        printk(KERN_ERR "Semaphore is in modem!!");
        return;
    }

    if(copy_to_user(data, (void*)(dev->dpram_vbase+addr), size)<0) {
		printk(KERN_ERR "[%s] Copy to user failed\n", __func__);
    }

    {
      int i;
      printk(KERN_ERR "\n %s dpram=0x%08x, Read Data=%d,  ", __func__, (int)(dev->dpram_vbase+addr), size);
	  for (i = 0; i < size; i++)	
		  printk(KERN_ERR "%02x ", *((unsigned char *)data + i));
	  printk(KERN_ERR "\n");
    }
    
}

#if 0
static int 
dpram_recovery_ioremap(struct dpram_dev *dev)
{
    int i;
    
	dev->dpram_vbase = (int)ioremap_nocache(DPRAM_ST_ADDR_PHYS, DPRAM_SIZE);
	if (dev->dpram_vbase == NULL) {
		printk("failed ioremap\n");
		return -ENOENT;
	}

    printk(KERN_DEBUG "dpram vbase=0x%8x\n", dev->dpram_vbase);
    
    dev->dpram_sem_vaddr = dev->dpram_vbase + DPRAM_SEM_ADDR_OFFSET;
    
	dev->dpram_mailboxAB_vaddr = dev->dpram_vbase + DPRAM_MAILBOX_AB_ADDR_OFFSET;
	dev->dpram_mailboxBA_vaddr = dev->dpram_vbase + DPRAM_MAILBOX_BA_ADDR_OFFSET;

    dev->memsize = DPRAM_MEMORY_SIZE;

    // clear dpram 
    //Thomas Ryu    
    //*(int*)dev->dpram_sem_vaddr = 0;
    
    for(i=0;i<DPRAM_SIZE;i=i+4) {
        __writel(dev, i, 0xffffffff);
    }
	return 0;
}
#endif
//Thomas Ryu, from dpram.c
static int 
dpram_recovery_ioremap(struct dpram_dev *dev)
{
    int i;

    printk(MSGDBG "%s" MSGEND, __func__);
    
	dev->dpram_vbase = (int)ioremap_nocache(DPRAM_ST_ADDR_PHYS + DPRAM_SHARED_BANK, DPRAM_SIZE);
	if (dev->dpram_vbase == NULL) {
		printk("failed ioremap\n");
		return -ENOENT;
	}

    printk(KERN_DEBUG "dpram vbase=0x%8x\n", dev->dpram_vbase);
    
    dev->dpram_sem_vaddr = dev->dpram_vbase + DPRAM_SEM_ADDR_OFFSET;
    
	dev->dpram_mailboxAB_vaddr = dev->dpram_vbase + DPRAM_MAILBOX_AB_ADDR_OFFSET;
	dev->dpram_mailboxBA_vaddr = dev->dpram_vbase + DPRAM_MAILBOX_BA_ADDR_OFFSET;

    dev->memsize = DPRAM_MEMORY_SIZE;

    printk(MSGDBG "            dpram_vbase = 0x%08x" MSGEND, dev->dpram_vbase);
    printk(MSGDBG "        dpram_sem_vaddr = 0x%08x" MSGEND, dev->dpram_sem_vaddr);
    printk(MSGDBG "  dpram_mailboxAB_vaddr = 0x%08x" MSGEND, dev->dpram_mailboxAB_vaddr);
    printk(MSGDBG "  dpram_mailboxBA_vaddr = 0x%08x" MSGEND, dev->dpram_mailboxBA_vaddr);
    printk(MSGDBG "                memsize = 0x%08x" MSGEND, dev->memsize);    

    for(i=0;i<DPRAM_SIZE;i=i+4) {
        //__writel(dev, i, 0xffffffff);
        if( __writel_once(dev, i, 0xffffffff) ){
          printk(KERN_DEBUG "Exit during dpram initialization.\n");
          return 0;
        }
        
    }
	return 0;
}




static int
modem_pwr_status(struct dpram_dev *dev)
{

    printk(KERN_ERR "%s\n", __func__);
    
	return gpio_get_value(GPIO_PHONE_ACTIVE);
}

static int 
modem_pwroff(struct dpram_dev *dev)
{
    printk(KERN_ERR "%s\n", __func__);
	
	gpio_set_value(GPIO_PHONE_ON, GPIO_LEVEL_LOW);
	gpio_set_value(GPIO_PHONE_RST_N, GPIO_LEVEL_LOW);
	mdelay(100);
	
	printk(MSGINFO "Modem power off sem(%d)" MSGEND, *((int*)dev->dpram_sem_vaddr));
	
	return 0;
}

static int 
modem_reset(struct dpram_dev *dev)
{
    printk(KERN_ERR "%s\n", __func__);
    
    gpio_set_value(GPIO_PHONE_ON, GPIO_LEVEL_HIGH);
    msleep(50);

    gpio_set_value(GPIO_CP_RST, GPIO_LEVEL_LOW);
    msleep(100);
    gpio_set_value(GPIO_CP_RST, GPIO_LEVEL_HIGH);
    msleep(500);

    gpio_set_value(GPIO_PHONE_ON, GPIO_LEVEL_LOW); 
	
	return 0;
}

static int 
modem_pwron(struct dpram_dev *dev)
{
	int err = -1;
	int msec;

    printk(KERN_ERR "%s\n", __func__);
	
	printk(MSGINFO "Modem power on. sem(%d)" MSGEND, *((int*)dev->dpram_sem_vaddr));

    // toss semaphore to modem
	*((int*)dev->dpram_sem_vaddr)= 0x00;
    
	printk(MSGDBG "semaphore tossed to modem(%d)" MSGEND, *((int*)dev->dpram_sem_vaddr));

	gpio_set_value(GPIO_PHONE_ON, GPIO_LEVEL_HIGH);
	mdelay(50);
	gpio_set_value(GPIO_PHONE_RST_N, GPIO_LEVEL_LOW);
	mdelay(100);
	gpio_set_value(GPIO_PHONE_RST_N, GPIO_LEVEL_HIGH);
	msleep(500);
	gpio_set_value(GPIO_PHONE_ON, GPIO_LEVEL_LOW);

	for(msec=0;msec<100000;msec++) {
	    if(modem_pwr_status(dev)) {
	        err = 0;
			printk(MSGINFO "Phone is active %d" MSGEND, modem_pwr_status(dev));
			break;
	    }
		msleep(1);
	}
	
    return err;
}


static int acquire_semaphore(struct dpram_dev *dev)
{
	int retrycnt = 3;
	int wait;

retry:

	if(read_semaphore(dev) == 0x1)
		goto success;

	printk(MSGERR "Modem has Semaphore!" MSGEND);
	
	set_mailboxBA(dev, 0x5555ffff);

	modem_reset(dev);
	
	wait = 20;
	while(wait-- >= 0) {
		msleep(100);
		if(read_semaphore(dev) == 0x1)
			goto success;
	}

	if(retrycnt-- <= 0)
		return -1;
	
	goto retry;
			
success:
	printk(MSGINFO "We have Semaphore!" MSGEND);
	modem_pwroff(dev);	
	set_mailboxBA(dev, 0x0);
	return 0;
}

int
dpram_recovery_write_modem_firmware(
    struct dpram_dev *dev, char __user *firmware, int size)
{
    int ret = 0;

    printk(KERN_ERR "%s\n", __func__);

	acquire_semaphore(dev);
    // write firmware size
    __writel(dev, DPRAM_FIRMWARE_SIZE_ADDR, size);
    
    // write firmware
    __write_from_user(dev, DPRAM_FIRMWARE_ADDR, firmware, size);
    
	return ret;
}

int
dpram_recovery_start_firmware_update(struct dpram_dev *dev)
{
	int err = 0;
	int msec = 0;
	u32 val;

    printk(KERN_ERR "%s\n", __func__);

	acquire_semaphore(dev);
	
    // write boot magic
    __writel(dev, DPRAM_BOOT_MAGIC_ADDR, DPRAM_BOOT_MAGIC_RECOVERY_FOTA);
    __writel(dev, DPRAM_BOOT_TYPE_ADDR, DPRAM_BOOT_TYPE_DPRAM_DELTA);

    // boot modem
    err = modem_pwron(dev);
    if(err < 0) {
        printk(KERN_ERR MSGCRIT "Modem power on failed. phone active is %d" MSGEND, 
            modem_pwr_status(dev));
		goto out;
    }

	// clear mailboxBA
	set_mailboxBA(dev, 0xFFFFFFFF);
	
	// wait for job sync message
	while(true) {
		val = get_mailboxAB_Once(dev);
		if((val&STATUS_JOB_MAGIC_M) == STATUS_JOB_MAGIC_CODE) {
			err = 0;
			break;
		}
		
		msleep(1);
		if(++msec > 100000) {
			err = -2;
			printk(MSGCRIT "Failed to sync with modem (%x)" MSGEND, val);
			goto out;
		}

		if((msec%1000) == 0) {
			printk(MSGINFO "Waiting for sync message... 0x%08x (pwr:%s)" MSGEND,
				val, modem_pwr_status(dev)?"ON":"OFF");
		}
	}

	if(err == 0) {
		printk(MSGINFO "Modem ready to start the firmware update" MSGEND);
	
		// let modem start the job
		set_mailboxBA(dev, STATUS_JOB_MAGIC_CODE);
		// if we have the semaphore, return it.
		return_semaphore(dev);
	}
	
out:
    return err;

}

static int
dpram_recovery_check_status(struct dpram_dev *dev, 
	int __user *pct, char __user *msg)
{
	u32 status;
	static u32 prev_status = 0;
	int percent = 0;
	int err = 0;
	char buf[DPRAM_MODEM_MSG_SIZE];
	int debugprint = false;
	int wait = 0;

    //printk(KERN_ERR "%s\n", __func__);
	
    // check mailboxAB for the modem status
    status = get_mailboxAB(dev);

	debugprint = (prev_status != status);
	
	if(debugprint) printk("Job status : 0x%08x (pwr:%s)\n", status,
		modem_pwr_status(dev)?"ON":"OFF");

	if((status & STATUS_JOB_MAGIC_M) != STATUS_JOB_MAGIC_CODE) {
		if(debugprint) printk("Job not accepted yet\n");
		err = 1;
		percent = 0;
		strncpy(buf, "Job not accepted yet", DPRAM_MODEM_MSG_SIZE);
		goto out;
	}

	if((status & STATUS_JOB_STARTED_M) == STATUS_JOB_STARTED_M) {
		return_semaphore(dev);
		percent = status & STATUS_JOB_PROGRESS_M;
		if(debugprint) printk("Job progress pct=%d\n", percent);
		err = 3;
	}
	else {
		percent = 0;
		if(debugprint) printk("Job NOT started yet...\n");
		err = 2;
	}

	if(status & STATUS_JOB_ENDED_M) {
		percent = status & STATUS_JOB_PROGRESS_M;
		
		// wait till we have semaphore
		printk(MSGWARN "Wait for semaphore" MSGEND);
		while(true) {
			msleep(10);
			if(read_semaphore(dev) == 1) {
				printk(MSGINFO "We have semaphore" MSGEND);
				break;
			}

			if(wait++ > 1000) {
				printk(MSGWARN "Proceeding without semaphore" MSGEND);
				break;
			}
			
		}
		
		__read(dev, DPRAM_MODEM_STRING_MSG_ADDR, buf, DPRAM_MODEM_MSG_SIZE);

		if(status & STATUS_JOB_ERROR_M) {
			err = -1;
			printk(KERN_ERR "Job ended with error msg : %s\n", buf);
		}
		else if((status & STATUS_JOB_COMPLETE_M) == STATUS_JOB_COMPLETE_M) {
			err = 0;
			printk(KERN_DEBUG "Job completed successfully : %s\n", buf);
		}
	}
out:
	prev_status = status;
	if(copy_to_user((void*)pct, (void*)&percent, sizeof(int))<0) {
		printk(KERN_ERR "[%s:%d] Copy to user failed\n", __func__, __LINE__);
	}
	
	if(copy_to_user((void*)msg, (void*)&buf[0], DPRAM_MODEM_MSG_SIZE)<0) {
		printk(KERN_ERR "[%s:%d] Copy to user failed\n", __func__, __LINE__);
	}
	
	return err;
}

static ssize_t 
dpram_recovery_read(struct file *filp, char __user *buff, 
    size_t count, loff_t *fpos)
{
    struct dpram_dev *dev = filp->private_data;
	loff_t ifpos;

    printk(KERN_ERR "%s\n", __func__);

	// let user read/write from DPRAM_FIRMWARE_ADDR this address onwards..
	ifpos = *fpos + DPRAM_FIRMWARE_ADDR;

    if(ifpos > dev->memsize) {
        printk(KERN_ERR "Request offset overflow!\n");
        return -EOVERFLOW;
    }
    
    __read_to_user(dev, (int)ifpos, buff, count);
}

static ssize_t 
dpram_recovery_write(struct file *filp, const char __user *buff,
    size_t size, loff_t *fpos)
{
    struct dpram_dev *dev = filp->private_data;
	loff_t ifpos;

    printk(KERN_ERR "%s\n", __func__);

	// let user read/write from DPRAM_FIRMWARE_ADDR this address onwards..
	ifpos = *fpos + DPRAM_FIRMWARE_ADDR;
	
    if(ifpos > dev->memsize) {
        printk(KERN_ERR "Request offset overflow!\n");
        return -EOVERFLOW;
    }
    
    __write_from_user(dev, (int)ifpos, (char*)buff, size);
}

static int
dpram_recovery_ioctl(struct inode *inode, struct file *filp,
	   unsigned int cmd, unsigned long arg)
    
{
    struct dpram_dev *dev;
	int ret = 0;

    printk(MSGDBG "%s" MSGEND, __func__);
    printk(KERN_ERR "%s\n", __func__);
    
    dev = container_of(inode->i_cdev, struct dpram_dev, cdev);
    
	switch (cmd) {
		
        case IOCTL_ST_FW_UPDATE:
	        printk(KERN_ERR "%s IOCTL_ST_FW_UPDATE\n", __func__);
			
			if(arg == NULL) {
				printk(MSGWARN "Firmware should be written prior to this call" MSGEND);
			}
			else {
				struct dpram_firmware fw;
				
    			if(copy_from_user((void *)&fw, (void *)arg, sizeof(fw)) < 0) {
                    printk("[IOCTL_ST_FW_UPDATE]copy from user failed!");
                    ret = -1;
    			}
                
    			if (dpram_recovery_write_modem_firmware(dev, fw.firmware, fw.size) < 0) {
    				printk("firmware write failed\n");
    				ret = -2;
    			}
			}
			
			if(dpram_recovery_start_firmware_update(dev) < 0) {
				printk("Firmware update failed\n");
				ret = -3;
			}
			break;

    	case IOCTL_CHK_STAT:
			{
				struct stat_info *pst;

   		       // printk(KERN_ERR "%s IOCTL_CHK_STAT\n", __func__);
   		        
				pst = (struct stat_info*)arg;
	    		ret = dpram_recovery_check_status(dev, &(pst->pct), pst->msg);
    		}
    		break;
			
        case IOCTL_MOD_PWROFF:
	        printk(KERN_ERR "%s IOCTL_MOD_PWROFF\n", __func__);        
			modem_pwroff(dev);
			break;
			
        default:
            printk(KERN_ERR "Unknown command");
			break;
	}

	return ret;
}

static int 
dpram_recovery_open(struct inode *inode, struct file *filp)
{
    struct dpram_dev *dev;
    
    printk(KERN_ERR "%s\n", __func__);
    
    dev = container_of(inode->i_cdev, struct dpram_dev, cdev);
    filp->private_data = (void*)dev;

    return 0;
}

static int 
dpram_recovery_release(struct inode *inode, struct file *filp)
{
    printk(KERN_DEBUG "dpram recovery device released.\n");
    return 0;
}

static struct file_operations dpram_recovery_fops = {
    .owner = THIS_MODULE,
    .read = dpram_recovery_read,
    .write = dpram_recovery_write,
    .ioctl = dpram_recovery_ioctl,
    .open = dpram_recovery_open,
    .release = dpram_recovery_release,
};

static irqreturn_t
dpram_irq_handler(int irq, void *dev_id)
{
	u32 mbAB;
    printk(KERN_ERR "%s\n", __func__);
    
	mbAB = get_mailboxAB(dpram);
	printk(MSGDBG "Dpram irq received : 0x%8x" MSGEND, mbAB);

	return IRQ_HANDLED;
}

static irqreturn_t
phone_active_irq_handler(int irq, void *dev_id)
{
    printk(KERN_ERR "%s\n", __func__);
    
	printk(MSGDBG "Phone active irq : %d" MSGEND, modem_pwr_status(dpram));
	return IRQ_HANDLED;
}

static int 
register_interrupt_handler(void)
{
	int err = 0;

    printk(KERN_ERR "%s\n", __func__);
    
	err = request_irq(IRQ_ONEDRAM_INT_N, dpram_irq_handler, 
						IRQF_DISABLED, "Dpram irq", NULL);

	if (err) {
		printk("DPRAM interrupt handler failed.\n");
		return -1;
	}

	err = request_irq(IRQ_PHONE_ACTIVE, phone_active_irq_handler, 
						IRQF_DISABLED, "Phone Active", NULL);

	if (err) {
		printk("Phone active interrupt handler failed.\n");
		free_irq(IRQ_ONEDRAM_INT_N, NULL);
		return -1;
	}

	printk(MSGINFO "Interrupt handler registered." MSGEND);
	
	return err;
}

static int
hw_init(void)
{
    printk(KERN_ERR "%s\n", __func__);
    
	s3c_gpio_cfgpin(GPIO_PHONE_ACTIVE, S3C_GPIO_SFN(GPIO_PHONE_ACTIVE_AF));
	s3c_gpio_setpull(GPIO_PHONE_ACTIVE, S3C_GPIO_PULL_NONE); 
	set_irq_type(IRQ_PHONE_ACTIVE, IRQ_TYPE_EDGE_BOTH);

	s3c_gpio_cfgpin(GPIO_ONEDRAM_INT_N, S3C_GPIO_SFN(GPIO_ONEDRAM_INT_N_AF));
	s3c_gpio_setpull(GPIO_ONEDRAM_INT_N, S3C_GPIO_PULL_NONE); 
	set_irq_type(IRQ_ONEDRAM_INT_N, IRQ_TYPE_EDGE_FALLING);

    //Thomas Ryu
	if (gpio_is_valid(GPIO_PHONE_ON)) {
		if (gpio_request(GPIO_PHONE_ON, "dpram/GPIO_PHONE_ON"))
			printk(KERN_ERR "Filed to request GPIO_PHONE_ON!\n");
		gpio_direction_output(GPIO_PHONE_ON, GPIO_LEVEL_LOW);
	}
	s3c_gpio_setpull(GPIO_PHONE_ON, S3C_GPIO_PULL_NONE); 
	gpio_set_value(GPIO_PHONE_ON, GPIO_LEVEL_LOW);

	if (gpio_is_valid(GPIO_PHONE_RST_N)) {
		if (gpio_request(GPIO_PHONE_RST_N, "dpram/GPIO_PHONE_RST_N"))
			printk(KERN_ERR "Filed to request GPIO_PHONE_RST_N!\n");
		gpio_direction_output(GPIO_PHONE_RST_N, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_PHONE_RST_N, S3C_GPIO_PULL_NONE); 

	if (gpio_is_valid(GPIO_PDA_ACTIVE)) {
		if (gpio_request(GPIO_PDA_ACTIVE, "dpram/GPIO_PDA_ACTIVE"))
			printk(KERN_ERR "Filed to request GPIO_PDA_ACTIVE!\n");
		gpio_direction_output(GPIO_PDA_ACTIVE, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_PDA_ACTIVE, S3C_GPIO_PULL_NONE); 


	return 0;
}

static int __init 
dpram_recovery_init(void)
{
    int err = 0;
    int devno = MKDEV(MAJOR_NUM, 0);

    printk(KERN_ERR "%s\n", __func__);
    
    dpram = kmalloc(sizeof(struct dpram_dev), GFP_KERNEL);
        
    if(dpram == NULL) {
        printk(KERN_ERR "dpram recovery device allocation failed!!\n");
        err = -ENOMEM;
        goto out;
    }

    if(dpram_recovery_ioremap(dpram)<0) {
        printk(KERN_ERR "Dpram recovery ioremap failed!\n");
        goto out_err1;
    }

	hw_init();
	
	register_interrupt_handler();
	
    if(register_chrdev_region(devno, 1, DRIVER_NAME) < 0) {
        printk(KERN_DEBUG "chrdev region register failed\n");
        err = -1;
        goto out_err1;
    }
        
    cdev_init(&dpram->cdev, &dpram_recovery_fops);
    dpram->cdev.owner = THIS_MODULE;
    dpram->cdev.ops = &dpram_recovery_fops;
        
    err = cdev_add(&dpram->cdev, devno, 1);
        
    if(err < 0) {
        printk(KERN_ERR "Dpram recovery device failed to register!\n");
        goto out_err1;
    }

    
    printk(KERN_ERR "%s %d\n", __func__, __LINE__);
    
out:
	return err;

out_err2:
    cdev_del(&dpram->cdev);
    
out_err1:
    kfree(dpram);
    return err;
    
}

static void __exit 
dpram_recovery_exit(void)
{
    cdev_del(&dpram->cdev);
    kfree(dpram);
	printk (KERN_DEBUG "Dpram recovery char device is unregistered\n");
}

module_init(dpram_recovery_init);
module_exit(dpram_recovery_exit);

MODULE_AUTHOR("Samsung Electronics Co., LTD");
MODULE_DESCRIPTION("Onedram Device Driver for recovery.");
MODULE_LICENSE("GPL");
