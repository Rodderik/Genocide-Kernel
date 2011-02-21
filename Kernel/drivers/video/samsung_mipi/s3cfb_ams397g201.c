/* linux/drivers/video/samsung/s3cfb_ams397g201.c
 *
 * MIPI-DSI based AMS397G201 AMOLED lcd panel driver.
 *
 * InKi Dae, <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Modified by Samsung Electronics (UK) on May 2010
 *
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/backlight.h>
#include <linux/lcd.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-dsim.h>

#include <mach/atlas/gpio-aries.h>
#include <mach/dsim.h>
#include <mach/mipi_ddi.h>
#include <linux/regulator/consumer.h>

#include "s5p-dsim.h"

#include "AMS403GF01_gamma_1_9.h"
#include "AMS403GF01_gamma_2_2.h"

/*********** for debug **********************************************************/
#if 1
#define gprintk(fmt, x... ) printk( "%s(%d): " fmt, __FUNCTION__ ,__LINE__, ## x)
#else
#define gprintk(x...) do { } while (0)
#endif
/*******************************************************************************/


#define GAMMASET_CONTROL //for 1.9/2.2 gamma control from platform
#define ACL_ENABLE

#define LCD_ESD_INT   IRQ_EINT10
//IRQ_EINT(10)

// SERI-START (grip-lcd-lock)
// add lcd partial constants
#define DCS_PARTIAL_MODE_ON	0X12
#define DCS_NORMAL_MODE_ON	0X13
#define DCS_PARTIAL_AREA	0X30
// SERI-END

#define DIM_BL	20
#define MIN_BL	30
#define MAX_BL	255
#define MAX_GAMMA_VALUE	24
#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define	DEFMASK			0xFF00
#define COMMAND_ONLY		0xFE
#define DATA_ONLY		0xFF

#define AMS403GF01_GAMMA_PARAM_SET_DATA_COUNT       26

/* Indicates the state of the device */

volatile unsigned char bFrameDone = 1;
int current_gamma_value = -1;
int on_19gamma = 0;
int bd_brightness = 0;
static struct timer_list polling_timer;
static int ldi_enable = 0;
static struct s5p_lcd lcd;
struct s5p_lcd{
	struct spi_device *g_spi;
	struct lcd_device *lcd_dev;
	struct backlight_device *bl_dev;

};

static struct mipi_ddi_platform_data *ddi_pd;
extern unsigned char s5p_dsim_set_hs_enable(unsigned int dsim_base);
extern unsigned char s5p_dsim_set_stopstate(unsigned int dsim_base);

extern irqreturn_t s5p_dsim_intr(struct platform_device *pdev);
extern u8 s3cfb_frame_done(void);
extern void s3cfb_trigger(void);

#ifdef GAMMASET_CONTROL
struct class *gammaset_class;
struct device *switch_gammaset_dev;
#endif

char image_update = 1;
//image_update = 1  -> 업데이트 할 image 존재
//image_update = 0  -> 업데이트 할 image  없음 
EXPORT_SYMBOL(image_update);

char EGL_ready = 0;
//EGL_ready = 1  -> EGL 이 동작 
//EGL_ready = 0  -> EGL 이 동작 안함 
EXPORT_SYMBOL(EGL_ready);


#ifdef ACL_ENABLE
#include "AMS403GF01_acl.h"

#define ARRAY_SIZE(arr)	(s32)(sizeof(arr) / sizeof((arr)[0]))

static int acl_enable = 0;
static ACL_STATUS cur_acl_status = ACL_STATUS_0P;
static ACL_STATUS suspend_acl_status = ACL_STATUS_0P;

static unsigned int acl_is_processing = 0;
static DEFINE_MUTEX(acl_lock);

static struct class *acl_class;
static struct device *switch_aclset_dev;
#endif

int IsLDIEnabled(void)
{
	return ldi_enable;
}
EXPORT_SYMBOL(IsLDIEnabled);

void SetLDIEnabledFlag(int OnOff)
{
	ldi_enable = OnOff;
}

void tl2796_reset_lcd(void)
{
	int err;

	err = gpio_request(S5PV210_MP05(5), "MLCD_RST");
        if (err) {
                printk(KERN_ERR "failed to request MP0(5) for "
                        "lcd reset control\n");
                return;
        }

        gpio_direction_output(S5PV210_MP05(5), 1);
        msleep(10);

        gpio_set_value(S5PV210_MP05(5), 0);
        msleep(10);

        gpio_set_value(S5PV210_MP05(5), 1);
        msleep(10);

        gpio_free(S5PV210_MP05(5));

}

