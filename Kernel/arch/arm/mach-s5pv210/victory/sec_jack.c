/*
 *	JACK device detection driver.
 *
 *	Copyright (C) 2009 Samsung Electronics, Inc.
 *
 *	Authors:
 *		Uk Kim <w0806.kim@samsung.com>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 */

#include <linux/module.h>
#include <linux/sysdev.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/switch.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/wakelock.h>

#include <mach/hardware.h>
#include <mach/gpio.h>
#include <mach/gpio.h>
//#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>
#include <asm/mach-types.h>

#include <mach/sec_jack.h>

#define CONFIG_DEBUG_SEC_JACK
#define SUBJECT "JACK_DRIVER"

#ifdef CONFIG_DEBUG_SEC_JACK
#define SEC_JACKDEV_DBG(format,...)\
	printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);

#else
#define DEBUG_LOG(format,...)
#endif

#define KEYCODE_SENDEND 248

#define DETECTION_CHECK_COUNT	2
#define	DETECTION_CHECK_TIME	get_jiffies_64() + (HZ/10)// 1000ms / 10 = 100ms
#define	SEND_END_ENABLE_TIME	get_jiffies_64() + (HZ*2)// 1000ms * 1 = 1sec

#define SEND_END_CHECK_COUNT	3
#define SEND_END_CHECK_TIME     get_jiffies_64() + (HZ/50) //2000ms

#define WAKELOCK_DET_TIMEOUT	HZ * 5 //5 sec

#define GPIO_POPUP_SW_EN               S5PV210_GPB(1)

static struct platform_driver sec_jack_driver;
extern unsigned int HWREV;

extern void wm8994_set_vps_related_output_path(int enable);

struct class *jack_class;
EXPORT_SYMBOL(jack_class);
static struct device *jack_selector_fs;				// Sysfs device, this is used for communication with Cal App.
EXPORT_SYMBOL(jack_selector_fs);
extern int s3c_adc_get_adc_data(int channel);

struct sec_jack_info {
	struct sec_jack_port port;
	struct input_dev *input;
};

static struct sec_jack_info *hi;

struct switch_dev switch_jack_detection = {
		.name = "h2w",
};


/* To support AT+FCESTEST=1 */
struct switch_dev switch_sendend = {
		.name = "send_end",
};

static struct timer_list jack_detect_timer;
static struct timer_list send_end_key_event_timer;

static unsigned int current_jack_type_status;
static unsigned int jack_detect_timer_token;
static unsigned int send_end_key_timer_token;
static unsigned int send_end_irq_token;
static unsigned int sendend_type; //0=short, 1=open
static struct wake_lock jack_sendend_wake_lock;
static int recording_status=0;

unsigned int get_headset_status(void)
{
	//SEC_JACKDEV_DBG(" headset_status %d", current_jack_type_status);
	return current_jack_type_status;
}
EXPORT_SYMBOL(get_headset_status);

void set_recording_status(int value)
{
	recording_status = value;
}
static int get_recording_status(void)
{
	return recording_status;
}
void car_vps_status_change(int status)
{
        printk("[ CAR_JACK_DRIVER ] %s \n",__func__);

        if(status)
    {
                current_jack_type_status = SEC_EXTRA_CAR_DOCK_SPEAKER;

        wm8994_set_vps_related_output_path(1);
    }
        else
    {
        int adc = s3c_adc_get_adc_data(SEC_HEADSET_ADC_CHANNEL);
            struct sec_gpio_info *det_jack = &hi->port.det_jack;
        int headset_detect = gpio_get_value(det_jack->gpio) ^ det_jack->low_active;

        if(headset_detect)
        {
            if(adc > 800)
            {
                current_jack_type_status = SEC_HEADSET_4_POLE_DEVICE;
            }
            else
            {
                current_jack_type_status = SEC_HEADSET_3_POLE_DEVICE;
            }
       }
        else
        {
                    current_jack_type_status = SEC_JACK_NO_DEVICE;
        }
	wm8994_set_vps_related_output_path(0);
    }

        switch_set_state(&switch_jack_detection, current_jack_type_status);
}

