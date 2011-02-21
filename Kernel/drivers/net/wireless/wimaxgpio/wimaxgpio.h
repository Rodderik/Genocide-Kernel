#ifndef __WIMAXGPIO_H__
#define __WIMAXGPIO_H__

#define DumpDebug(args...) {	\
						{		\
							printk("<1>\x1b[1;32m[WGPIO] ");printk(args);printk("\x1b[0m\n");	\
						}}

#define ENTER printk("<4>\x1b[1;32m--> %s() line %d\x1b[0m\n", __FUNCTION__, __LINE__);
#define LEAVE printk("<4>\x1b[1;32m<-- %s() line %d\x1b[0m\n", __FUNCTION__, __LINE__);

enum USB_PATH
{
	USB_PATH_PDA,
	USB_PATH_PHONE,
	USB_PATH_WIBRO,
    USB_PATH_NONE
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

int gpio_wimax_poweron (void);
int gpio_wimax_poweroff (void);


#endif	// __WIMAXGPIO_H__

