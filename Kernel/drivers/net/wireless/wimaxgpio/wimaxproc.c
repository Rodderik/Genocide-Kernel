#include "wimaxproc.h"

#include <linux/proc_fs.h>
#include "wimaxgpio.h"
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/module.h>
#include <linux/delay.h>

#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
#include <mach/instinctq.h>	//cky 20100111 headers for quattro
#elif defined (CONFIG_MACH_ARIES)
#include <mach/gpio-jupiter.h>	// Yongha for Victory WiMAX 20100208
#endif

#include "wimax_i2c.h"
#include "../wimax/wimax_plat.h"

extern unsigned int 	g_dwLineState;
extern unsigned int		g_dwSleepMode;

unsigned int g_dumpLogs;

// proc entry
static struct proc_dir_entry *g_proc_wmxmode_dir = NULL;
static struct proc_dir_entry *g_proc_wmxuart = NULL;
static struct proc_dir_entry *g_proc_eeprom = NULL;
static struct proc_dir_entry *g_proc_onoff = NULL;
static struct proc_dir_entry *g_proc_dump = NULL;
static struct proc_dir_entry *g_proc_sleepmode = NULL;

static int wimax_proc_mode(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int len;
	ENTER;
	len = sprintf(buf, "%c\n", g_dwLineState);
	if(len > 0) *eof = 1;
	LEAVE;
	return len;
}

static int wimax_proc_power(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
	int len;
	ENTER;
	len = sprintf(buf, "%c\n", gpio_get_value(WIMAX_EN));
	if(len > 0) *eof = 1;
	LEAVE;
	return len;
}

// switch UART path
static int proc_read_wmxuart(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int val = 0;

	val = gpio_get_value(WIMAX_EN);
	DumpDebug("WIMAX_EN: %d", val);

	val = gpio_get_value(WIMAX_RESET);
	DumpDebug("WIMAX_RESET: %d", val);

	val = gpio_get_value(WIMAX_CON0);
	DumpDebug("WIMAX_CON0: %d", val);

	val = gpio_get_value(WIMAX_CON1);
	DumpDebug("WIMAX_CON1: %d", val);

	val = gpio_get_value(WIMAX_CON2);
	DumpDebug("WIMAX_CON2: %d", val);

	val = gpio_get_value(WIMAX_WAKEUP);
	DumpDebug("WIMAX_WAKEUP: %d", val);

	val = gpio_get_value(WIMAX_INT);
	DumpDebug("WIMAX_INT: %d", val);

	val = gpio_get_value(WIMAX_IF_MODE0);
	DumpDebug("WIMAX_IF_MODE0: %d", val);

	val = gpio_get_value(WIMAX_IF_MODE1);
	DumpDebug("WIMAX_IF_MODE1: %d", val);

	val = gpio_get_value(WIMAX_USB_EN);
	DumpDebug("WIMAX_USB_EN: %d", val);

	val = gpio_get_value(I2C_SEL);
	DumpDebug("I2C_SEL: %d", val);

#if defined (CONFIG_MACH_QUATTRO)

#elif defined (CONFIG_MACH_ARIES)
	val = gpio_get_value(GPIO_UART_SEL);
	DumpDebug("UART_SEL: %d", val);

	val = gpio_get_value(GPIO_UART_SEL1);
	DumpDebug("UART_SEL1: %d", val);
#endif

	return 0;
}

static int proc_write_wmxuart(struct file *foke, const char *buffer, unsigned long count, void *data)
{
	if (buffer == NULL) return 0;
	
	if (buffer[0] == '0')
	{
		//printk("<1>UART Path: AP!\n");
#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
		s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_SFN(1));
		s3c_gpio_cfgpin(GPIO_UART_SEL2, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH);	// TRUE for AP, FALSE for WIMAX
		gpio_set_value(GPIO_UART_SEL2, GPIO_LEVEL_HIGH);
#elif defined (CONFIG_MACH_ARIES)
		s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_SFN(1));
		s3c_gpio_cfgpin(GPIO_UART_SEL1, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH);
		gpio_set_value(GPIO_UART_SEL1, GPIO_LEVEL_LOW);
