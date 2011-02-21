/* linux/drivers/media/video/samsung/tv20/hpd.c
 *
 * hpd interface ftn file for Samsung TVOut driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/irq.h>
#include <linux/kobject.h>

#include <asm/io.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>

#include "s5p_tv.h"
#include "hpd.h"

#define HPDDEBUG

#define USEEXTINT

#ifdef HPDDEBUG
#define HPDIFPRINTK(fmt, args...) \
	printk("[HPD_IF] %s: " fmt, __FUNCTION__ , ## args)
#else
#define HPDIFPRINTK(fmt, args...)
#endif

static struct hpd_struct hpd_struct;

#ifndef USEEXTINT
static int last_hpd_state;

static bool hdmi_on = false;
#endif

static DECLARE_WORK(hpd_work, (void *)s5p_handle_cable);

int s5p_hpd_get_state(void)
{
       return atomic_read(&hpd_struct.state);
}

int s5p_hpd_open(struct inode *inode, struct file *file)
{
#ifdef USEEXTINT
#else
	/* adjust the duration of HPD detection */
	s5p_tv_clk_gate(true);
	hdmi_on = true;

	s5p_hdmi_hpd_gen();

	s3c_gpio_cfgpin(S5PV210_GPH1(5), S3C_GPIO_SFN(0x4));
	s3c_gpio_setpull(S5PV210_GPH1(5), S3C_GPIO_PULL_UP);
	
	s5p_hdmi_enable_interrupts(HDMI_IRQ_HPD_PLUG);
	s5p_hdmi_enable_interrupts(HDMI_IRQ_HPD_UNPLUG);
#endif
	
	
	return 0;
}

int s5p_hpd_release(struct inode *inode, struct file *file)
{
	/* disable HPD interrupts */
#ifdef USEEXTINT
#else
	s5p_tv_clk_gate(false);
	hdmi_on = false;

	s5p_hdmi_disable_interrupts(HDMI_IRQ_HPD_PLUG);
	s5p_hdmi_disable_interrupts(HDMI_IRQ_HPD_UNPLUG);
#endif

	return 0;
}

ssize_t s5p_hpd_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
	ssize_t retval;
#ifdef USEEXTINT
	spin_lock_irq(&hpd_struct.lock);

	retval = put_user(atomic_read(&hpd_struct.state),
		(unsigned int __user *) buffer);

	spin_unlock_irq(&hpd_struct.lock);
#else
	if (wait_event_interruptible(hpd_struct.waitq, atomic_read(&hpd_struct.state) != -1))
		return -ERESTARTSYS;

	spin_lock_irq(&hpd_struct.lock);

	retval = put_user(atomic_read(&hpd_struct.state), (unsigned int __user *) buffer);

	atomic_set(&hpd_struct.state, -1);

	spin_unlock_irq(&hpd_struct.lock);
#endif
	return retval;
}

unsigned int s5p_hpd_poll(struct file *file, poll_table *wait)
{
	poll_wait(file, &hpd_struct.waitq, wait);

	if (atomic_read(&hpd_struct.state) != -1)
		return POLLIN | POLLRDNORM;

	return 0;
}

static const struct file_operations hpd_fops = {
	.owner   = THIS_MODULE,
	.open    = s5p_hpd_open,
	.release = s5p_hpd_release,
	.read    = s5p_hpd_read,
	.poll    = s5p_hpd_poll,
};

static struct miscdevice hpd_misc_device = {
	HPD_MINOR,
	"HPD",
	&hpd_fops,
};

/*
 * HPD interrupt handler
 *
 * Handles interrupt requests from HPD hardware.
 * Handler changes value of internal variable and notifies waiting thread.
 */
