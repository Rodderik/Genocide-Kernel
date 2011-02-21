#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/proc_fs.h>
#include <linux/ethtool.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <asm/byteorder.h>
#include <asm/uaccess.h>

#include <linux/delay.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
#include <linux/i2c/pmic.h>
#endif

#include <plat/sdhci.h>
#include <plat/devs.h>
#include <linux/mmc/host.h>

#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
#include <mach/instinctq.h>	//cky 20100111 headers for quattro
#elif defined (CONFIG_MACH_ARIES)
#include <mach/gpio-jupiter.h>	// Yongha for Victory WiMAX 20100208
#endif
#include <linux/fs.h> //tsh.park 20100930

#include "../wimax/wimax_plat.h" //cky 20100224
#include "wimaxgpio.h"
#include "wimaxproc.h"
#include "wimax_i2c.h"

extern void usb_switch_mode(int);	//cky 20100209 USB path switch
extern unsigned int system_rev;	//cky 20100527

#ifndef FILE_DEVICE_UNKNOWN
#define FILE_DEVICE_UNKNOWN 0x89	// 0x22
#endif

#define DRIVER_AUTHOR "<sangam.swamy@samsung.com>"
#define DRIVER_DESC "Samsung WiMax SDIO GPIO Device Driver"
#define WXMGPIOVERSION "0.1"

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
#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )	// file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )	// file & pipe

#define	CONTROL_IOCTL_WIMAX_POWER_CTL				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x821, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_EEPROM_PATH			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x837, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_MODE_CHANGE			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x838, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_EEPROM_DOWNLOAD		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x839, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_SLEEP_MODE				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83A, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_WRITE_REV				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83B, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_CHECK_CERT				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83C, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	CONTROL_IOCTL_WIMAX_CHECK_CAL				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x83D, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define	WIMAX_GPIO_DRIVER_VERSION_STRING  "1.0.1"	
#define SWMXGPIOMAJOR	234

static const char driver_name[] = "WIMAXGPIO";
static char charName[] = "swmxctl";
static unsigned int	gDeviceOpen = 0;

unsigned long DatadwValue = 0;
unsigned long ControldwValue = 0;
unsigned int 	g_bUSBPath = USB_PATH_PDA;
unsigned int 	g_dwLineState = SDIO_MODE;	//default
unsigned int	g_dwSleepMode = 0;	// 0 for VI, 1 for IDLE
unsigned int 	g_dwPowerState = 0;	// 0 for power off, 1 for power on

extern int s3c_bat_use_wimax(int onoff);	//cky 20100624

//cky 20100310 switch USB path
static void switch_usb_wimax(void)
{
	//cky 20100416 do not change usb mode //usb_switch_mode(3); // USB path WiMAX
}

static void switch_usb_pda(void)
{
	//cky 20100416 do not change usb mode //usb_switch_mode(1); // USB path PDA
}

int wimax_timeout_multiplier;
EXPORT_SYMBOL(wimax_timeout_multiplier);

//cky 20100527 switch EEPROM ap/wimax
void switch_eeprom_ap(void)
{
	if (system_rev < 8)
	{
		s3c_gpio_cfgpin(I2C_SEL, S3C_GPIO_SFN(1));
		gpio_set_value(I2C_SEL, GPIO_LEVEL_HIGH);
		s3c_gpio_slp_cfgpin(I2C_SEL, S3C_GPIO_SLP_OUT1);	//cky 20100511 add for AP sleep
	}
	else
	{
		s3c_gpio_cfgpin(I2C_SEL, S3C_GPIO_SFN(1));
		gpio_set_value(I2C_SEL, GPIO_LEVEL_LOW);
		s3c_gpio_slp_cfgpin(I2C_SEL, S3C_GPIO_SLP_OUT0);	//cky 20100511 add for AP sleep
	}

	msleep(10);
}