// SERI-START (grip-lcd-lock)
// add lcd partial mode values
int panel_on; //flag to keep track of panel status
int panel_initialized=0;
int partial_mode = 0;
static int ams397g201_partial_mode_on(struct device *dev, u8 display_area);
// SERI-END
int backlight_level = 0;

//mipi_temp
void ams397g201_write_0(unsigned char dcs_cmd)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, dcs_cmd, 0);
}

static void ams397g201_write_1(unsigned char dcs_cmd, unsigned char param1)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, dcs_cmd, param1);
}

static void ams397g201_write_2(unsigned char dcs_cmd, unsigned char param1, unsigned char param2)
{
       unsigned char buf[3];
	buf[0] = dcs_cmd;
	buf[1] = param1;
	buf[2] = param2;	

	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, 3);
      
}

// SERI-START (grip-lcd-lock)
// add lcd partial and dim mode functionalies
static void ams397g201_write_4(unsigned char dcs_cmd, unsigned char param1, unsigned char param2, unsigned char param3, unsigned char param4)
{
       unsigned char buf[5];
	buf[0] = dcs_cmd;
	buf[1] = param1;
	buf[2] = param2;	
	buf[3] = param3;
	buf[4] = param4;	

	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR, (unsigned int) buf, 5);
      
}

static int is_panel_on()
{
	//panel command 0x0a gives the current power mode of the panel, but no read function is currently supported by Samsung MIPI driver
	//so we will have to use flag to track the status of panel
	return panel_on;
}
// SERI-END

static void ams397g201_display_off(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA,
		0x28, 0x00);

	// SERI-START (grip-lcd-lock)
	// set panel on value
	panel_on = 0; 
	// SERI-END

}

void ams397g201_sleep_in(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x10, 0);
}

void ams397g201_sleep_out(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x11, 0);
}

static void ams397g201_display_on(struct device *dev)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x29, 0);

	// SERI-START (grip-lcd-lock)
	// set panel on value
	panel_on = 1;
	// SERI-END
}

static void ams397g201_set_tear_off(void)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x34, 0);
}

void ams397g201_set_tear_on(void)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_NO_PARA, 0x35, 0);
}

static void panel_gamma_send_sequence(const unsigned char * gamma_param_set, unsigned int count)
{
 	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,
		(unsigned int) gamma_param_set, count);    

    /* GAMMA Update */
    ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA, 0xFA, 0x03);    
}

void bl_update_status_22gamma(int bl)
{
	int gamma_value = 0;
	int i;

	for(i=0; i<100; i++)
	{
	    gprintk("ldi_enable : %d \n",ldi_enable);
        
		if(IsLDIEnabled())
			break;
		
		msleep(10);
	};	

	if(!(current_gamma_value == -1))
	{
		gprintk("#################22gamma start##########################\n");
		panel_gamma_send_sequence(ams403gf01_22gamma_set_tbl[current_gamma_value], 
                                    AMS403GF01_GAMMA_PARAM_SET_DATA_COUNT);
		gprintk("#################22gamma end##########################\n");
	}
};
EXPORT_SYMBOL(bl_update_status_22gamma);

void bl_update_status_19gamma(int bl)
{
	int gamma_value = 0;
	int i;

	for(i=0; i<100; i++)
	{
		gprintk("ldi_enable : %d \n",ldi_enable);

		if(IsLDIEnabled())
			break;
		
		msleep(10);
	};

	if(!(current_gamma_value == -1))
	{
		gprintk("#################19gamma start##########################\n");
		panel_gamma_send_sequence(ams403gf01_19gamma_set_tbl[current_gamma_value], 
                                    AMS403GF01_GAMMA_PARAM_SET_DATA_COUNT);       
		gprintk("#################19gamma end##########################\n");
	}
}
EXPORT_SYMBOL(bl_update_status_19gamma);


#ifdef GAMMASET_CONTROL //for 1.9/2.2 gamma control from platform
static ssize_t gammaset_file_cmd_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	gprintk("called %s \n",__func__);

	return sprintf(buf,"%u\n",bd_brightness);
}
static ssize_t gammaset_file_cmd_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	int value;
	
    gprintk("called %s \n",__func__);
	
	sscanf(buf, "%d", &value);

	if(IsLDIEnabled()==0)
	{
		printk("[gamma set] return because LDI is disabled, input value = %d \n",value);
		return size;
	}

	if(value==1 && on_19gamma==0)
	{
		on_19gamma = 1;
		bl_update_status_19gamma(bd_brightness);
	}
	else if(value==0 && on_19gamma==1)
	{
		on_19gamma = 0;
		bl_update_status_22gamma(bd_brightness);
	}
	else
		printk("\ngammaset_file_cmd_store value(%d) on_19gamma(%d) \n",value,on_19gamma);

	return size;
}