#endif
	}
	else if (buffer[0] == '1')
	{
		//printk("<1>UART Path: WiMAX!\n");
#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
		s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_SFN(1));
		s3c_gpio_cfgpin(GPIO_UART_SEL2, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_LOW);	// TRUE for AP, FALSE for WIMAX
		gpio_set_value(GPIO_UART_SEL2, GPIO_LEVEL_HIGH);
#elif defined (CONFIG_MACH_ARIES)
		s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_SFN(1));
		s3c_gpio_cfgpin(GPIO_UART_SEL1, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_LOW);
		gpio_set_value(GPIO_UART_SEL1, GPIO_LEVEL_HIGH);
#endif
	}

	return 0;
}

// write EEPROM
static int proc_read_eeprom(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	DumpDebug("Write EEPROM!!");
	WIMAX_BootInit();
	return 0;
}

static int proc_write_eeprom(struct file *foke, const char *buffer, unsigned long count, void *data)
{
		//DumpDebug("count: %d, buffer: %s", count, buffer);
	
	if (count != 5) return count;

	if (strncmp(buffer, "wb00", 4) == 0)
	{
		DumpDebug("Write EEPROM!!");
		WIMAX_BootInit();
	}
	else if (strncmp(buffer, "rb00", 4) == 0)
	{
		DumpDebug("Read Boot!!");
		WiMAX_ReadBoot();
	}
	else if (strncmp(buffer, "re00", 4) == 0)
	{
		DumpDebug("Read EEPROM!!");
		WiMAX_ReadEEPROM();
	}
	else if (strncmp(buffer, "ee00", 4) == 0)
	{
		DumpDebug("Erase EEPROM!!");
		WiMAX_RraseEEPROM();
	}
	else if (strncmp(buffer, "rcal", 4) == 0)
	{
		DumpDebug("Check Cal!!");
		WIMAX_Check_Cal();
	}
	else if (strncmp(buffer, "rcer", 4) == 0)
	{
		DumpDebug("Check Cert!!");
		WIMAX_Check_Cert();
	}
	else if (strncmp(buffer, "wrev", 4) == 0)
	{
		DumpDebug("Write Rev!!");
		WIMAX_Write_Rev();
	}
	else if (strncmp(buffer, "ons0", 4) == 0)
	{
		DumpDebug("Power On - SDIO MODE!!");
		gpio_wimax_poweroff();
		msleep(10);
		g_dwLineState = SDIO_MODE;
		gpio_wimax_poweron();
	}
	else if (strncmp(buffer, "off0", 4) == 0)
	{
		DumpDebug("Power Off!!");
		gpio_wimax_poweroff();
	}
	else if (strncmp(buffer, "wu00", 4) == 0)
	{
		DumpDebug("WiMAX UART!!");
		//printk("<1>UART Path: WiMAX!\n");
#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
		s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_SFN(1));
		s3c_gpio_cfgpin(GPIO_UART_SEL2, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_LOW);	// TRUE for AP, FALSE for WIMAX
		gpio_set_value(GPIO_UART_SEL2, GPIO_LEVEL_HIGH);
#elif defined (CONFIG_MACH_ARIES)
		s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_SFN(1));
		s3c_gpio_cfgpin(GPIO_UART_SEL1, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_LOW);
		gpio_set_value(GPIO_UART_SEL1, GPIO_LEVEL_HIGH);
#endif
	}
	else if (strncmp(buffer, "au00", 4) == 0)
	{
		DumpDebug("AP UART!!");
		//printk("<1>UART Path: AP!\n");
#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
		s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_SFN(1));
		s3c_gpio_cfgpin(GPIO_UART_SEL2, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH); // TRUE for AP, FALSE for WIMAX
		gpio_set_value(GPIO_UART_SEL2, GPIO_LEVEL_HIGH);
#elif defined (CONFIG_MACH_ARIES)
		s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_SFN(1));
		s3c_gpio_cfgpin(GPIO_UART_SEL1, S3C_GPIO_SFN(1));
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH);
		gpio_set_value(GPIO_UART_SEL1, GPIO_LEVEL_LOW);