void switch_eeprom_wimax(void)
{
	if (system_rev < 8)
	{
		s3c_gpio_cfgpin(I2C_SEL, S3C_GPIO_SFN(1));
		gpio_set_value(I2C_SEL, GPIO_LEVEL_LOW);
		s3c_gpio_slp_cfgpin(I2C_SEL, S3C_GPIO_SLP_OUT0);	//cky 20100511 add for AP sleep
	}
	else
	{
		s3c_gpio_cfgpin(I2C_SEL, S3C_GPIO_SFN(1));
		gpio_set_value(I2C_SEL, GPIO_LEVEL_HIGH);
		s3c_gpio_slp_cfgpin(I2C_SEL, S3C_GPIO_SLP_OUT1);	//cky 20100511 add for AP sleep
	}
	
	msleep(10);
}

#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
static void s3c_WIMAX_SDIO_on(void)
{
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(ELDO5, &reg_buff)) {
		pr_info("[WGPIO]%s: WIMAX SDIO 2.6V on(%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(ELDO5, 1);
	}
}

static void s3c_WIMAX_SDIO_off(void)
{
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(ELDO5, &reg_buff)) {
		pr_info("[WGPIO] %s: WIMAX SDIO 2.6V off(%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(ELDO5, 0);
	}
}
#endif	// CONFIG_MACH_QUATTRO

int g_isWiMAXProbe;	//cky 20100511
EXPORT_SYMBOL(g_isWiMAXProbe);

int gpio_wimax_poweron (void)
{
	int try_count;
	
	if(gpio_get_value(WIMAX_EN))  // sangam enable if it is low..
	{
		DumpDebug("Already Wimax powered ON");
		return -1;	// already power on
	}
	DumpDebug("Wimax power ON");
	g_dwPowerState = 1;

	if (g_dwLineState != SDIO_MODE)
	{
		DumpDebug("WiMAX USB Enable");

		switch_usb_wimax();

		s3c_gpio_cfgpin(WIMAX_USB_EN, S3C_GPIO_SFN(1));
		gpio_set_value(WIMAX_USB_EN, GPIO_LEVEL_HIGH);
		
		s3c_bat_use_wimax(1);	//cky 20100624
	}
	else
	{
		DumpDebug("SDIO MODE");

		switch_usb_pda();
	}

	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(WIMAX_RESET, S3C_GPIO_SFN(1));

	// EEPROM switch to WiMAX
	switch_eeprom_wimax();

	//s3c_WIMAX_SDIO_on();

	gpio_set_value(WIMAX_EN, GPIO_LEVEL_HIGH);

	g_isWiMAXProbe = 0;
	try_count = 0;
	while (1)
	{
		DumpDebug("RESET");
		gpio_set_value(WIMAX_RESET, GPIO_LEVEL_LOW);
		msleep(10);
		gpio_set_value(WIMAX_RESET, GPIO_LEVEL_HIGH);
		msleep(500);  //sangam dbg : Delay important for probe
		
		DumpDebug("PRESENCE CHANGE");
		sdhci_s3c_force_presence_change(&DEVICE_HSMMC);
		msleep(500);
		if (!g_isWiMAXProbe)
		{
			DumpDebug("PRESENCE CHANGE BACK");

			if (try_count++ > 3)
			{
				DumpDebug("PROBE FAIL");
				return -2;
			}

		}
		else
		{
			DumpDebug("PROBE OK");
			return 0;
		}
	}
}
EXPORT_SYMBOL(gpio_wimax_poweron);