static DEVICE_ATTR(gammaset_file_cmd,0666, gammaset_file_cmd_show, gammaset_file_cmd_store);
#endif

#ifdef ACL_ENABLE 

#define ACL_CUTOFF_PARAM_SET_DATA_COUNT     29

#define ACL_OFF     0x0
#define ACL_ON      0x1
#define AWON        0x2
#define AWINOUT     0x4

static void ams397g201_acl_onoff(unsigned int isOn)
{
    if( isOn )
    {
        ams397g201_write_2(0xC0, AWINOUT|AWON|ACL_ON, 0x01);        /* ACL FRAME_SIZE = 480 * 992 */
    }
    else
    {
        ams397g201_write_2(0xC0, AWINOUT|AWON|ACL_OFF, 0x01);        
    }
}

static void panel_acl_send_sequence(const unsigned char * acl_param_tbl, unsigned int count)
{
    /* Level 2 key command */
    ams397g201_write_2(0xF0, 0x5A, 0x5A);

    /* ACL Param  */
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,
		(unsigned int) acl_param_tbl, count);	

    ams397g201_acl_onoff(1);                
}



static const char * acl_status_debug_strtbl[ACL_STATUS_MAX] = {
    "ACL_STATUS_0P",
    "ACL_STATUS_40P",    
    "ACL_STATUS_43P",    
    "ACL_STATUS_45P",    
    "ACL_STATUS_47P",    
    "ACL_STATUS_48P",    
    "ACL_STATUS_50P",    
};

static void update_acl_status(ACL_STATUS newAclStatus)
{   
    unsigned int flags;
    unsigned int uRegValue;

    if( newAclStatus >= ACL_STATUS_MAX )
    {
        gprintk(" newAclStatus(%d) is invalid \n", newAclStatus);
    }
        
    if( cur_acl_status != newAclStatus )
    {
        mutex_lock(&acl_lock);
        
        acl_is_processing = 1;
        msleep(15); //for 60HZ

        if( newAclStatus == ACL_STATUS_0P )
        {           
            ams397g201_acl_onoff(0);
        }
        else
        {
            panel_acl_send_sequence(acl_cutoff_param_set_tbl[newAclStatus], 
                                    ACL_CUTOFF_PARAM_SET_DATA_COUNT);
        }
        
        acl_is_processing = 0;        
        cur_acl_status = newAclStatus;

        mutex_unlock(&acl_lock);
    }

    gprintk("cur_acl_status is %s \n", acl_status_debug_strtbl[cur_acl_status]);    
}

static ssize_t aclset_file_cmd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	gprintk("called %s \n",__func__);

	return sprintf(buf,"%u\n", acl_enable);
}
static ssize_t aclset_file_cmd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

    gprintk("called %s \n",__func__);

	sscanf(buf, "%d", &value);


	if(IsLDIEnabled()==0)
	{
		printk(KERN_DEBUG "[acl set] return because LDI is disabled, input value = %d \n",value);
		return size;
	}

	if(value==1 && acl_enable == 0)
	{		
		acl_enable = value;
	
        if (current_gamma_value >=0 && current_gamma_value <= 1)
        {
            update_acl_status(ACL_STATUS_0P);            //0 %
        }
        else if(current_gamma_value >=2 && current_gamma_value <= 12)
        {
            update_acl_status(ACL_STATUS_40P);
        }
        else if(current_gamma_value ==13)
        {
            update_acl_status(ACL_STATUS_43P);
        }
        else if(current_gamma_value ==14)
        {
            update_acl_status(ACL_STATUS_45P);
        }
        else if(current_gamma_value ==15)
        {
            update_acl_status(ACL_STATUS_47P);
        }
        else if(current_gamma_value ==16)
        {
            update_acl_status(ACL_STATUS_48P);
        }
        else
        {
            update_acl_status(ACL_STATUS_50P);
        }
	}
	else if(value==0 && acl_enable == 1)
	{
		acl_enable = value;

        //ACL Off
        update_acl_status(ACL_STATUS_0P);            //0 %
		
        gprintk("ACL is disable \n");    
	}
	else
		printk("aclset_file_cmd_store value is same : value(%d)\n",value);

	return size;
}

static DEVICE_ATTR(aclset_file_cmd,0666, aclset_file_cmd_show, aclset_file_cmd_store);
#endif

static void ETC_CONDITION_SET_1(void)
{
    unsigned char buf[3];

    buf[0] = 0xF0;
    buf[1] = 0x5A;
    buf[2] = 0x5A;

	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,
		(unsigned int) buf, 3);

    //Frame Rate
    ams397g201_write_1(0xFD, 0x16); //60Hz    
}