#endif
	}
	else if (strncmp(buffer, "don0", 4) == 0)
	{
		DumpDebug("Enable Dump!!");
		g_dumpLogs = 1;
	}
	else if (strncmp(buffer, "doff", 4) == 0)
	{
		DumpDebug("Disable Dump!!");
		g_dumpLogs = 0;
	}
	else if (strncmp(buffer, "gpio", 4) == 0)
	{
		int val = 0;

		DumpDebug("WiMAX GPIO status!!");

		val = gpio_get_value(WIMAX_EN);
		DumpDebug("WIMAX_EN: %d", val);

		val = gpio_get_value(WIMAX_RESET);
		DumpDebug("WIMAX_RESET: %d", val);

		val = gpio_get_value(WIMAX_CON0);
		DumpDebug("WIMAX_CON0: %d", val);

		val = gpio_get_value(WIMAX_CON1);
		DumpDebug("WIMAX_CON1: %d", val);

		val = gpio_get_value(WIMAX_CON2);
		DumpDebug("WIMAX_CON2: %d", val);

		val = gpio_get_value(WIMAX_WAKEUP);
		DumpDebug("WIMAX_WAKEUP: %d", val);

		val = gpio_get_value(WIMAX_INT);
		DumpDebug("WIMAX_INT: %d", val);

		val = gpio_get_value(WIMAX_IF_MODE0);
		DumpDebug("WIMAX_IF_MODE0: %d", val);

		val = gpio_get_value(WIMAX_IF_MODE1);
		DumpDebug("WIMAX_IF_MODE1: %d", val);

		val = gpio_get_value(WIMAX_USB_EN);
		DumpDebug("WIMAX_USB_EN: %d", val);

		val = gpio_get_value(I2C_SEL);
		DumpDebug("I2C_SEL: %d", val);

#if defined (CONFIG_MACH_QUATTRO)

#elif defined (CONFIG_MACH_ARIES)
		val = gpio_get_value(GPIO_UART_SEL);
		DumpDebug("UART_SEL: %d", val);

		val = gpio_get_value(GPIO_UART_SEL1);
		DumpDebug("UART_SEL1: %d", val);
#endif
	}
		
	return count;
}

// on/off test
static int proc_read_onoff(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	gpio_wimax_poweroff();
	
	return 0;
}

static int proc_write_onoff(struct file *foke, const char *buffer, unsigned long count, void *data)
{
	if (buffer[0] == 's')
	{
		if (g_dwLineState != SDIO_MODE || gpio_get_value(WIMAX_EN) == 0)
		{
			gpio_wimax_poweroff();
			msleep(10);
			g_dwLineState = SDIO_MODE;
			gpio_wimax_poweron();
		}
	}
	else if (buffer[0] == 'w')
	{
		if (g_dwLineState != WTM_MODE || gpio_get_value(WIMAX_EN) == 0)
		{
			gpio_wimax_poweroff();
			msleep(10);
			g_dwLineState = WTM_MODE;
			gpio_wimax_poweron();
		}
	}
	else if (buffer[0] == 'u')
	{
		if (g_dwLineState != USB_MODE || gpio_get_value(WIMAX_EN) == 0)
			{
			gpio_wimax_poweroff();
			msleep(10);
			g_dwLineState = USB_MODE;
			gpio_wimax_poweron();
		}
	}
	else if (buffer[0] == 'a')
	{
		if (g_dwLineState != AUTH_MODE || gpio_get_value(WIMAX_EN) == 0)
			{
			gpio_wimax_poweroff();
			msleep(10);
			g_dwLineState = AUTH_MODE;
			gpio_wimax_poweron();
		}
	}

	return 0;
}

// Dump Logs
int isDumpEnabled(void)
{
	if (g_dumpLogs == 1) return 1;
	else return 0;
}
EXPORT_SYMBOL(isDumpEnabled);

static int proc_read_dump(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	DumpDebug("CONTROL_IOCTL_WIMAX_CHECK_CAL");
	gpio_wimax_poweroff();

	//WIMAX_Check_Cal();
	
	return 0;
}