irqreturn_t s5p_hpd_irq_handler(int irq, void *pw)
{
#ifdef USEEXTINT
	spin_lock_irq(&hpd_struct.lock);

	if (gpio_get_value(S5PV210_GPH1(5))) {
		if (atomic_read(&hpd_struct.state) == HPD_HI)
			goto out;
		atomic_set(&hpd_struct.state, HPD_HI);
	} else {
		if (atomic_read(&hpd_struct.state) == HPD_LO)
			goto out;
		atomic_set(&hpd_struct.state, HPD_LO);
	}
	
	if(atomic_read(&hpd_struct.state)){
		set_irq_type(IRQ_EINT13, IRQ_TYPE_LEVEL_LOW);
	}else{
		set_irq_type(IRQ_EINT13, IRQ_TYPE_LEVEL_HIGH);
	}

	/* Send UEvent for HPD - added by jyg1004 */
	schedule_work(&hpd_work);

	spin_unlock_irq(&hpd_struct.lock);

	HPDIFPRINTK("hpd_status = %d\n", atomic_read(&hpd_struct.state));
	return IRQ_HANDLED;
out:
	spin_unlock_irq(&hpd_struct.lock);

	return IRQ_HANDLED;
#else
	u8 flag;
	int ret = IRQ_HANDLED;
	
	/* read flag register */
	flag = s5p_hdmi_get_interrupts();

	/* is this our interrupt? */

	if (!(flag & (1 << HDMI_IRQ_HPD_PLUG | 1 << HDMI_IRQ_HPD_UNPLUG))) {
		ret = IRQ_NONE;
		goto out;
	}

	/* workaround: ignore HPD IRQ caused by reseting HDCP engine */
	if (s5p_hdmi_get_swhpd_status()) {
		s5p_hdmi_swhpd_disable();
		/* clear pending bit */
		s5p_hdmi_clear_pending(HDMI_IRQ_HPD_UNPLUG);
		s5p_hdmi_clear_pending(HDMI_IRQ_HPD_PLUG);
		ret = IRQ_HANDLED;
		goto out;
	}

	if (flag == (1 << HDMI_IRQ_HPD_PLUG | 1 << HDMI_IRQ_HPD_UNPLUG)) {
		HPDIFPRINTK("HPD_HI && HPD_LO\n");

		if (last_hpd_state == HPD_HI && s5p_hdmi_get_hpd_status())
			//if ( last_hpd_state == HPD_HI )
			flag = 1 << HDMI_IRQ_HPD_UNPLUG;
		else
			flag = 1 << HDMI_IRQ_HPD_PLUG;
	}

	if (flag & (1 << HDMI_IRQ_HPD_PLUG)) {
		HPDIFPRINTK("HPD_HI\n");
		/* clear pending bit */
		s5p_hdmi_clear_pending(HDMI_IRQ_HPD_PLUG);
		s5p_hdmi_clear_pending(HDMI_IRQ_HPD_UNPLUG);
		atomic_set(&hpd_struct.state, HPD_HI);
		// workaround: enable HDMI_IRQ_HPD_UNPLUG interrupt
		s5p_hdmi_disable_interrupts(HDMI_IRQ_HPD_PLUG);
		s5p_hdmi_enable_interrupts(HDMI_IRQ_HPD_UNPLUG);		
		last_hpd_state = HPD_HI;
		wake_up_interruptible(&hpd_struct.waitq);
	} else if (flag & (1 << HDMI_IRQ_HPD_UNPLUG)) {
		HPDIFPRINTK("HPD_LO\n");
		/* clear pending bit */
		s5p_hdmi_clear_pending(HDMI_IRQ_HPD_PLUG);
		s5p_hdmi_clear_pending(HDMI_IRQ_HPD_UNPLUG);
		atomic_set(&hpd_struct.state, HPD_LO);
		// workaround: disable HDMI_IRQ_HPD_UNPLUG interrupt
		last_hpd_state = HPD_LO;
		s5p_hdmi_disable_interrupts(HDMI_IRQ_HPD_UNPLUG);
		s5p_hdmi_enable_interrupts(HDMI_IRQ_HPD_PLUG);
		wake_up_interruptible(&hpd_struct.waitq);
	}

out:
	return IRQ_HANDLED;

#endif
}

static int __init s5p_hpd_probe(struct platform_device *pdev)
{
	if (misc_register(&hpd_misc_device)) {
		printk(KERN_WARNING " Couldn't register device 10, %d.\n", HPD_MINOR);
		return -EBUSY;
	}

	init_waitqueue_head(&hpd_struct.waitq);

	spin_lock_init(&hpd_struct.lock);

	atomic_set(&hpd_struct.state, -1);

#ifdef USEEXTINT
	s3c_gpio_cfgpin(S5PV210_GPH1(5), S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(S5PV210_GPH1(5), S3C_GPIO_PULL_UP);

	if (gpio_get_value(S5PV210_GPH1(5)))
		atomic_set(&hpd_struct.state, HPD_HI);
	else
		atomic_set(&hpd_struct.state, HPD_LO);

	set_irq_type(IRQ_EINT13, IRQ_TYPE_EDGE_BOTH);

	if (request_irq(IRQ_EINT13, s5p_hpd_irq_handler, IRQF_DISABLED, "hpd", s5p_hpd_irq_handler)){
		printk(KERN_ERR  "failed to install %s irq\n", "hpd");
		misc_deregister(&hpd_misc_device);
		return -EIO;
	}

#else
	/* must be checked */
	s5p_hdmi_register_isr(s5p_hpd_irq_handler, (u8)HDMI_IRQ_HPD_PLUG);
	s5p_hdmi_register_isr(s5p_hpd_irq_handler, (u8)HDMI_IRQ_HPD_UNPLUG);	
#endif

	return 0;
}

/*
 *  Remove
 */
static int s5p_hpd_remove(struct platform_device *pdev)
{
	return 0;
}


#ifdef CONFIG_PM
/*
 *  Suspend
 */
int s5p_hpd_suspend(struct platform_device *dev, pm_message_t state)
{
#ifndef USEEXTINT
	if ( hdmi_on )
		s5p_tv_clk_gate(false);
#endif
		
	return 0;
}

/*
 *  Resume
 */
int s5p_hpd_resume(struct platform_device *dev)
{
#ifndef USEEXTINT
	if ( hdmi_on )
		s5p_tv_clk_gate(true);
#endif
	
	return 0;
}
#else
#define s5p_hpd_suspend NULL
#define s5p_hpd_resume NULL
#endif

static struct platform_driver s5p_hpd_driver = {
	.probe		= s5p_hpd_probe,
	.remove		= s5p_hpd_remove,
	.suspend	= s5p_hpd_suspend,
	.resume		= s5p_hpd_resume,
	.driver		= {
		.name	= "s5p-hpd",
		.owner	= THIS_MODULE,
	},
};

static char banner[] __initdata =  "S5PC11X HPD Driver, (c) 2009 Samsung Electronics\n";

int __init s5p_hpd_init(void)
{
	int ret;

	printk(banner);

	ret = platform_driver_register(&s5p_hpd_driver);

	if (ret) {
		printk(KERN_ERR "Platform Device Register Failed %d\n", ret);
		return -1;
	}

	return 0;
}

static void __exit s5p_hpd_exit(void)
{
	misc_deregister(&hpd_misc_device);
}

module_init(s5p_hpd_init);
module_exit(s5p_hpd_exit);