char gamma_lcd_init = 0;
void GAMMA_CONDITION_SET(void)
{
	unsigned char buf[26];

    /* GAMMA SET */
	buf[0] = 0xFA;   
	buf[1] = 0x02;
	buf[2] = 0x5E;
	buf[3] = 0x5E;
	buf[4] = 0x6F;
	buf[5] = 0x9A;
	buf[6] = 0x82;
	buf[7] = 0x74;
	buf[8] = 0x9F;
	buf[9] = 0xA3;
	buf[10] = 0x90;
	buf[11] = 0xBB;
	buf[12] = 0xBD;
	buf[13] = 0xAF;
	buf[14] = 0x8A;
	buf[15] = 0x8D;
	buf[16] = 0x7C;
	buf[17] = 0xA9;
	buf[18] = 0xAA;
	buf[19] = 0xA0;
	buf[20] = 0x00;
	buf[21] = 0xC4;
	buf[22] = 0x00;
	buf[23] = 0xC3;
	buf[24] = 0x01;
	buf[25] = 0x04;

	gamma_lcd_init = 0xAB;

	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,
		(unsigned int) buf, 26);

    /* GAMMA UPDATE */
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_WR_1_PARA,
		(unsigned int) 0xFA, 0x03);
}

static void PANEL_CONDITION_SET(void)
{
	unsigned char buf[14];
    
	buf[0] =  0xF8;
	buf[1] =  0x28;
	buf[2] =  0x28;
	buf[3] =  0x08;
	buf[4] =  0x08;
	buf[5] =  0x40;
	buf[6] =  0xB0;
	buf[7] =  0x50;
	buf[8] =  0x90;
	buf[9] =  0x10;
	buf[10] = 0x30;
	buf[11] = 0x10;
	buf[12] = 0x00;
	buf[13] = 0x00;

	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,
		(unsigned int) buf, 14);     
}

static void ETC_CONDITION_SET_2(void)
{
	unsigned char buf[5];
    
	buf[0] =  0xF6;
	buf[1] =  0x00;
	buf[2] =  0x84;
	buf[3] =  0x09;
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,
		(unsigned int) buf, 4);    

    ams397g201_write_1(0xB0, 0x01); 
    ams397g201_write_1(0xC0, 0x01);     /* ACL FRAME_SIZE = 480 * 992 */

    buf[0] = 0x2A;
    buf[1] = 0x00;
    buf[2] = 0x3C;
    buf[3] = 0x02;
    buf[4] = 0x1B;
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,
		(unsigned int) buf, 5);    

    buf[0] = 0x2B;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x03;
    buf[4] = 0xDF;    
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_LONG_WR,
		(unsigned int) buf, 5);    

    ams397g201_write_1(0xB0, 0x09); 
    ams397g201_write_1(0xD5, 0x64); 

    ams397g201_write_1(0xB0, 0x0B); 
    ams397g201_write_1(0xD5, 0xA4); 

    ams397g201_write_1(0xB0, 0x0C); 
    ams397g201_write_1(0xD5, 0x7E); 

    ams397g201_write_1(0xB0, 0x0D); 
    ams397g201_write_1(0xD5, 0x20);     

    ams397g201_write_1(0xF7, 0x03); 
    ams397g201_write_1(0xB0, 0x02); 
    ams397g201_write_1(0xB3, 0xC3); 

}



void lcd_pannel_on(void)
{        
	//Stand by off
	ams397g201_write_0(0x11);

       msleep(130);

	ams397g201_write_0(0x29);

//	ams397g201_write_0(0x35);       
}

void lcd_panel_init(void)
{
    ETC_CONDITION_SET_1();
    
    //60프레임 적용 	
    ams397g201_write_1(0xB0, 0x01);	 //sehun_test
    ams397g201_write_1(0xF2, 0x07);	
	
//    GAMMA_CONDITION_SET();

    PANEL_CONDITION_SET();

    ETC_CONDITION_SET_2();

    lcd_pannel_on(); 
}

static void update_gamma(int gamma_value)
{
 	u32 uRegValue;

	if( gamma_value >= AM403GF01_2_2_GAMMA_MAX )
	{
		gprintk(" gamma_value(%d) is invalid \n", gamma_value);
	}
	
	if(on_19gamma)
		panel_gamma_send_sequence(ams403gf01_19gamma_set_tbl[gamma_value], AMS403GF01_GAMMA_PARAM_SET_DATA_COUNT);
	else
		panel_gamma_send_sequence(ams403gf01_22gamma_set_tbl[gamma_value], AMS403GF01_GAMMA_PARAM_SET_DATA_COUNT);     

	current_gamma_value = gamma_value;

	gprintk("current_gamma_value is %d \n", current_gamma_value);    
}