static int proc_write_dump(struct file *foke, const char *buffer, unsigned long count, void *data)
{
	if (buffer[0] == '0')
	{
		if (isDumpEnabled())
		{
			DumpDebug("Control Dump Disabled.");
			g_dumpLogs = 0;
		}
	}
	else if (buffer[0] == '1')
	{
		if (!isDumpEnabled())
		{
			DumpDebug("Control Dump Enabled.");
			g_dumpLogs = 1;
		}
	}

	return 0;
}

static int proc_read_sleepmode(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	
	return 0;
}

static int proc_write_sleepmode(struct file *foke, const char *buffer, unsigned long count, void *data)
{
	if (buffer[0] == '0' && g_dwSleepMode == 1)
	{
		DumpDebug("WiMAX Sleep Mode: VI");
		g_dwSleepMode = 0;
	}
	else if (buffer[0] == '1' && g_dwSleepMode == 0)
	{
		DumpDebug("WiMAX Sleep Mode: IDLE");
		g_dwSleepMode = 1;
	}

	return 0;
}

void WiMAX_Proc_Init(void)
{
	/* create procfs directory & entry */
	g_proc_wmxmode_dir = proc_mkdir("wmxmode", NULL);
	create_proc_read_entry("mode", 0644, g_proc_wmxmode_dir,
		wimax_proc_mode, NULL);

	// check on/off
	create_proc_read_entry("power", 0644, g_proc_wmxmode_dir,
		wimax_proc_power, NULL);

	// switch UART path
	g_proc_wmxuart = create_proc_entry("wmxuart", 0600, g_proc_wmxmode_dir);
	g_proc_wmxuart->data = NULL;
//	g_proc_wmxuart->owner = THIS_MODULE;
	g_proc_wmxuart->read_proc = proc_read_wmxuart;
	g_proc_wmxuart->write_proc = proc_write_wmxuart;

	// write EEPROM
	g_proc_eeprom = create_proc_entry("eeprom", 0666, g_proc_wmxmode_dir);
	g_proc_eeprom->data = NULL;
//	g_proc_eeprom->owner = THIS_MODULE;
	g_proc_eeprom->read_proc = proc_read_eeprom;
	g_proc_eeprom->write_proc = proc_write_eeprom;

	// on/off test
	g_proc_onoff = create_proc_entry("onoff", 0600, g_proc_wmxmode_dir);
	g_proc_onoff->data = NULL;
//	g_proc_onoff->owner = THIS_MODULE;
	g_proc_onoff->read_proc = proc_read_onoff;
	g_proc_onoff->write_proc = proc_write_onoff;

	// Dump Logs
	g_proc_dump = create_proc_entry("dump", 0600, g_proc_wmxmode_dir);
	g_proc_dump->data = NULL;
//	g_proc_dump->owner = THIS_MODULE;
	g_proc_dump->read_proc = proc_read_dump;
	g_proc_dump->write_proc = proc_write_dump;
	
	// Sleep Mode
	g_proc_sleepmode = create_proc_entry("sleepmode", 0600, g_proc_wmxmode_dir);
	g_proc_sleepmode->data = NULL;
//	g_proc_sleepmode->owner = THIS_MODULE;
	g_proc_sleepmode->read_proc = proc_read_sleepmode;
	g_proc_sleepmode->write_proc = proc_write_sleepmode;
}

void WiMAX_Proc_Deinit(void)
{
	remove_proc_entry("sleepmode", g_proc_wmxmode_dir);
	remove_proc_entry("dump", g_proc_wmxmode_dir);
	remove_proc_entry("onoff", g_proc_wmxmode_dir);
	remove_proc_entry("eeprom", g_proc_wmxmode_dir);
	remove_proc_entry("wmxuart", g_proc_wmxmode_dir);
	remove_proc_entry("power", g_proc_wmxmode_dir);
	remove_proc_entry("mode", g_proc_wmxmode_dir);
	remove_proc_entry("wmxmode", NULL);
}