void vps_status_change(int status)
{
        printk("[ JACK_DRIVER ] %s \n",__func__);

        if(status)
    {
                current_jack_type_status = SEC_EXTRA_DOCK_SPEAKER;

        wm8994_set_vps_related_output_path(1);
    }
        else
    {
        int adc = s3c_adc_get_adc_data(SEC_HEADSET_ADC_CHANNEL);
            struct sec_gpio_info *det_jack = &hi->port.det_jack;
        int headset_detect = gpio_get_value(det_jack->gpio) ^ det_jack->low_active;

        if(headset_detect)
        {
            if(adc > 800)
            {
                current_jack_type_status = SEC_HEADSET_4_POLE_DEVICE;
            }
            else
            {
                current_jack_type_status = SEC_HEADSET_3_POLE_DEVICE;
            }
        }
        else
        {
                    current_jack_type_status = SEC_JACK_NO_DEVICE;
        }
        wm8994_set_vps_related_output_path(0);
     }

        switch_set_state(&switch_jack_detection, current_jack_type_status);
}


static void jack_input_selector(int jack_type_status)
{
	SEC_JACKDEV_DBG("jack_type_status = 0X%x", jack_type_status);
}

static void jack_type_detect_change(struct work_struct *ignored)
{
	int adc = 0;
		struct sec_gpio_info   *det_jack = &hi->port.det_jack;
		struct sec_gpio_info   *send_end = &hi->port.send_end;
		struct sec_gpio_info   *send_end_open = &hi->port.send_end_open;
		int state = gpio_get_value(det_jack->gpio) ^ det_jack->low_active;
		int sendend_state,sendend_open_state;

		if(state)
		{
			sendend_state = gpio_get_value(send_end->gpio) ^ send_end->low_active;
			SEC_JACKDEV_DBG("SendEnd state short %d \n",sendend_state);

	#if 1 //open_send_end do nothing
			if (1) // suik_Fix (HWREV >= 0x01)
			{
				sendend_open_state = gpio_get_value(send_end_open->gpio) ^ send_end_open->low_active;
				SEC_JACKDEV_DBG("SendEnd state short %d open %d\n",sendend_state,sendend_open_state);
				//if(sendend_state || sendend_open_state)   //suik_Fix
				if(!sendend_open_state)
				{
					printk("4 pole  headset attached\n");
					current_jack_type_status = SEC_HEADSET_4_POLE_DEVICE;
					#if 1 // REV07 Only suik_Check
					if(gpio_get_value(send_end->gpio))
					#endif
					{
					   enable_irq (send_end->eint);  //suik_Fix
					}
					enable_irq (send_end_open->eint);
				}else
				{
					printk("3 pole headset attatched\n");
					current_jack_type_status = SEC_HEADSET_3_POLE_DEVICE;
				}

			}else
	#endif
			{
				if(sendend_state)
				{
					printk("4 pole  headset attached\n");
					current_jack_type_status = SEC_HEADSET_4_POLE_DEVICE;
				}else
				{
					printk("3 pole headset attatched\n");
					current_jack_type_status = SEC_HEADSET_3_POLE_DEVICE;
				}
				enable_irq (send_end->eint);
			}
			send_end_irq_token++;
			switch_set_state(&switch_jack_detection, current_jack_type_status);
			jack_input_selector(current_jack_type_status);
		}
	wake_unlock(&jack_sendend_wake_lock);
}


static DECLARE_DELAYED_WORK(detect_jack_type_work, jack_type_detect_change);