//WORK QUEING FUNCTION
static void ams397g201_esd(void)
{
        printk("ams397g201_esd \n");

        u32 err,intsrc;

		
        err = readl(ddi_pd->dsim_base + S5P_DSIM_INTSRC);
        printk("ESD_DSIM_INTSRC -> %x \n", intsrc);
	
        intsrc = readl(ddi_pd->dsim_base + S5P_DSIM_SWRST);        
        intsrc  = intsrc | 1 << 16;
        writel(intsrc, ddi_pd->dsim_base + S5P_DSIM_SWRST);
    
	tl2796_reset_lcd();

	mdelay(10);

	lcd_panel_init();

       update_gamma(current_gamma_value);

	lcd_pannel_on();

	ams397g201_set_tear_on();

	enable_irq( LCD_ESD_INT);

}

static DECLARE_WORK(lcd_esd_work, ams397g201_esd);

//IRQ Handler
static int ams397g201_esd_irq_handler(void)
{
       printk("ESD_irq_handler \n");

	disable_irq( LCD_ESD_INT);

	if(EGL_ready == 1 )
	schedule_work(&lcd_esd_work);
	else
		enable_irq( LCD_ESD_INT);

	return IRQ_HANDLED;
}

char lcd_on_off=1;	
static int ams397g201_panel_init(struct device *dev)
{
	lcd_panel_init();

	panel_initialized = 1;
	lcd_on_off = 1;

	if(partial_mode)
		ams397g201_partial_mode_on(dev, partial_mode);

   /* this function should be called after partial mode call(if any)*/
	return 0;
}

static int ams397g201_set_link(void *pd, unsigned int dsim_base,
	unsigned char (*cmd_write) (unsigned int dsim_base, unsigned int data0,
	    unsigned int data1, unsigned int data2),
	unsigned char (*cmd_read) (unsigned int dsim_base, unsigned int data0,
	    unsigned int data1, unsigned int data2))
{
	struct mipi_ddi_platform_data *temp_pd = NULL;

	temp_pd = (struct mipi_ddi_platform_data *) pd;
	if (temp_pd == NULL) {
		printk(KERN_ERR "mipi_ddi_platform_data is null.\n");
		return -1;
	}

	ddi_pd = temp_pd;

	ddi_pd->dsim_base = dsim_base;

	if (cmd_write)
		ddi_pd->cmd_write = cmd_write;
	else
		printk(KERN_WARNING "cmd_write function is null.\n");

	if (cmd_read)
		ddi_pd->cmd_read = cmd_read;
	else
		printk(KERN_WARNING "cmd_read function is null.\n");

	return 0;
}

static irqreturn_t ams397g201_te_interrupt(int irq, void *dev_id)
{
	struct device *dev = NULL;
	static unsigned int display_on = DSIM_NONE_STATE;
	u32 intsrc = 0;
	u32 dsim_clkcon = 0;

	dev = (struct device *) dev_id;

	if( image_update == 0 )
		return IRQ_HANDLED;

	disable_irq_nosync(irq);

	intsrc = readl(ddi_pd->dsim_base + S5P_DSIM_INTSRC);

      if (intsrc & (1 << 24)) //####
      {
      	        if( bFrameDone == 0x1 && acl_is_processing == 0)
	{
		       intsrc  = intsrc | 1 << 24; 
		       writel(intsrc, ddi_pd->dsim_base + S5P_DSIM_INTSRC); 
		
			//	dsim_clkcon = readl(ddi_pd->dsim_base + S5P_DSIM_CLKCTRL) | ( 1<< 31);
			//	writel(dsim_clkcon, ddi_pd->dsim_base + S5P_DSIM_CLKCTRL);
			//	printk("@@@\n");
		s3cfb_trigger();
	}
       	}
	//else if (bFrameDone == 0x2)
	//{
	//	writel((1 <<16), ddi_pd->dsim_base + S5P_DSIM_SWRST);
	//}

	if(EGL_ready == 1 )
	image_update = 0;
	enable_irq(irq);

	return IRQ_HANDLED;
}
// SERI-START (grip-lcd-lock)
// add lcd partial mode functions

static bool ams397g201_partial_mode_status(struct device *dev)
{
	
	return false;
 	
}