int gpio_wimax_poweroff (void)
{
	if(!gpio_get_value(WIMAX_EN))  // sangam enable if it is low..
	{
		DumpDebug("Already Wimax powered OFF");
		return -3;	// already power off
	}
	DumpDebug("Wimax power OFF");

	g_dwPowerState = 0;
	wimax_timeout_multiplier = 1;	//sumanth 20100702
	
	msleep(200);
	DumpDebug("WiMAX GPIO LOW");
	
	// EEPROM switch to AP
	switch_eeprom_ap();
	
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(WIMAX_RESET, S3C_GPIO_SFN(1));
	s3c_gpio_cfgpin(WIMAX_USB_EN, S3C_GPIO_SFN(1));

	//s3c_gpio_setpull(GPIO_WIMAX_RESET_N, S3C_GPIO_PULL_NONE);
	gpio_set_value(WIMAX_RESET, GPIO_LEVEL_LOW);
	gpio_set_value(WIMAX_USB_EN, GPIO_LEVEL_LOW);

	switch_usb_pda();

	if (g_dwLineState != SDIO_MODE)
	{
		s3c_bat_use_wimax(0);	//cky 20100624
	}

	gpio_set_value(WIMAX_EN, GPIO_LEVEL_LOW);

	sdhci_s3c_force_presence_change(&DEVICE_HSMMC);

	msleep(500);

	//s3c_WIMAX_SDIO_off();

	return 0;
}
EXPORT_SYMBOL(gpio_wimax_poweroff);

// get Sleep Mode
int getWiMAXSleepMode(void)
{
	return g_dwSleepMode;
}
EXPORT_SYMBOL(getWiMAXSleepMode);

int getWiMAXPowerState(void)
{
	return g_dwPowerState;
}
EXPORT_SYMBOL(getWiMAXPowerState);

static int
swmxdev_open (struct inode * inode, struct file * file)
{
	ENTER;
	if(gDeviceOpen > 0) {
		DumpDebug("Device in Use by %d", gDeviceOpen);
		//cky 20100614 workaround return  -EEXIST;
	}

	gDeviceOpen = current->tgid;

	LEAVE;
	return 0;
}

static int
swmxdev_release (struct inode * inode, struct file * file)
{
	ENTER;
	gDeviceOpen = 0;
	LEAVE;
	return 0;
}

static int
swmxdev_ioctl (struct inode * inode, struct file * file, u_int cmd, u_long arg)
{
	int ret = 0;
	
	ENTER;
	DumpDebug("CMD: %x, PID: %d",cmd, current->tgid);

	switch(cmd)
	{
		case CONTROL_IOCTL_WIMAX_EEPROM_PATH:
		{
			gpio_wimax_poweron();
			DumpDebug("CONTROL_IOCTL_WIMAX_EEPROM_PATH..");
			if(((unsigned char *)arg)[0] == 0) {
				DumpDebug("changed to AP mode....");
			}
			else {
				DumpDebug("changed to WIMAX EEPROM mode....");
			}
			break;
		}
		case CONTROL_IOCTL_WIMAX_POWER_CTL:
		{
			DumpDebug("CONTROL_IOCTL_WIMAX_POWER_CTL..");
			if(((unsigned char *)arg)[0] == 0) {
				ret = gpio_wimax_poweroff();
			}
			else {
				ret = gpio_wimax_poweron();
			}
			break;
		}
		case CONTROL_IOCTL_WIMAX_MODE_CHANGE:
		{
			DumpDebug("CONTROL_IOCTL_WIMAX_MODE_CHANGE to %d..", ((unsigned char *)arg)[0]);

#if 0	//cky 20100416 do not check prev. mode
			if (((unsigned char *)arg)[0] == g_dwLineState) {
				DumpDebug("IOCTL Already in mode %d", g_dwLineState);
				return 0;
			}
#endif
			if( (((unsigned char *)arg)[0]  < 0) || (((unsigned char *)arg)[0]  > AUTH_MODE) ) {
				DumpDebug("Wrong mode %d", ((unsigned char *)arg)[0]);
				return 0;
			}

			gpio_wimax_poweroff();
			//msleep(100);

			g_dwLineState = ((unsigned char *)arg)[0];

			ret = gpio_wimax_poweron();
		break;		
		}
		case CONTROL_IOCTL_WIMAX_EEPROM_DOWNLOAD:
		{
			DumpDebug("CONTROL_IOCTL_WIMAX_EEPROM_DOWNLOAD");
			gpio_wimax_poweroff();

			wimax_timeout_multiplier = 3;	//sumanth 20100702
			
			WIMAX_BootInit();
			break;
		}
		case CONTROL_IOCTL_WIMAX_SLEEP_MODE:
		{
			if (((unsigned char *)arg)[0] == 0)	// AP sleep -> WiMAX VI
			{
				DumpDebug("AP SLEEP: WIMAX VI");
				g_dwSleepMode = 0;	// VI
			}
			else
			{
				DumpDebug("AP SLEEP: WIMAX IDLE");
				g_dwSleepMode = 1;	// IDLE
			}

			break;
		}
		case CONTROL_IOCTL_WIMAX_WRITE_REV:
		{
			DumpDebug("CONTROL_IOCTL_WIMAX_WRITE_REV");
			gpio_wimax_poweroff();
			
			WIMAX_Write_Rev();
			break;
		}
		case CONTROL_IOCTL_WIMAX_CHECK_CERT:
		{
			DumpDebug("CONTROL_IOCTL_WIMAX_CHECK_CERT");
			gpio_wimax_poweroff();

			ret = WIMAX_Check_Cert();
			
			break;
		}
		case CONTROL_IOCTL_WIMAX_CHECK_CAL:
		{
			DumpDebug("CONTROL_IOCTL_WIMAX_CHECK_CAL");
			gpio_wimax_poweroff();

			ret = WIMAX_Check_Cal();

			break;
		}
	}
	
	LEAVE;
	return ret;
}