static void jack_detect_change(struct work_struct *ignored)
{
		struct sec_gpio_info   *det_jack = &hi->port.det_jack;
		struct sec_gpio_info   *send_end = &hi->port.send_end;
		struct sec_gpio_info   *send_end_open = &hi->port.send_end_open;
		int state;
		int sendend_state;//rakesh.gohel for new detection algorithm

		SEC_JACKDEV_DBG("");
		del_timer(&jack_detect_timer);
		cancel_delayed_work_sync(&detect_jack_type_work);
		state = gpio_get_value(det_jack->gpio) ^ det_jack->low_active;

		SEC_JACKDEV_DBG("jack_detect_change state %d send_end_irq_token %d", state,send_end_irq_token);
		if (state && !send_end_irq_token)
		{
			wake_lock(&jack_sendend_wake_lock);
			gpio_set_value(GPIO_POPUP_SW_EN, 1); //suik_Fix
			s3c_gpio_slp_cfgpin(GPIO_POPUP_SW_EN, S3C_GPIO_SLP_OUT1);
			SEC_JACKDEV_DBG("JACK dev attached timer start\n");
			jack_detect_timer_token = 0;
			jack_detect_timer.expires = DETECTION_CHECK_TIME;
			add_timer(&jack_detect_timer);
			sendend_type =0x00;//short type always
		}
		else if(!state)
		{
			current_jack_type_status = SEC_JACK_NO_DEVICE;
	              if(!get_recording_status())
	              {
	                    gpio_set_value(GPIO_MICBIAS_EN, 0);
	              }
			switch_set_state(&switch_jack_detection, current_jack_type_status);
			gpio_set_value(GPIO_POPUP_SW_EN, 0); //suik_Fix
			s3c_gpio_slp_cfgpin(GPIO_POPUP_SW_EN, S3C_GPIO_SLP_OUT0);
			printk("JACK dev detached %d \n", send_end_irq_token);
			//headset_status = SEC_JACK_NO_DEVICE;
			if(send_end_irq_token > 0)
			{
				if (1) //suik_Fix (HWREV >= 0x01)
					disable_irq (send_end_open->eint);
				disable_irq (send_end->eint);
				send_end_irq_token--;
				sendend_type = 0;
			}
			wake_unlock(&jack_sendend_wake_lock);
		}
		else
			SEC_JACKDEV_DBG("Headset state does not valid. or send_end event");

}

static void sendend_switch_change(struct work_struct *ignored)
{

	struct sec_gpio_info   *det_jack = &hi->port.det_jack;
	struct sec_gpio_info   *send_end;
	int state, headset_state;
	SEC_JACKDEV_DBG("");
	del_timer(&send_end_key_event_timer);
	send_end_key_timer_token = 0;

		send_end = &hi->port.send_end;
	headset_state = gpio_get_value(det_jack->gpio) ^ det_jack->low_active;
	state = gpio_get_value(send_end->gpio) ^ send_end->low_active;

	if(headset_state && send_end_irq_token)//headset connect && send irq enable
	{
		SEC_JACKDEV_DBG(" sendend_switch_change sendend state %d\n",state);
		if(state)
		{
			wake_lock(&jack_sendend_wake_lock);
			send_end_key_event_timer.expires = SEND_END_CHECK_TIME;
			add_timer(&send_end_key_event_timer);
			switch_set_state(&switch_sendend, state);
			SEC_JACKDEV_DBG("SEND/END %s.timer start \n", "pressed");
		}else
		{
			SEC_JACKDEV_DBG(KERN_ERR "sendend isr work queue\n");
			switch_set_state(&switch_sendend, state);
			input_report_key(hi->input, KEYCODE_SENDEND, 0); //released  //suik_Fix
			input_sync(hi->input);
			printk("SEND/END %s.\n", "released");
			wake_unlock(&jack_sendend_wake_lock);
		}
	}else
	{
		SEC_JACKDEV_DBG("SEND/END Button is %s but headset disconnect or irq disable.\n", state?"pressed":"released");
	}
}

static void open_sendend_switch_change(struct work_struct *ignored)
{

	struct sec_gpio_info   *det_jack = &hi->port.det_jack;
	struct sec_gpio_info   *send_end;
	int state, headset_state;
	SEC_JACKDEV_DBG("");
	del_timer(&send_end_key_event_timer);
	send_end_key_timer_token = 0;

        send_end = &hi->port.send_end_open;

	headset_state = gpio_get_value(det_jack->gpio) ^ det_jack->low_active;
	state = gpio_get_value(send_end->gpio) ^ send_end->low_active;

	if(headset_state && send_end_irq_token)//headset connect && send irq enable
	{
		SEC_JACKDEV_DBG(" open_sendend_switch_change sendend state %d\n",state);
		if(!state)  //suik_Fix sams as Sendend(Short)
		{
			SEC_JACKDEV_DBG(KERN_ERR "sendend isr work queue\n");
			switch_set_state(&switch_sendend, state);
			input_report_key(hi->input, KEYCODE_SENDEND, 0); //released    //suik_Fix
			input_sync(hi->input);
			printk("SEND/END %s.\n", "released");
			wake_unlock(&jack_sendend_wake_lock);
		}else
		{
			wake_lock(&jack_sendend_wake_lock);
			send_end_key_event_timer.expires = SEND_END_CHECK_TIME;
			add_timer(&send_end_key_event_timer);
			switch_set_state(&switch_sendend, state);
			SEC_JACKDEV_DBG("SEND/END %s.timer start \n", "pressed");
		}

	}else
	{
		SEC_JACKDEV_DBG("SEND/END Button is %s but headset disconnect or irq disable.\n", state?"pressed":"released");
	}
}
#if 0
static int sendend_timer_work_func(struct work_struct *ignored)
{
	struct sec_gpio_info   *det_jack = &hi->port.det_jack;
 	int headset_state;

	headset_state = gpio_get_value(det_jack->gpio) ^ det_jack->low_active;

	//send_end_enable = 1;


	if(send_end_pressed && !headset_state)
	{
		input_report_key(hi->input, KEYCODE_SENDEND,0);
		input_sync(hi->input);
		switch_set_state(&switch_sendend,0);
		send_end_pressed = 0;
		SEC_JACKDEV_DBG("Button is %s forcely.\n", "released");
	}


}
#endif
static DECLARE_WORK(jack_detect_work, jack_detect_change);
static DECLARE_WORK(sendend_switch_work, sendend_switch_change);
//static DECLARE_WORK(sendend_timer_work, sendend_timer_work_func);