static int ams397g201_partial_mode_on(struct device *dev, u8 display_area)
{
	int r=0;
	
	partial_mode = display_area;
	if(!panel_initialized)
	{
		// panel is not yet initialized, so return. stored partial_mode will be used when initializing the panel	
		gprintk("panel not yet initialized, returning");
		return r;
	}
	gprintk("partial mode req being sent to screen: %d\r\n", display_area);


	switch (display_area){
	case 1: //main screen: from row 1 to 799 (0x31F)
		ams397g201_write_4(DCS_PARTIAL_AREA, 0, 2, 0x3, 0x20);
		break;

	case 2: //softkeys screen: from row 799 (0x31F) to 895 (0x37F)
		ams397g201_write_4(DCS_PARTIAL_AREA, 0x3, 0x20, 0x3, 0x80);
		break;

	case 3: //main + softkeys screen
		ams397g201_write_4(DCS_PARTIAL_AREA, 0, 2, 0x3, 0x80);
		break;

	case 4: //ticker screen
		ams397g201_write_4(DCS_PARTIAL_AREA, 0x3, 0x80, 0x3, 0xE0);
		break;

	case 6: //softkeys + ticker screen
		ams397g201_write_4(DCS_PARTIAL_AREA, 0x3, 0x20, 0x3, 0xE0);
		break;

	case 7: //full screen
		ams397g201_write_4(DCS_PARTIAL_AREA, 0, 2, 0x3, 0xE0);
		break;


	default:
		return -EINVAL;

	}
	
	//Uncomment the msleep() below if there is any issue with the panel entering partial mode
	//usually once the panel has entered into partial mode, sometimes it will not enter into
	//partial mode again without this msleep. Fortunately, in our use cases the panel might  
	//enter into partial mode only once each time after it is switched on. So, no need to 
	//use this msleep() for Garnett.
	
	//msleep(25);
	ams397g201_write_0(DCS_PARTIAL_MODE_ON);	

	return r;

}

static int ams397g201_partial_mode_off(struct device *dev)
{
	partial_mode = 0;

	if(!panel_initialized)
	{
		gprintk("panel not yet initialized, returning\r\n");
		return 0;
	}
	ams397g201_write_0(DCS_NORMAL_MODE_ON);
	
	return 0;
}

// SERI-END
static void ams397g201_GetGammaValue(void)
{
	ddi_pd->cmd_write(ddi_pd->dsim_base, DCS_RD_NO_PARA,0xfa,0);
}

extern char upper_connect;
static int s5p_bl_update_status(struct backlight_device* bd)
{

	int bl = bd->props.brightness;
	int level = 0;
	int gamma_value = 0;
	int gamma_val_x10 = 0;
	char i;
	u32 uRegValue;

	if(upper_connect == 0)
		return 0;
			
	if (bl >= MIN_BL)
	{
		gamma_val_x10 = 10*(MAX_GAMMA_VALUE-1)*bl/(MAX_BL-MIN_BL) + (10 - 10*(MAX_GAMMA_VALUE-1)*(MIN_BL)/(MAX_BL-MIN_BL)) ;
		gamma_value = (gamma_val_x10+5)/10;
	}	
	else
	{
		gamma_value = 0;
	}
	
	for(i=0; i<200; i++)
	{       
		if(IsLDIEnabled())
			break;
 		gprintk("ldi_enable : %d \n",ldi_enable);        

		msleep(10);
	};
	
       gprintk("update status brightness[0~255] : (%d) \n",bd->props.brightness);
	  
	if(IsLDIEnabled())
	{
		bFrameDone = 0;

	       mdelay(10);
		for(i=0;i<15;i++)
		{
			uRegValue = readl(ddi_pd->dsim_base + S5P_DSIM_INTSRC);
			if ( uRegValue & (1 << 24) )
				break;
	 		msleep(7);			
		}

		if(lcd_on_off == 0) 
			ams397g201_display_on(bd->dev.parent);

		if(bl == 0)
			level = 0;	//lcd off
		else if((bl < MIN_BL) && (bl > 0))
			level = 1;	//dimming
		else
			level = 6;	//normal

              pr_err("bl=%d, gamma_value=%d, acl_enable=%d,on_19gamma=%d \n",bl, gamma_value, acl_enable, on_19gamma);

		if(level==0)
		{
			msleep(20);
			ams397g201_display_off(bd->dev.parent);
			gprintk("Update status brightness[0~255]:(%d) - LCD OFF \n", bl);
			lcd_on_off=0;
			bd_brightness = 0;
			backlight_level = 0;
			current_gamma_value = -1;
			bFrameDone = 1;
			return 0;
		}	

		bd_brightness = bd->props.brightness;
		backlight_level = level;
        
		if(current_gamma_value == gamma_value)
		{
			bFrameDone = 1;
			return 0;
		}

	 	if(level)
		{
			switch(level)
			{
				case  5:
				case  4:
				case  3:
				case  2:
				case  1: //dimming
                     {            
				#ifdef ACL_ENABLE
					if (acl_enable)
					{
                        update_acl_status(ACL_STATUS_0P);						
					}
				#endif


                    update_gamma(gamma_value);
                
					break;                
                }
				case  6:
								{		                    
#ifdef ACL_ENABLE
                    if (acl_enable)
                    {   
						if (gamma_value == 1)
						{
						    update_acl_status(ACL_STATUS_0P);
						}
						else
						{							
							if(gamma_value >=2 && gamma_value <=12) 
							{
							    update_acl_status(ACL_STATUS_40P);
							}
							else if(gamma_value == 13) 
							{
							    update_acl_status(ACL_STATUS_43P);
							}
							else if(gamma_value == 14) 
							{
							    update_acl_status(ACL_STATUS_45P);
							}
							else if(gamma_value == 15) 
							{
							    update_acl_status(ACL_STATUS_47P);
							}
							else if(gamma_value == 16) 
							{
							    update_acl_status(ACL_STATUS_48P);
							}
							else
							{
							    update_acl_status(ACL_STATUS_50P);
							}	
						}	
					}              
#endif
                    update_gamma(gamma_value);
                    					
					break;
				}
			}

            current_gamma_value = gamma_value;            
		}			
	}
	bFrameDone = 1;
	
	return 0;
}

