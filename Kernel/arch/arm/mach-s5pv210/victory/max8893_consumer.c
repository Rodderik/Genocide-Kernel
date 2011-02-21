/*
 *  linux/arch/arm/plat-s5pc11x/max8893_consumer.c
 *
 *  CPU frequency scaling for S5PC110
 *
 *  Copyright (C) 2009 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/stat.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <mach/regs-gpio.h>
#include <mach/regs-clock.h>
#include <plat/gpio-cfg.h>
#include <asm/gpio.h>
#include <plat/cpu-freq.h>
#include <linux/platform_device.h>
#include <linux/regulator/max8893.h>
#include <linux/regulator/consumer.h>



#define DBG(fmt...)
//#define DBG printk

#if 1 //victory.rev 02 (temp_pfe)
extern int hw_rev_adc;
#endif

static int max8893_consumer_probe(struct platform_device *pdev)
{
    int ret = 0;

	DBG("func =%s \n",__func__);	

	if(1150 <= hw_rev_adc){
      max8893_ldo_disable_direct(MAX8893_BUCK);
      DBG("max8893 Buck is disabled\n");        
	}

	return ret;	
}

static int max8893_consumer_remove(struct platform_device *pdev)
{
    int ret = 0;

	DBG("func =%s \n",__func__);	
	
	return ret;

}

static int max8893_consumer_suspend(struct platform_device *dev, pm_message_t state)
{
	int i, pmic_controling_list;

	DBG("func =%s \n",__func__);	

	if(hw_rev_adc <= 995){ 
	    max8893_ldo_disable_direct(MAX8893_BUCK);
      //max8893_ldo_set_voltage_direct(MAX8893_BUCK,1800000,1800000);
      DBG("max8893 Buck is disabled\n");        
	}
   
	//max8893_ldo_disable_direct(MAX8893_BUCK);
	//max8893_ldo_disable_direct(MAX8893_LDO1);
	//max8893_ldo_disable_direct(MAX8893_LDO2);
	//max8893_ldo_disable_direct(MAX8893_LDO3);
	//max8893_ldo_disable_direct(MAX8893_LDO4);
	//max8893_ldo_disable_direct(MAX8893_LDO5);	
	
	return 0;
}

static int max8893_consumer_resume(struct platform_device *dev)
{
	int i, saved_control;

	DBG("func =%s \n",__func__);	

  max8893_ldo_set_voltage_direct(MAX8893_LDO1,2900000,2900000);
  max8893_ldo_set_voltage_direct(MAX8893_LDO2,3000000,3000000); //20100628_inchul(from HW)
  max8893_ldo_set_voltage_direct(MAX8893_LDO3,3000000,3000000);
  max8893_ldo_set_voltage_direct(MAX8893_LDO4,3000000,3000000);
  max8893_ldo_set_voltage_direct(MAX8893_LDO5,1800000,1800000);
  max8893_ldo_set_voltage_direct(MAX8893_BUCK,1800000,1800000);

	if(hw_rev_adc <= 995){
	    max8893_ldo_enable_direct(MAX8893_BUCK);
      DBG("max8893 Buck is enabled\n");        
	}

	//max8893_ldo_enable_direct(MAX8893_BUCK);
	//max8893_ldo_enable_direct(MAX8893_LDO1);
	//max8893_ldo_enable_direct(MAX8893_LDO2);
	//max8893_ldo_enable_direct(MAX8893_LDO3);
	//max8893_ldo_enable_direct(MAX8893_LDO4);
	//max8893_ldo_enable_direct(MAX8893_LDO5);	
	
	return 0;
}

static struct platform_driver max8893_consumer_driver = {
	.driver = {
		   .name = "max8893-consumer",
		   .owner = THIS_MODULE,
		   },
	.probe = max8893_consumer_probe,
	.remove = max8893_consumer_remove,
    .suspend = max8893_consumer_suspend, 
    .resume = max8893_consumer_resume,

};

static int max8893_consumer_init(void)
{
	return platform_driver_register(&max8893_consumer_driver);
}
late_initcall(max8893_consumer_init);

static void max8893_consumer_exit(void)
{
	platform_driver_unregister(&max8893_consumer_driver);
}
module_exit(max8893_consumer_exit);

MODULE_AUTHOR("Amit Daniel");
MODULE_DESCRIPTION("MAX 8893 consumer driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:max8893-consumer");