//IRQ Handler
static irqreturn_t detect_irq_handler(int irq, void *dev_id)
{

	SEC_JACKDEV_DBG("jack isr");
	schedule_work(&jack_detect_work);
	return IRQ_HANDLED;
}

static void jack_detect_timer_handler(unsigned long arg)
{
	struct sec_gpio_info *det_jack = &hi->port.det_jack;
	int state;

	state = gpio_get_value(det_jack->gpio) ^ det_jack->low_active;

	if(state)
	{
		SEC_JACKDEV_DBG("jack_detect_timer_token is %d\n", jack_detect_timer_token);
		if(jack_detect_timer_token < DETECTION_CHECK_COUNT)
		{
			jack_detect_timer.expires = DETECTION_CHECK_TIME;
			add_timer(&jack_detect_timer);
			jack_detect_timer_token++;
			//gpio_set_value(GPIO_MICBIAS_EN, 1); //suik_Fix for saving Sleep current
		}
		else if(jack_detect_timer_token == DETECTION_CHECK_COUNT)
		{
			jack_detect_timer.expires = SEND_END_ENABLE_TIME;
			jack_detect_timer_token = 0;
			schedule_delayed_work(&detect_jack_type_work,50);
		}
		else if(jack_detect_timer_token == 4)
		{
			SEC_JACKDEV_DBG("mic bias enable add work queue \n");
			jack_detect_timer_token = 0;
		}
		else
			printk(KERN_ALERT "wrong jack_detect_timer_token count %d", jack_detect_timer_token);
	}
	else
		printk(KERN_ALERT "headset detach!! %d", jack_detect_timer_token);
}

static void send_end_key_event_timer_handler(unsigned long arg)
{

		struct sec_gpio_info   *det_jack = &hi->port.det_jack;
		struct sec_gpio_info   *send_end = &hi->port.send_end;
		struct sec_gpio_info   *send_end_open = &hi->port.send_end_open;
		int sendend_state, headset_state = 0;

		headset_state = gpio_get_value(det_jack->gpio) ^ det_jack->low_active;

	#if 1 //open_send_end do nothing..//suik_Fix
		if (sendend_type)
		{
			sendend_state = gpio_get_value(send_end_open->gpio) ^ send_end_open->low_active;
		}else
	#endif
		{
			sendend_state = gpio_get_value(send_end->gpio) ^ send_end->low_active;
		}

		if(headset_state && sendend_state)
		{
			if(send_end_key_timer_token < SEND_END_CHECK_COUNT)
			{
				send_end_key_timer_token++;
				send_end_key_event_timer.expires = SEND_END_CHECK_TIME;
				add_timer(&send_end_key_event_timer);
				SEC_JACKDEV_DBG("SendEnd Timer Restart %d", send_end_key_timer_token);
			}
			else if(send_end_key_timer_token == SEND_END_CHECK_COUNT)
			{
				printk("SEND/END is pressed\n");
				input_report_key(hi->input, KEYCODE_SENDEND, 1); //suik_Fix
				input_sync(hi->input);
				send_end_key_timer_token = 0;
			}
			else
				printk(KERN_ALERT "[JACK]wrong timer counter %d\n", send_end_key_timer_token);
		}else
			printk(KERN_ALERT "[JACK]GPIO Error\n");
}