static int s5p_bl_get_brightness(struct backlilght_device* bd)
{
    gprintk("called %s \n",__func__);

	return bd_brightness;
}


static struct backlight_ops s5p_bl_ops = {
	.update_status = s5p_bl_update_status,
	.get_brightness = s5p_bl_get_brightness,	
};

static void polling_ESD_timer_func(unsigned long unused)
{
	ams397g201_write_0(0x38);
	
	mod_timer(&polling_timer,
		  jiffies + msecs_to_jiffies(5000));
}

/* sysfs export of baclight control */
static int s3cfb_sysfs_show_lcd_update(struct device *dev, struct device_attribute *attr, char *buf)
{
	return ;
}

static int s3cfb_sysfs_store_lcd_update(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	int value;

	sscanf(buf, "%d", &value);

	if(value==1 )
	       image_update = 1;

	return len;
}

static DEVICE_ATTR(lcd_update, 0777, s3cfb_sysfs_show_lcd_update, s3cfb_sysfs_store_lcd_update);

static int ams397g201_probe(struct device *dev)
{
    int ret = 0;
    struct regulator *r_lcd_1_8v, *r_lcd_3_0v;

   lcd.bl_dev =backlight_device_register("s5p_bl",dev,&lcd,&s5p_bl_ops);
   lcd.bl_dev->props.max_brightness = 255;
   
   SetLDIEnabledFlag(1);

   //For ESD test panel error
   setup_timer(&polling_timer, polling_ESD_timer_func, 0);
   mod_timer(&polling_timer,
   	jiffies + msecs_to_jiffies(5000));


#ifdef GAMMASET_CONTROL //for 1.9/2.2 gamma control from platform
	gammaset_class = class_create(THIS_MODULE, "gammaset");
	if (IS_ERR(gammaset_class))
		pr_err("Failed to create class(gammaset_class)!\n");

	switch_gammaset_dev = device_create(gammaset_class, NULL, 0, NULL, "switch_gammaset");
	if (IS_ERR(switch_gammaset_dev))
		pr_err("Failed to create device(switch_gammaset_dev)!\n");

	if (device_create_file(switch_gammaset_dev, &dev_attr_gammaset_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_gammaset_file_cmd.attr.name);

	if (device_create_file(switch_gammaset_dev, &dev_attr_lcd_update) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_lcd_update);

#endif

#ifdef ACL_ENABLE //ACL On,Off
	acl_class = class_create(THIS_MODULE, "aclset");
	if (IS_ERR(acl_class))
		pr_err("Failed to create class(acl_class)!\n");

	switch_aclset_dev = device_create(acl_class, NULL, 0, NULL, "switch_aclset");
	if (IS_ERR(switch_aclset_dev))
		pr_err("Failed to create device(switch_aclset_dev)!\n");

	if (device_create_file(switch_aclset_dev, &dev_attr_aclset_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_aclset_file_cmd.attr.name);
#endif   

   
#if 1		
        //TE interrupt pin ( GPF0_1 /GPF0_INT[1] ) enable filter
        s3c_gpio_int_flt(S5PV210_GPA0(0),1);

	/* irq for TE signal *///sehun_test
	if (request_irq(S3C_GPIOINT(F0,1), ams397g201_te_interrupt, IRQF_TRIGGER_RISING | IRQF_SHARED,
		    "te_signal", dev)) {
		dev_err(dev, "request_irq failed.\n");
		goto err_te_irq;
	}	

	#if 0
		/* irq for ESD signal */
		s3c_gpio_cfgpin(S5PV210_GPH1(2), S3C_GPIO_SFN(0xFF));
		s3c_gpio_setpull(S5PV210_GPH1(2), S3C_GPIO_PULL_DOWN);

		set_irq_type(LCD_ESD_INT, IRQ_TYPE_EDGE_RISING);
	        if( (ret = request_irq(LCD_ESD_INT, ams397g201_esd_irq_handler,IRQF_DISABLED , "LCD_ESD", NULL)) < 0 )
	        {
	            printk("%s, request_irq failed %d, ret= %d\n",__FUNCTION__, LCD_ESD_INT, ret );
	        }

		 disable_irq( LCD_ESD_INT);

	        mdelay(10);
			
		 enable_irq( LCD_ESD_INT);
	 #endif
#endif
	r_lcd_1_8v = regulator_get(dev, "VLCD_1.8V");
	if (IS_ERR(r_lcd_1_8v)) {
		dev_err(dev, "failed to get regulator VLCD_1.8V.\n");
		return -EINVAL;
	}
#if 0
	ret = regulator_set_voltage(r_lcd_1_8v, 1800000, 1800000);
	if(ret != 0) {
		printk(KERN_ERR "%s: Cannot set VLCD_1.8V\n", __func__);
		return -EINVAL;
	}

	ret = regulator_enable(r_lcd_1_8v);
	if (ret < 0) {
		printk(KERN_ERR "failed to enable regulator VLCD_1_8v.\n");
		return ret;
	}
#endif
	r_lcd_3_0v = regulator_get(dev, "VLCD_3.0V");
	if (IS_ERR(r_lcd_3_0v)) {
		dev_err(dev, "failed to get regulator VLCD_3.0V.\n");
		return -EINVAL;
	}

#if 0
	ret = regulator_set_voltage(r_lcd_3_0v, 3000000, 3000000);
	if(ret != 0) {
		printk(KERN_ERR "%s: Cannot set VLCD_3.0V\n", __func__);
		return -EINVAL;
	}

	ret = regulator_enable(r_lcd_3_0v);
	if (ret < 0) {
		printk(KERN_ERR "failed to enable regulator VLCD_3_0v.\n");
		return ret;
	}
#endif
#if 0 //@@
	/* lcd power on */
	if (ddi_pd->lcd_power_on)
		ddi_pd->lcd_power_on((void *) dev, 1);
	else
		dev_warn(dev, "lcd_power_on func is null.\n");

	mdelay(60);

	/* lcd reset */
	if (ddi_pd->lcd_reset)
		ddi_pd->lcd_reset();
	else
		dev_warn(dev, "lcd_reset func is null.\n");
#endif
	dev_info(dev, "ams397g201 lcd panel driver based on mipi-dsi has been probed.\n");

err_te_irq:

	return 0;
}

#ifdef CONFIG_PM
static int ams397g201_suspend(struct device *dev, pm_message_t mesg)
{
    ACL_STATUS  prev_acl_status;
        
	pr_err("[Pannel][%s] \n",__func__);
	
	// SERI-START (grip-lcd-lock)
	panel_initialized = 0;
	// SERI-END
	
#ifdef ACL_ENABLE
    update_acl_status(ACL_STATUS_0P);
#endif

	ams397g201_sleep_in(dev);
	msleep(120);

	return 0;
}

static int ams397g201_resume(struct device *dev)
{    
	pr_err("[Pannel][%s] \n",__func__);

	return 0;
}
#else
#define ams397g201_suspend	NULL
#define ams397g201_resume	NULL
#endif

static struct mipi_lcd_driver ams397g201_mipi_driver = {
	.name = "ams397g201",

	.init = ams397g201_panel_init,
	.display_on = ams397g201_display_on,
	.set_link = ams397g201_set_link,
	.probe = ams397g201_probe,
	.suspend = ams397g201_suspend,
	.resume = ams397g201_resume,
	// SERI-START (grip-lcd-lock)
	// add lcd partial and dim mode functionalies
	.partial_mode_status = ams397g201_partial_mode_status,
	.partial_mode_on = ams397g201_partial_mode_on,
	.partial_mode_off = ams397g201_partial_mode_off,
	.display_off = ams397g201_display_off,
	//SERI: end
};

static int ams397g201_init(void)
{
	s5p_dsim_register_lcd_driver(&ams397g201_mipi_driver);

	return 0;
}

static void ams397g201_exit(void)
{
	return;
}

module_init(ams397g201_init);
module_exit(ams397g201_exit);

MODULE_AUTHOR("InKi Dae <inki.dae@samsung.com>");
MODULE_DESCRIPTION("MIPI-DSI based AMS397G201 AMOLED LCD Panel Driver");
MODULE_LICENSE("GPL");