static ssize_t
swmxdev_read (struct file * file, char * buf, size_t count, loff_t *ppos)
{
	ENTER;
	LEAVE;
	return 0;	
}

static ssize_t
swmxdev_write (struct file * file, const char * buf, size_t count, loff_t *ppos)
{
	ENTER;
	LEAVE;
	return 0;
}

static struct file_operations swmx_fops = {
	owner:		THIS_MODULE,
	open:		swmxdev_open,
	release:	swmxdev_release,
	ioctl:		swmxdev_ioctl,
	read:		swmxdev_read,
	write:		swmxdev_write,
};

static int __init wmxGPIO_init(void)
{
	int error = 0;
	pr_info("[WGPIO] %s: %s, " DRIVER_DESC "\n", driver_name, WIMAX_GPIO_DRIVER_VERSION_STRING);

	DumpDebug("IOCTL SDIO GPIO driver installing");
	error = register_chrdev(SWMXGPIOMAJOR, charName, &swmx_fops);

	if(error < 0) {
		DumpDebug("WiBroB_drv: register_chrdev() failed");
		return error;
	}

	g_dwSleepMode = 0;	// default sleep mode VI
	
	WiMAX_Proc_Init();

	g_dwPowerState = 0;
	wimax_timeout_multiplier = 1;	//sumanth 20100702
	
	WiMAX_EEPROM_Fix();
	// wimax_i2c_con
	if (system_rev < 8)
	{
		s3c_gpio_cfgpin(I2C_SEL, S3C_GPIO_SFN(1));
		gpio_set_value(I2C_SEL, GPIO_LEVEL_LOW);			// active path to cmc730
		s3c_gpio_slp_cfgpin(I2C_SEL, S3C_GPIO_SLP_OUT1);	// suspend path to ap
	}
	else
	{
		s3c_gpio_cfgpin(I2C_SEL, S3C_GPIO_SFN(1));
		gpio_set_value(I2C_SEL, GPIO_LEVEL_HIGH);			// active path to cmc730
		s3c_gpio_slp_cfgpin(I2C_SEL, S3C_GPIO_SLP_OUT0);	// suspend path to ap
	}

	return error;
}

static void __exit wmxGPIO_exit(void)
{
	DumpDebug("SDIO GPIO driver Uninstall");
	unregister_chrdev(SWMXGPIOMAJOR, charName);

	WiMAX_Proc_Deinit();
}

module_init(wmxGPIO_init);
module_exit(wmxGPIO_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