static irqreturn_t send_end_irq_handler(int irq, void *dev_id)
{

 struct sec_gpio_info   *det_jack = &hi->port.det_jack;
   int headset_state;

  SEC_JACKDEV_DBG("[SHORT]send_end_irq_handler isr");
	del_timer(&send_end_key_event_timer);
	headset_state = gpio_get_value(det_jack->gpio) ^ det_jack->low_active;

	if (headset_state)
	{
		sendend_type = 0;
		schedule_work(&sendend_switch_work);
	}

	return IRQ_HANDLED;
}

//static DECLARE_WORK(jack_detect_work, jack_detect_change);
//static DECLARE_WORK(sendend_switch_work, sendend_switch_change);
static DECLARE_WORK(open_sendend_switch_work, open_sendend_switch_change);



static irqreturn_t send_end_open_irq_handler(int irq, void *dev_id)
{
   struct sec_gpio_info   *det_jack = &hi->port.det_jack;
   int headset_state;

  SEC_JACKDEV_DBG("[OPEN]send_end_open_irq_handler isr");
	del_timer(&send_end_key_event_timer);
	headset_state = gpio_get_value(det_jack->gpio) ^ det_jack->low_active;

	if (headset_state)
	{
		sendend_type = 0x01;
		schedule_work(&open_sendend_switch_work);		//suik_Fix
	}

	return IRQ_HANDLED;
}


//USER can select jack type if driver can't check the jack type
static int strtoi(char *buf)
{
	int ret;
	ret = buf[0]-48;
	return ret;
}

static ssize_t select_jack_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_INFO "[JACK] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t select_jack_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value = 0;
	struct sec_gpio_info   *det_jack = &hi->port.det_jack;
	struct sec_gpio_info   *send_end = &hi->port.send_end;
	int state = gpio_get_value(det_jack->gpio) ^ det_jack->low_active;

	SEC_JACKDEV_DBG("buf = %s", buf);
	SEC_JACKDEV_DBG("buf size = %d", sizeof(buf));
	SEC_JACKDEV_DBG("buf size = %d", strlen(buf));

	if(state)
	{
		if(current_jack_type_status != SEC_UNKNOWN_DEVICE)
		{
			printk(KERN_ERR "user can't select jack device if current_jack_status isn't unknown status");
			return -1;
		}

		if(sizeof(buf)!=1)
		{
			printk("input error\n");
			printk("Must be stored ( 1,2,4)\n");
			return -1;
		}

		value = strtoi(buf);
		SEC_JACKDEV_DBG("User  selection : 0X%x", value);

		switch(value)
		{
			case SEC_HEADSET_3_POLE_DEVICE:
			{
				current_jack_type_status = SEC_HEADSET_3_POLE_DEVICE;
				switch_set_state(&switch_jack_detection, current_jack_type_status);
				jack_input_selector(current_jack_type_status);
				break;
			}
			case SEC_HEADSET_4_POLE_DEVICE:
			{
				enable_irq (send_end->eint);
				send_end_irq_token++;
				current_jack_type_status = SEC_HEADSET_4_POLE_DEVICE;
				switch_set_state(&switch_jack_detection, current_jack_type_status);
				jack_input_selector(current_jack_type_status);
				break;
			}
			case SEC_TVOUT_DEVICE:
			{
				current_jack_type_status = SEC_TVOUT_DEVICE;
				gpio_set_value(GPIO_MICBIAS_EN, 0);
				switch_set_state(&switch_jack_detection, current_jack_type_status);
				jack_input_selector(current_jack_type_status);
				break;
			}
		}
	}
	else
	{
		printk(KERN_ALERT "Error : mic bias enable complete but headset detached!!\n");
		current_jack_type_status = SEC_JACK_NO_DEVICE;
		gpio_set_value(GPIO_MICBIAS_EN, 0);
	}

	return size;

}
static DEVICE_ATTR(select_jack, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, select_jack_show, select_jack_store);
static int sec_jack_probe(struct platform_device *pdev)
{
	int ret;
	struct sec_jack_platform_data *pdata = pdev->dev.platform_data;
	struct sec_gpio_info   *det_jack;
	struct sec_gpio_info   *send_end;
	struct input_dev	   *input;
	current_jack_type_status = SEC_JACK_NO_DEVICE;
	sendend_type =0x00;//short type always

	printk(KERN_INFO "SEC JACK: Registering jack driver\n");

	hi = kzalloc(sizeof(struct sec_jack_info), GFP_KERNEL);
	if (!hi)
		return -ENOMEM;

	memcpy (&hi->port, pdata->port, sizeof(struct sec_jack_port));

	input = hi->input = input_allocate_device();
	if (!input)
	{
		ret = -ENOMEM;
		printk(KERN_ERR "SEC JACK: Failed to allocate input device.\n");
		goto err_request_input_dev;
	}

	input->name = "sec_jack";
	set_bit(EV_SYN, input->evbit);
	set_bit(EV_KEY, input->evbit);
	set_bit(KEYCODE_SENDEND, input->keybit);

	ret = input_register_device(input);
	if (ret < 0)
	{
		printk(KERN_ERR "SEC JACK: Failed to register driver\n");
		goto err_register_input_dev;
	}

	init_timer(&jack_detect_timer);
	jack_detect_timer.function = jack_detect_timer_handler;

	init_timer(&send_end_key_event_timer);
	send_end_key_event_timer.function = send_end_key_event_timer_handler;

	SEC_JACKDEV_DBG("registering switch_sendend switch_dev sysfs sec_jack");

	ret = switch_dev_register(&switch_jack_detection);
	if (ret < 0)
	{
		printk(KERN_ERR "SEC JACK: Failed to register switch device\n");
		goto err_switch_dev_register;
	}

	ret = switch_dev_register(&switch_sendend);
	if (ret < 0)
	{
		printk(KERN_ERR "SEC JACK: Failed to register switch sendend device\n");
		goto err_switch_dev_register;
	}

	//Create JACK Device file in Sysfs
	jack_class = class_create(THIS_MODULE, "jack");
	if(IS_ERR(jack_class))
	{
		printk(KERN_ERR "Failed to create class(sec_jack)\n");
	}

	jack_selector_fs = device_create(jack_class, NULL, 0, NULL, "jack_selector");
	if (IS_ERR(jack_selector_fs))
		printk(KERN_ERR "Failed to create device(sec_jack)!= %ld\n", IS_ERR(jack_selector_fs));

	if (device_create_file(jack_selector_fs, &dev_attr_select_jack) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_select_jack.attr.name);

	//GPIO configuration for short SENDEND
	send_end = &hi->port.send_end;
	s3c_gpio_cfgpin(send_end->gpio, S3C_GPIO_SFN(send_end->gpio_af));
	s3c_gpio_setpull(send_end->gpio, S3C_GPIO_PULL_NONE);
	set_irq_type(send_end->eint, IRQ_TYPE_EDGE_BOTH);

	ret = request_irq(send_end->eint, send_end_irq_handler, IRQF_DISABLED, "sec_headset_send_end", NULL);

	SEC_JACKDEV_DBG("sended isr send=0X%x, ret =%d", send_end->eint, ret);
	if (ret < 0)
	{
		printk(KERN_ERR "SEC HEADSET: Failed to register send/end interrupt.\n");
		goto err_request_send_end_irq;
	}

	disable_irq(send_end->eint);

	if (1) //(HWREV >= 0x01)  //suik_Fix
	{
		//GPIO configuration for open SENDEND
		send_end = &hi->port.send_end_open;

		if(system_rev >= 0x08) //seonha
		{
			send_end->eint = IRQ_EINT2;
			send_end->gpio = GPIO_3P_SEND_END;
			send_end->gpio_af = GPIO_3P_SEND_END_AF;
		}

		s3c_gpio_cfgpin(send_end->gpio, S3C_GPIO_SFN(send_end->gpio_af));
		s3c_gpio_setpull(send_end->gpio, S3C_GPIO_PULL_NONE);
		set_irq_type(send_end->eint, IRQ_TYPE_EDGE_BOTH);

		ret = request_irq(send_end->eint, send_end_open_irq_handler, IRQF_DISABLED, "sec_headset_send_end_open", NULL);

		SEC_JACKDEV_DBG("sended open isr send=0X%x, ret =%d", send_end->eint, ret);
		if (ret < 0)
		{
			printk(KERN_ERR "SEC HEADSET: Failed to register send/end interrupt.\n");
			goto err_request_send_end_irq;
		}

		disable_irq(send_end->eint);
	}

	det_jack = &hi->port.det_jack;
	s3c_gpio_cfgpin(det_jack->gpio, S3C_GPIO_SFN(det_jack->gpio_af));
	s3c_gpio_setpull(det_jack->gpio, S3C_GPIO_PULL_NONE);
	set_irq_type(det_jack->eint, IRQ_TYPE_EDGE_BOTH);

	ret = request_irq(det_jack->eint, detect_irq_handler, IRQF_DISABLED, "sec_headset_detect", NULL);

	SEC_JACKDEV_DBG("det isr det=0X%x, ret =%d", det_jack->eint, ret);
	if (ret < 0)
	{
		printk(KERN_ERR "SEC HEADSET: Failed to register detect interrupt.\n");
		goto err_request_detect_irq;
	}
	//if (HWREV >= 0x01)  suik_Fix
	//	det_jack->low_active = 0;

	SEC_JACKDEV_DBG("sec_jack_probe HWREV =%d, 0x01=%d jack->low_active =%d", HWREV, 0x01, det_jack->low_active);

	gpio_direction_output(GPIO_MICBIAS_EN,1);
	s3c_gpio_slp_cfgpin(GPIO_MICBIAS_EN, S3C_GPIO_SLP_PREV);

	gpio_direction_output(GPIO_POPUP_SW_EN,1);  //POPUP_SW_EN  //suik_Fix
	//s3c_gpio_slp_cfgpin(GPIO_POPUP_SW_EN, S3C_GPIO_SLP_PREV);

	wake_lock_init(&jack_sendend_wake_lock, WAKE_LOCK_SUSPEND, "sec_jack");

	schedule_work(&jack_detect_work);

	return 0;

err_request_send_end_irq:
	free_irq(det_jack->eint, 0);
err_request_detect_irq:
	switch_dev_unregister(&switch_jack_detection);
	switch_dev_unregister(&switch_sendend);
err_switch_dev_register:
	input_unregister_device(input);
err_register_input_dev:
	input_free_device(input);
err_request_input_dev:
	kfree (hi);

	return ret;
}

static int sec_jack_remove(struct platform_device *pdev)
{
	SEC_JACKDEV_DBG("");
	input_unregister_device(hi->input);
	free_irq(hi->port.det_jack.eint, 0);
	free_irq(hi->port.send_end.eint, 0);
	free_irq(hi->port.send_end_open.eint, 0);
	switch_dev_unregister(&switch_jack_detection);
	switch_dev_unregister(&switch_sendend);
	return 0;
}

#ifdef CONFIG_PM
static int sec_jack_suspend(struct platform_device *pdev, pm_message_t state)
{
	#if 0 // froyo_merge_check
	if(current_jack_type_status == SEC_JACK_NO_DEVICE || current_jack_type_status == SEC_HEADSET_3_POLE_DEVICE)
	{
        if(!get_recording_status())
        {
		#if !defined(CONFIG_ARIES_NTT)
			gpio_set_value(GPIO_MICBIAS_EN, 0);
		#elif defined(CONFIG_ARIES_NTT)
			gpio_set_value(GPIO_SUB_MICBIAS_EN, 0);
#endif
        }

	}
	#endif
	return 0;
}
static int sec_jack_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define s3c_headset_resume	NULL
#define s3c_headset_suspend	NULL
#endif

static int __init sec_jack_init(void)
{
	SEC_JACKDEV_DBG("");
	return platform_driver_register(&sec_jack_driver);
}

static void __exit sec_jack_exit(void)
{
	platform_driver_unregister(&sec_jack_driver);
}

static struct platform_driver sec_jack_driver = {
	.probe		= sec_jack_probe,
	.remove		= sec_jack_remove,
	.suspend	= sec_jack_suspend,
	.resume		= sec_jack_resume,
	.driver		= {
		.name		= "sec_jack",
		.owner		= THIS_MODULE,
	},
};

module_init(sec_jack_init);
module_exit(sec_jack_exit);

MODULE_AUTHOR("Uk Kim <w0806.kim@samsung.com>");
MODULE_DESCRIPTION("SEC JACK detection driver");
MODULE_LICENSE("GPL");
