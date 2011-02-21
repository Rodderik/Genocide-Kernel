/*
 * linux/drivers/power/s3c6410_battery.c
 *
 * Battery measurement code for S3C6410 platform.
 *
 * based on palmtx_battery.c
 *
 * Copyright (C) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/wakelock.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/battery.h>
#include <plat/gpio-cfg.h>
#include <linux/earlysuspend.h>
#include <linux/io.h>
#include <mach/regs-clock.h>
#include <mach/regs-power.h>
#include <mach/map.h>

#include "s5pc110_battery.h"

static struct wake_lock vbus_wake_lock;
static struct wake_lock low_batt_wake_lock;	// lobat pwroff

#ifdef __FUEL_GAUGES_IC__
extern int fg_init(void);
extern void fg_exit(void);
extern void fuel_gauge_rcomp(void);
extern unsigned int fg_read_soc(void);
extern unsigned int fg_read_vcell(void);
extern unsigned int fg_reset_soc(void);
#endif /* __FUEL_GAUGES_IC__ */

#ifdef CONFIG_KERNEL_DEBUG_SEC
extern void kernel_sec_clear_upload_magic_number(void);	// hanapark_DF24
extern void kernel_sec_set_upload_magic_number(void);	// hanapark_DF24
#endif

/* Prototypes */
extern int s3c_adc_get_adc_data(int channel);
extern void MAX8998_IRQ_init(void);
extern void maxim_charging_control(unsigned int dev_type  , unsigned int cmd);
extern void set_low_bat_interrupt(int on);	// lobat pwroff
extern void charging_stop_without_magic_number(void); // hanapark_DF01
extern u8 FSA9480_Get_JIG_OnOff_Status(void);
unsigned char maxim_chg_status(void);

static unsigned int s3c_bat_check_v_f(void);

#define LPM_MODE


static ssize_t s3c_bat_show_property(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf);
static ssize_t s3c_bat_store(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count);
static void s3c_set_chg_en(int enable);


#ifdef __TEST_MODE_INTERFACE__
static struct power_supply *s3c_power_supplies_test = NULL;
static void polling_timer_func(unsigned long unused);
static void s3c_bat_status_update(struct power_supply *bat_ps);
#endif /* __TEST_MODE_INTERFACE__ */

static void use_browser_timer_func(unsigned long unused);
static void use_wimax_timer_func(unsigned long unused);
static void use_data_call_timer_func(unsigned long unused);
static int s3c_bat_use_browser(int onoff);
static int s3c_bat_use_data_call(int onoff);

#define USE_BROWSER		(0x1 << 3)
#define USE_WIMAX			(0x1 << 4)
#define USE_HOTSPOT		(0x1 << 5)
#define USE_TALK_WCDMA	(0x1 << 6)
#define USE_DATA_CALL		(0x1 << 7)

#define TRUE	1
#define FALSE	0

#define ADC_DATA_ARR_SIZE	6
#define ADC_TOTAL_COUNT		10
#define POLLING_INTERVAL	2000
#ifdef __TEST_MODE_INTERFACE__
#define POLLING_INTERVAL_TEST	1000
#endif /* __TEST_MODE_INTERFACE__ */

static struct work_struct bat_work;
static struct work_struct cable_work;
static struct device *dev;
static struct timer_list polling_timer;

static struct timer_list use_wimax_timer;
static struct timer_list use_browser_timer;
static struct timer_list use_data_call_timer;

static int s3c_battery_initial;
static int force_update;

static int batt_max;
static int batt_full;
static int batt_safe_rech;
static int batt_almost;
static int batt_high;
static int batt_medium;
static int batt_low;
static int batt_critical;
static int batt_min;
static int batt_off;

static unsigned int start_time_msec;
static unsigned int total_time_msec;

static char *status_text[] = {
	[POWER_SUPPLY_STATUS_UNKNOWN] =		"Unknown",
	[POWER_SUPPLY_STATUS_CHARGING] =	"Charging",
	[POWER_SUPPLY_STATUS_DISCHARGING] =	"Discharging",
	[POWER_SUPPLY_STATUS_NOT_CHARGING] =	"Not Charging",
	[POWER_SUPPLY_STATUS_FULL] =		"Full",
};

typedef enum {
	CHARGER_BATTERY = 0,
	CHARGER_USB,
	CHARGER_AC,
	CHARGER_DISCHARGE
} charger_type_t;

struct battery_info {
	u32 batt_id;		/* Battery ID from ADC */
	s32 batt_vol;		/* Battery voltage from ADC */
	s32 batt_vol_adc;	/* Battery ADC value */
	s32 batt_vol_adc_cal;	/* Battery ADC value (calibrated)*/
	s32 batt_temp;		/* Battery Temperature (C) from ADC */
	s32 batt_temp_adc;	/* Battery Temperature ADC value */
	s32 batt_temp_adc_cal;	/* Battery Temperature ADC value (calibrated) */
	s32 batt_current;	/* Battery current from ADC */
	u32 level;		/* formula */
	u32 charging_source;	/* 0: no cable, 1:usb, 2:AC */
	u32 charging_enabled;	/* 0: Disable, 1: Enable */
	u32 batt_health;	/* Battery Health (Authority) */
	u32 batt_is_full;       /* 0 : Not full 1: Full */
	u32 batt_is_recharging; /* 0 : Not recharging 1: Recharging */
	s32 batt_vol_adc_aver;	/* batt vol adc average */
#ifdef __FUEL_GAUGES_IC__
	u32 decimal_point_level;	// lobat pwroff
#endif
#ifdef __TEST_MODE_INTERFACE__
	u32 batt_test_mode;	/* test mode */
	s32 batt_vol_aver;	/* batt vol average */
	s32 batt_temp_aver;	/* batt temp average */
	s32 batt_temp_adc_aver;	/* batt temp adc average */
	s32 batt_v_f_adc;	/* batt V_F adc */
        s32 batt_slate_mode; /* slate mode */
     
#endif /* __TEST_MODE_INTERFACE__ */
};

/* lock to protect the battery info */
static DEFINE_MUTEX(work_lock);

struct s3c_battery_info {
	int present;
	int polling;
	unsigned int polling_interval;
	unsigned int device_state;

	struct battery_info bat_info;
#ifdef LPM_MODE
	unsigned int charging_mode_booting;
#endif
};
static struct s3c_battery_info s3c_bat_info;

struct adc_sample_info {
	unsigned int cnt;
	int total_adc;
	int average_adc;
	int adc_arr[ADC_TOTAL_COUNT];
	int index;
};
static struct adc_sample_info adc_sample[ENDOFADC];

struct battery_driver 
{
	struct early_suspend	early_suspend;
};
struct battery_driver *battery = NULL;


extern charging_device_type curent_device_type;

#ifdef LPM_MODE
void charging_mode_set(unsigned int val)
{
	s3c_bat_info.charging_mode_booting=val;
}
unsigned int charging_mode_get(void)
{
	return s3c_bat_info.charging_mode_booting;
}
#endif

static int get_usb_power_state(void)
{
	if(curent_device_type==PM_CHARGER_USB_INSERT)
		return 1;
	else
		return 0;
}	

static inline int s3c_adc_get_adc_data_ex(int channel) {
	if (channel == S3C_ADC_TEMPERATURE)
		return ((4096 - s3c_adc_get_adc_data(channel)) >> 4);		// 2010.05.08.
	else if (channel == S3C_ADC_HW_VERSION)	// hanapark_Victory
		return (s3c_adc_get_adc_data(channel));
	else
		return (s3c_adc_get_adc_data(channel) >> 2);
}

#if 0
static unsigned long calculate_average_adc(adc_channel_type channel, int adc)
{
	unsigned int cnt = 0;
	int total_adc = 0;
	int average_adc = 0;
	int index = 0;

	cnt = adc_sample[channel].cnt;
	total_adc = adc_sample[channel].total_adc;

	if (adc < 0 || adc == 0) {
		dev_err(dev, "%s: invalid adc : %d\n", __func__, adc);
		adc = adc_sample[channel].average_adc;
	}

	if( cnt < ADC_TOTAL_COUNT ) {
		adc_sample[channel].adc_arr[cnt] = adc;
		adc_sample[channel].index = cnt;
		adc_sample[channel].cnt = ++cnt;

		total_adc += adc;
		average_adc = total_adc / cnt;
	} else {
#if 0
		if (channel == S3C_ADC_VOLTAGE &&
				!s3c_bat_info.bat_info.charging_enabled && 
				adc > adc_sample[channel].average_adc) {
			dev_dbg(dev, "%s: adc over avg : %d\n", __func__, adc);
			return adc_sample[channel].average_adc;
		}
#endif
		index = adc_sample[channel].index;
		if (++index >= ADC_TOTAL_COUNT)
			index = 0;

		total_adc = (total_adc - adc_sample[channel].adc_arr[index]) + adc;
		average_adc = total_adc / ADC_TOTAL_COUNT;

		adc_sample[channel].adc_arr[index] = adc;
		adc_sample[channel].index = index;
	}

	adc_sample[channel].total_adc = total_adc;
	adc_sample[channel].average_adc = average_adc;

	dev_dbg(dev, "%s: ch:%d adc=%d, avg_adc=%d\n",
			__func__, channel, adc, average_adc);
	return average_adc;
}
#endif

static int s3c_bat_get_adc_data(adc_channel_type adc_ch)
{
	int adc_arr[ADC_DATA_ARR_SIZE];
	int adc_max = 0;
	int adc_min = 0;
	int adc_total = 0;
	int i;

	for (i = 0; i < ADC_DATA_ARR_SIZE; i++) {
		adc_arr[i] = s3c_adc_get_adc_data_ex(adc_ch);
		dev_dbg(dev, "%s: adc_arr = %d\n", __func__, adc_arr[i]);
		if ( i != 0) {
			if (adc_arr[i] > adc_max) 
				adc_max = adc_arr[i];
			else if (adc_arr[i] < adc_min)
				adc_min = adc_arr[i];
		} else {
			adc_max = adc_arr[0];
			adc_min = adc_arr[0];
		}
		adc_total += adc_arr[i];
	}

	dev_dbg(dev, "%s: adc_max = %d, adc_min = %d\n",
			__func__, adc_max, adc_min);

	return (adc_total - adc_max - adc_min) / (ADC_DATA_ARR_SIZE - 2);
}

#ifdef __TEMPERATURE_TEST__
int batt_temp_test_mode = 0;	// 0: auto test, 1: manual block, 2: manual normal (recover)
unsigned int start_time_test = 0;
unsigned int test_interval = 2;
#endif

#ifdef __MANUAL_TEMP_TEST__
static int manual_temp_adc = 180;
#endif

#ifdef __SOC_TEST__
static int soc_test = 100;
#endif

static unsigned long s3c_read_temp(struct power_supply *bat_ps)
{
	int adc = 0;

	dev_dbg(dev, "%s\n", __func__);
	adc = s3c_bat_get_adc_data(S3C_ADC_TEMPERATURE);

#ifdef __MANUAL_TEMP_TEST__
	adc = manual_temp_adc;
#endif

#ifdef __TEMPERATURE_TEST__	// hanapark (temperature test)
{
	unsigned int total_time;

	if (batt_temp_test_mode == 0)	// auto test
	{
		if (!start_time_test)
			start_time_test = jiffies_to_msecs(jiffies);

		if (start_time_test)
		{
			total_time = jiffies_to_msecs(jiffies) - start_time_test;

			if ((0*60*1000) < total_time && total_time < (test_interval*60*1000))
			{
				if (charging_mode_get())
					adc = TEMP_HIGH_RECOVER_LPM - 2;
				else
					adc = TEMP_HIGH_RECOVER - 2;
			}
			else if ((test_interval*60*1000) < total_time && total_time < (test_interval*2*60*1000))
			{
				if (charging_mode_get())
					adc = TEMP_HIGH_BLOCK_LPM + 2;
				else
					adc = TEMP_HIGH_BLOCK + 2;
			}
			else if (total_time > (test_interval*2*60*1000))
			{
				start_time_test = jiffies_to_msecs(jiffies);	// Restart !
			}
		}
	}	
	else if (batt_temp_test_mode == 1)	// high block
	{
		if (charging_mode_get())
			adc = TEMP_HIGH_BLOCK_LPM + 2;
		else
			adc = TEMP_HIGH_BLOCK + 2;
	}
}
#endif	/* __TEMPERATURE_TEST__*/

	dev_dbg(dev, "%s: adc = %d\n", __func__, adc);
	s3c_bat_info.bat_info.batt_temp_adc = adc;
	return adc;
}

static int is_over_abs_time(void)
{
	unsigned int total_time;

	if (!start_time_msec)
		return 0;

	if (s3c_bat_info.bat_info.batt_is_recharging)
		total_time = TOTAL_RECHARGING_TIME;
	else
		total_time = TOTAL_CHARGING_TIME;

	if(time_after((unsigned long)jiffies, (unsigned long)(start_time_msec + total_time))) 
	{ 	 
		printk("%s abs time over (abs time: %u, start time: %u)\n",__func__, total_time, start_time_msec);
		return 1;
	} 
	else
		return 0;
}

#ifdef __CHECK_CHG_CURRENT__
static void check_chg_current(struct power_supply *bat_ps)
{
	static int cnt = 0;
	unsigned long chg_current = 0; 

	chg_current = s3c_bat_get_adc_data(S3C_ADC_CHG_CURRENT);
	s3c_bat_info.bat_info.batt_current = chg_current;

	if ( (s3c_bat_info.bat_info.batt_vol >= FULL_CHARGE_COND_VOLTAGE) &&		// full charge condition added
		(s3c_bat_info.bat_info.batt_current <= CURRENT_OF_FULL_CHG) ) {
#ifdef __TEST_MODE_INTERFACE__
		if (s3c_bat_info.bat_info.batt_test_mode == 1)	// test mode (interval 1 sec)
			cnt++;
		else	// non-test mode (interval 2 sec)
#endif
			cnt += 2;

		if (cnt >= CHG_CURRENT_COUNT)
		{
			dev_info(dev, "%s: battery full (bat_vol = %d) \n", __func__, s3c_bat_info.bat_info.batt_vol);
			s3c_set_chg_en(0);
			s3c_bat_info.bat_info.batt_is_full = 1;
			force_update = 1;
			cnt = 0;
		}
	} else {
		cnt = 0;
	}
}
#endif /* __CHECK_CHG_CURRENT__ */

#ifdef __ADJUST_RECHARGE_ADC__
static void check_recharging_bat(int bat_vol)
{
	static int cnt = 0;
	static int cnt_backup = 0;

	if (s3c_bat_info.bat_info.batt_is_full)
		dev_info(dev, "check recharge : bat_vol = %d \n", __func__, bat_vol);

	if (s3c_bat_info.bat_info.batt_is_full && 
		!s3c_bat_info.bat_info.charging_enabled &&
		/*batt_recharging != -1 &&*/ bat_vol < RECHARGE_COND_VOLTAGE/*batt_recharging*/) {	// hanapark (recharge voltage)
#ifdef __TEST_MODE_INTERFACE__
		if (s3c_bat_info.bat_info.batt_test_mode == 1)	// test mode (interval 1 sec)
			cnt++;
		else	// non-test mode (interval 2 sec)
#endif
			cnt += 2;

		if (cnt >= BATT_RECHARGE_COUNT) {	// hanapark
			dev_info(dev, "%s: recharging (bat_vol = %d) \n", __func__, bat_vol);
			s3c_bat_info.bat_info.batt_is_recharging = 1;
			s3c_set_chg_en(1);
			cnt = 0;
		}
	} else {
		cnt = 0;
	}

	if (s3c_bat_info.bat_info.batt_is_full && 
		!s3c_bat_info.bat_info.charging_enabled &&
		bat_vol < RECHARGE_COND_VOLTAGE_BACKUP) {	// recharge condition added
#ifdef __TEST_MODE_INTERFACE__
		if (s3c_bat_info.bat_info.batt_test_mode == 1)	// test mode (interval 1 sec)
			cnt_backup++;
		else	// non-test mode (interval 2 sec)
#endif
			cnt_backup += 2;

		if (cnt_backup >= BATT_RECHARGE_COUNT) {
			dev_info(dev, "%s: recharging backup (bat_vol = %d) \n", __func__, bat_vol);
			s3c_bat_info.bat_info.batt_is_recharging = 1;
			s3c_set_chg_en(1);
			cnt_backup = 0;
		}
	} else {
		cnt_backup = 0;
	}
}
#endif /* __ADJUST_RECHARGE_ADC__ */

static int s3c_get_bat_level(struct power_supply *bat_ps)
{
	int fg_soc = -1;
	int fg_vcell = -1;

#ifdef __SOC_TEST
	fg_soc = soc_test;
#else
	if ((fg_soc = fg_read_soc()) < 0) {
		dev_err(dev, "%s: Can't read soc!!!\n", __func__);
		fg_soc = s3c_bat_info.bat_info.level;
	}
#endif

	if ((fg_vcell = fg_read_vcell()) < 0) {
		dev_err(dev, "%s: Can't read vcell!!!\n", __func__);
		fg_vcell = s3c_bat_info.bat_info.batt_vol;
	} else
		s3c_bat_info.bat_info.batt_vol = fg_vcell;

	if (is_over_abs_time()) {
		fg_soc = 100;
		s3c_bat_info.bat_info.batt_is_full = 1;
		dev_info(dev, "%s: charging time is over\n", __func__);
		s3c_set_chg_en(0);
		goto __end__;
	}

#ifdef __CHECK_CHG_CURRENT__
		if (s3c_bat_info.bat_info.charging_enabled) {
			check_chg_current(bat_ps);
			if (s3c_bat_info.bat_info.batt_is_full)
				fg_soc = 100;
		} 
#endif /* __CHECK_CHG_CURRENT__ */
#ifdef __ADJUST_RECHARGE_ADC__
	check_recharging_bat(fg_vcell);
#endif /* __ADJUST_RECHARGE_ADC__ */

__end__:
	dev_dbg(dev, "%s: fg_vcell = %d, fg_soc = %d, is_full = %d\n",
			__func__, fg_vcell, fg_soc, 
			s3c_bat_info.bat_info.batt_is_full);
	return fg_soc;
}

static int s3c_get_bat_vol(struct power_supply *bat_ps)
{
	return s3c_bat_info.bat_info.batt_vol;
}

static u32 s3c_get_bat_health(void)
{
	return s3c_bat_info.bat_info.batt_health;
}

static void s3c_set_bat_health(u32 batt_health)
{
	s3c_bat_info.bat_info.batt_health = batt_health;
}

static void s3c_set_time_for_charging(int mode) {
	if (mode) {
		/* record start time for abs timer */
		start_time_msec = jiffies;
		//dev_info(dev, "%s: start_time(%u)\n", __func__,
		//		start_time_msec);
	} else {
		/* initialize start time for abs timer */
		start_time_msec = 0;
		total_time_msec = 0;
		//dev_info(dev, "%s: reset abs timer\n", __func__);
	}
}

static void s3c_set_chg_en(int enable)
{
	int chg_en_val = maxim_chg_status();

	if (enable) {
		if (chg_en_val) {
			if(curent_device_type==PM_CHARGER_TA)
				maxim_charging_control(PM_CHARGER_TA, TRUE);
			else if (curent_device_type==PM_CHARGER_USB_INSERT)
				maxim_charging_control(PM_CHARGER_USB_INSERT, TRUE);
			else{
				maxim_charging_control(PM_CHARGER_DEFAULT, FALSE);
			}
			s3c_set_time_for_charging(1);
		}
	} else {
			maxim_charging_control(PM_CHARGER_DEFAULT, FALSE);
			s3c_set_time_for_charging(0);
			s3c_bat_info.bat_info.batt_is_recharging = 0;
			s3c_bat_info.bat_info.batt_current = 0;	// hanapark_Victory
	}
	s3c_bat_info.bat_info.charging_enabled = enable;
}

static void s3c_temp_control(int mode) {
	if (s3c_bat_get_adc_data(S3C_ADC_HW_VERSION) < 540)	// hanapark_DE14 (Temperature issue depends on HW Rev.)
	{
		mode = POWER_SUPPLY_HEALTH_GOOD;
	}

	switch (mode) {
	case POWER_SUPPLY_HEALTH_GOOD:
		dev_info(dev, "%s: GOOD\n", __func__);
		s3c_set_bat_health(mode);
		break;
	case POWER_SUPPLY_HEALTH_OVERHEAT:
		dev_info(dev, "%s: OVERHEAT\n", __func__);
		s3c_set_bat_health(mode);
		s3c_bat_info.bat_info.batt_is_full = 0;	// hanapark_Atlas
		break;
	case POWER_SUPPLY_HEALTH_FREEZE:
		dev_info(dev, "%s: FREEZE\n", __func__);
		s3c_set_bat_health(mode);
		s3c_bat_info.bat_info.batt_is_full = 0;	// hanapark_Atlas
		break;
	default:
		break;
	}
	schedule_work(&cable_work);
}

static int s3c_get_bat_temp(struct power_supply *bat_ps)
{
	int temp = 0;
	int array_size = 0;
	int i = 0;
	int temp_adc = s3c_read_temp(bat_ps);
	int health = s3c_get_bat_health();
	int health_new = health;	// hanapark_DF21
#ifdef __TEST_MODE_INTERFACE__
	s3c_bat_info.bat_info.batt_temp_adc_aver = temp_adc;
#endif /* __TEST_MODE_INTERFACE__ */

#ifdef LPM_MODE
	if (charging_mode_get() == 1)
	{
		if (temp_adc >= TEMP_HIGH_BLOCK_LPM) {
			if (health != POWER_SUPPLY_HEALTH_OVERHEAT &&
					health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
//				s3c_temp_control(POWER_SUPPLY_HEALTH_OVERHEAT);
				health_new = POWER_SUPPLY_HEALTH_OVERHEAT;	// hanapark_DF21
				printk("%s: temp code = %d, overheat (block) \n", __func__, temp_adc);
			}
		} else if (temp_adc <= TEMP_HIGH_RECOVER_LPM &&
				temp_adc >= TEMP_LOW_RECOVER_LPM) {
			if (health == POWER_SUPPLY_HEALTH_OVERHEAT ||
					health == POWER_SUPPLY_HEALTH_FREEZE) {
//				s3c_temp_control(POWER_SUPPLY_HEALTH_GOOD);				
				health_new = POWER_SUPPLY_HEALTH_GOOD;	// hanapark_DF21
				printk("%s: temp code = %d, good (recover) \n", __func__, temp_adc);
			}
		} else if (temp_adc <= TEMP_LOW_BLOCK_LPM) {
			if (health != POWER_SUPPLY_HEALTH_FREEZE &&
					health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
//				s3c_temp_control(POWER_SUPPLY_HEALTH_FREEZE);
				health_new = POWER_SUPPLY_HEALTH_FREEZE;	// hanapark_DF21
				printk("%s: temp code = %d, freeze (block) \n", __func__, temp_adc);
			}
		}
	}
	else	// (Power-on charging)
#endif /* LPM_MODE */
	{
		if (temp_adc >= TEMP_HIGH_BLOCK) {
			if (health != POWER_SUPPLY_HEALTH_OVERHEAT &&
					health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
//				s3c_temp_control(POWER_SUPPLY_HEALTH_OVERHEAT);
				health_new = POWER_SUPPLY_HEALTH_OVERHEAT;	// hanapark_DF21
				printk("%s: temp code = %d, overheat (block) \n", __func__, temp_adc);
			}
		} else if (temp_adc <= TEMP_HIGH_RECOVER &&
				temp_adc >= TEMP_LOW_RECOVER) {
			if (health == POWER_SUPPLY_HEALTH_OVERHEAT ||
					health == POWER_SUPPLY_HEALTH_FREEZE) {
//				s3c_temp_control(POWER_SUPPLY_HEALTH_GOOD);
				health_new = POWER_SUPPLY_HEALTH_GOOD;	// hanapark_DF21
				printk("%s: temp code = %d, good (recover) \n", __func__, temp_adc);
			}
		} else if (temp_adc <= TEMP_LOW_BLOCK) {
			if (health != POWER_SUPPLY_HEALTH_FREEZE &&
					health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
//				s3c_temp_control(POWER_SUPPLY_HEALTH_FREEZE);
				health_new = POWER_SUPPLY_HEALTH_FREEZE;	// hanapark_DF21
				printk("%s: temp code = %d, freeze (block) \n", __func__, temp_adc);
			}
		}
	}

	if (/*health != POWER_SUPPLY_HEALTH_OVERHEAT &&
		health != POWER_SUPPLY_HEALTH_FREEZE &&
		health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE &&*/	// hanapark_DF21
		//(s3c_bat_info.bat_info.charging_source == CHARGER_BATTERY) || 	// No charger... force good
		(health == POWER_SUPPLY_HEALTH_GOOD &&	// not yet blocked... (meet block temperature during execution!)
		s3c_bat_info.device_state != 0x0) )
	{
		// ignore overheat !
		health_new = POWER_SUPPLY_HEALTH_GOOD;
//		s3c_temp_control(POWER_SUPPLY_HEALTH_GOOD);
//		printk("Ignore overheat/freeze (force good)! (device_state = 0x%x)\n", s3c_bat_info.device_state);
//		goto __read_temp_complete__;
	}

	if (health != health_new)
	{
		printk("battery health changed! (%d -> %d)\n", health, health_new);
		s3c_temp_control(health_new);	// hanapark_DF21
	}

__map_temperature__:	
	array_size = ARRAY_SIZE(temper_table);
	for (i = 0; i < (array_size - 1); i++) {
		if (i == 0) {
			if (temp_adc <= temper_table[0][0]) {
				temp = temper_table[0][1];
				break;
			} else if (temp_adc >= temper_table[array_size-1][0]) {
				temp = temper_table[array_size-1][1];
				break;
			}
		}

		if (temper_table[i][0] < temp_adc &&
				temper_table[i+1][0] >= temp_adc) {
			temp = temper_table[i+1][1];
		}
	}
	dev_dbg(dev, "%s: temp = %d, adc = %d\n",
			__func__, temp, temp_adc);

#ifdef __TEST_MODE_INTERFACE__
       	s3c_bat_info.bat_info.batt_temp_aver = temp;
#endif /* __TEST_MODE_INTERFACE__ */

	return temp;
}

static int s3c_bat_get_charging_status(void)
{
	charger_type_t charger = CHARGER_BATTERY; 
	int ret = 0;

	charger = s3c_bat_info.bat_info.charging_source;

	switch (charger) {
		case CHARGER_BATTERY:
			ret = POWER_SUPPLY_STATUS_NOT_CHARGING;
			break;
		case CHARGER_USB:
			if(s3c_bat_info.bat_info.batt_slate_mode){
			charger = CHARGER_BATTERY;
			ret = POWER_SUPPLY_STATUS_NOT_CHARGING;
			break;
		}
		case CHARGER_AC:
			if (s3c_bat_info.bat_info.batt_is_full)
				ret = POWER_SUPPLY_STATUS_FULL;
			else
				ret = POWER_SUPPLY_STATUS_CHARGING;
			break;
		case CHARGER_DISCHARGE:
			ret = POWER_SUPPLY_STATUS_DISCHARGING;
			break;
		default:
			ret = POWER_SUPPLY_STATUS_UNKNOWN;
	}

	dev_dbg(dev, "%s: %s\n", __func__, status_text[ret]);
	return ret;
}

static int s3c_bat_get_property(struct power_supply *bat_ps, 
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	dev_dbg(bat_ps->dev, "%s: psp = %d\n", __func__, psp);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = s3c_bat_get_charging_status();
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = s3c_get_bat_health();
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = s3c_bat_info.present;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
#ifdef __SOC_TEST__
		val->intval = soc_test;
#else
		val->intval = s3c_bat_info.bat_info.level;
#endif
		dev_dbg(dev, "%s: level = %d\n", __func__, 
				val->intval);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = s3c_bat_info.bat_info.batt_temp;
		dev_dbg(bat_ps->dev, "%s: temp = %d\n", __func__, 
				val->intval);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int s3c_power_get_property(struct power_supply *bat_ps, 
		enum power_supply_property psp, 
		union power_supply_propval *val)
{
       charger_type_t charger = CHARGER_BATTERY;
       charger = s3c_bat_info.bat_info.charging_source;
	switch (psp)
	{
		case POWER_SUPPLY_PROP_ONLINE:
			if (bat_ps->type == POWER_SUPPLY_TYPE_MAINS)
			{
				val->intval = (curent_device_type == PM_CHARGER_TA ? 1 : 0);
			}
			else if (bat_ps->type == POWER_SUPPLY_TYPE_USB){
			if(s3c_bat_info.bat_info.batt_slate_mode){
			    charger = CHARGER_BATTERY;			
	                }
			val->intval = (charger == CHARGER_USB ? 1 : 0);
			}
			else
			{
				val->intval = 0;
			}
			break;
		default:
			return -EINVAL;
	}
	
	return 0;
}

#define SEC_BATTERY_ATTR(_name)								\
{											\
        .attr = { .name = #_name, .mode = S_IRUGO | S_IWUGO, .owner = THIS_MODULE },	\
        .show = s3c_bat_show_property,							\
        .store = s3c_bat_store,								\
}

//chk: vict froyo
static struct device_attribute s3c_battery_attrs[] = {
        SEC_BATTERY_ATTR(batt_vol),
        SEC_BATTERY_ATTR(batt_vol_adc),
        SEC_BATTERY_ATTR(batt_vol_adc_cal),
        SEC_BATTERY_ATTR(batt_temp),
        SEC_BATTERY_ATTR(batt_temp_adc),
        SEC_BATTERY_ATTR(batt_temp_adc_cal),
	SEC_BATTERY_ATTR(batt_vol_adc_aver),
#ifdef __TEST_MODE_INTERFACE__
	/* test mode */
	SEC_BATTERY_ATTR(batt_test_mode),
	/* average */
	SEC_BATTERY_ATTR(batt_vol_aver),
	SEC_BATTERY_ATTR(batt_temp_aver),
	SEC_BATTERY_ATTR(batt_temp_adc_aver),
	SEC_BATTERY_ATTR(batt_v_f_adc),
#endif /* __TEST_MODE_INTERFACE__ */
#ifdef __CHECK_CHG_CURRENT__
	SEC_BATTERY_ATTR(batt_chg_current),
	SEC_BATTERY_ATTR(batt_chg_current_aver),
#endif /* __CHECK_CHG_CURRENT__ */
	SEC_BATTERY_ATTR(charging_source),
#ifdef __FUEL_GAUGES_IC__
	SEC_BATTERY_ATTR(fg_soc),
	SEC_BATTERY_ATTR(reset_soc),
	SEC_BATTERY_ATTR(full_notify),
	SEC_BATTERY_ATTR(fg_point_level),	// lobat pwroff
#endif /* __FUEL_GAUGES_IC__ */
#ifdef LPM_MODE
	SEC_BATTERY_ATTR(charging_mode_booting),
	SEC_BATTERY_ATTR(batt_temp_check),
	SEC_BATTERY_ATTR(batt_full_check),
#endif
	SEC_BATTERY_ATTR(hw_version_adc),	// hanapark_Victory
#ifdef __TEMPERATURE_TEST__
	SEC_BATTERY_ATTR(batt_temp_test),
#endif
#ifdef __MANUAL_TEMP_TEST__
	SEC_BATTERY_ATTR(manual_temp_adc),
#endif
#ifdef __SOC_TEST__
	SEC_BATTERY_ATTR(soc_test),
#endif
#ifdef __TEMP_BLOCK_EXCEPTION__
	SEC_BATTERY_ATTR(chargingblock_clear),	// hanapark_Atlas (from Max)
	SEC_BATTERY_ATTR(talk_wcdma),	// 3G voice call
	SEC_BATTERY_ATTR(data_call),	// Data call
#endif /* __TEMP_BLOCK_EXCEPTION__ */
 SEC_BATTERY_ATTR(batt_slate_mode),
};


enum {
        BATT_VOL = 0,
        BATT_VOL_ADC,
        BATT_VOL_ADC_CAL,
        BATT_TEMP,
        BATT_TEMP_ADC,
        BATT_TEMP_ADC_CAL,
	BATT_VOL_ADC_AVER,
#ifdef __TEST_MODE_INTERFACE__
	BATT_TEST_MODE,
	BATT_VOL_AVER,
	BATT_TEMP_AVER,
	BATT_TEMP_ADC_AVER,
	BATT_V_F_ADC,
#endif /* __TEST_MODE_INTERFACE__ */
#ifdef __CHECK_CHG_CURRENT__
	BATT_CHG_CURRENT,	
	BATT_CHG_CURRENT_AVER,
#endif /* __CHECK_CHG_CURRENT__ */
	BATT_CHARGING_SOURCE,
#ifdef __FUEL_GAUGES_IC__
	BATT_FG_SOC,
	BATT_RESET_SOC,
	BATT_FULL_NOTIFY,
	BATT_DECIMAL_POINT_LEVEL,	// lobat pwroff
#endif /* __FUEL_GAUGES_IC__ */
#ifdef LPM_MODE
	CHARGING_MODE_BOOTING,
	BATT_TEMP_CHECK,
	BATT_FULL_CHECK,
#endif
	HW_VERSION_ADC,	// hanapark_Victory
#ifdef __TEMPERATURE_TEST__
	BATT_TEMP_TEST,
#endif
#ifdef __MANUAL_TEMP_TEST__
	MANUAL_TEMP_ADC,
#endif
#ifdef __SOC_TEST__
	SOC_TEST,
#endif
#ifdef __TEMP_BLOCK_EXCEPTION__
	BATT_CHARGINGBLOCK_CLEAR,	// hanapark_Atlas (from Max)
	TALK_WCDMA,	// 3G voice call
	DATA_CALL,	// Data call
#endif /* __TEMP_BLOCK_EXCEPTION__ */
       BATT_SLATE_MODE,
};

static int s3c_bat_create_attrs(struct device * dev)
{
        int i, rc;
        
        for (i = 0; i < ARRAY_SIZE(s3c_battery_attrs); i++) {
                rc = device_create_file(dev, &s3c_battery_attrs[i]);
                if (rc)
                        goto s3c_attrs_failed;
        }
        goto succeed;
        
s3c_attrs_failed:
        while (i--)
                device_remove_file(dev, &s3c_battery_attrs[i]);
succeed:        
        return rc;
}

static ssize_t s3c_bat_show_property(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf)
{
        int i = 0;
        const ptrdiff_t off = attr - s3c_battery_attrs;

        switch (off) {
        case BATT_VOL:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_vol);
                break;
        case BATT_VOL_ADC:
#ifndef __FUEL_GAUGES_IC__
		s3c_bat_info.bat_info.batt_vol_adc = 
			s3c_bat_get_adc_data(S3C_ADC_VOLTAGE);
#else
		s3c_bat_info.bat_info.batt_vol_adc = 0;
#endif /* __FUEL_GAUGES_IC__ */
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_vol_adc);
                break;
        case BATT_VOL_ADC_CAL:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_vol_adc_cal);
                break;
        case BATT_TEMP:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_temp);
                break;
        case BATT_TEMP_ADC:
		s3c_bat_info.bat_info.batt_temp_adc = 
			s3c_bat_get_adc_data(S3C_ADC_TEMPERATURE);
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_temp_adc);
                break;	
#ifdef __TEST_MODE_INTERFACE__
	case BATT_TEST_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
		s3c_bat_info.bat_info.batt_test_mode);
		break;
#endif /* __TEST_MODE_INTERFACE__ */
        case BATT_TEMP_ADC_CAL:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_temp_adc_cal);
                break;
        case BATT_VOL_ADC_AVER:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_vol_adc_aver);
		break;
#ifdef __TEST_MODE_INTERFACE__
	case BATT_VOL_AVER:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_vol_aver);
		break;
	case BATT_TEMP_AVER:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_temp_aver);
		break;
	case BATT_TEMP_ADC_AVER:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_temp_adc_aver);
		break;
	case BATT_V_F_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_v_f_adc);
		break;
#endif /* __TEST_MODE_INTERFACE__ */
#ifdef __CHECK_CHG_CURRENT__
	case BATT_CHG_CURRENT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				s3c_bat_get_adc_data(S3C_ADC_CHG_CURRENT));
		break;
	case BATT_CHG_CURRENT_AVER:	// hanapark_Victory (BatteryStatus.java)
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				s3c_bat_info.bat_info.batt_current);
		break;
#endif /* __CHECK_CHG_CURRENT__ */
	case BATT_CHARGING_SOURCE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.charging_source);
		break;
#ifdef __FUEL_GAUGES_IC__
	case BATT_FG_SOC:
		if (s3c_bat_info.bat_info.decimal_point_level == 0)	// lobat pwroff
		{
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				s3c_bat_info.bat_info.level);	// maybe 0
		}
		else
		{
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			fg_read_soc());
		}
		break;
	case BATT_FULL_NOTIFY:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_is_full);
		break;	
	case BATT_DECIMAL_POINT_LEVEL:	// lobat pwroff
	   i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
		 	s3c_bat_info.bat_info.decimal_point_level);
		break;	
#endif /* __FUEL_GAUGES_IC__ */
#ifdef LPM_MODE
	case CHARGING_MODE_BOOTING:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			charging_mode_get());
		break;		
	case BATT_TEMP_CHECK:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_health);
		break;		
	case BATT_FULL_CHECK:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_is_full );
		break;			
#endif
	case HW_VERSION_ADC:	// hanapark_Victory
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_get_adc_data(S3C_ADC_HW_VERSION));
		break;
#ifdef __TEMPERATURE_TEST__
	case BATT_TEMP_TEST:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			batt_temp_test_mode);
		break;
#endif
#ifdef __MANUAL_TEMP_TEST__
	case MANUAL_TEMP_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			manual_temp_adc);
		break;
#endif
#ifdef __SOC_TEST__
	case SOC_TEST:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			soc_test);
		break;
#endif
#ifdef __TEMP_BLOCK_EXCEPTION__
	case BATT_CHARGINGBLOCK_CLEAR:	// hanapark_Atlas (from Max)
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.device_state);	// s3c_bat_info.bat_info.batt_chargingblock_clear
		break;
	case TALK_WCDMA:	// 3G voice call
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(s3c_bat_info.device_state & USE_TALK_WCDMA));
		break;
	case DATA_CALL: // Data call
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(s3c_bat_info.device_state & USE_DATA_CALL));
		break;
#endif /* __TEMP_BLOCK_EXCEPTION__ */
        case BATT_SLATE_MODE:
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_slate_mode);
			break;
        default:
                i = -EINVAL;
        }       
        
        return i;
}

static void s3c_bat_set_vol_cal(int batt_cal)
{
	int max_cal = 4096;

	if (!batt_cal)
		return;

	if (batt_cal >= max_cal) {
		dev_err(dev, "%s: invalid battery_cal(%d)\n", __func__, batt_cal);
		return;
	}

	batt_max = batt_cal + BATT_MAXIMUM;
	batt_full = batt_cal + BATT_FULL;
	batt_safe_rech = batt_cal + BATT_SAFE_RECHARGE;
	batt_almost = batt_cal + BATT_ALMOST_FULL;
	batt_high = batt_cal + BATT_HIGH;
	batt_medium = batt_cal + BATT_MED;
	batt_low = batt_cal + BATT_LOW;
	batt_critical = batt_cal + BATT_CRITICAL;
	batt_min = batt_cal + BATT_MINIMUM;
	batt_off = batt_cal + BATT_OFF;
}

static ssize_t s3c_bat_store(struct device *dev, 
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int x = 0;
	int ret = 0;
	const ptrdiff_t off = attr - s3c_battery_attrs;

        switch (off) {
        case BATT_VOL_ADC_CAL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_info.bat_info.batt_vol_adc_cal = x;
			s3c_bat_set_vol_cal(x);
			ret = count;
		}
		dev_info(dev, "%s: batt_vol_adc_cal = %d\n", __func__, x);
                break;
        case BATT_TEMP_ADC_CAL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_info.bat_info.batt_temp_adc_cal = x;
			ret = count;
		}
		dev_info(dev, "%s: batt_temp_adc_cal = %d\n", __func__, x);
                break;
#ifdef __TEST_MODE_INTERFACE__
	case BATT_TEST_MODE:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_info.bat_info.batt_test_mode = x;
			ret = count;
		}
		if (s3c_bat_info.bat_info.batt_test_mode) {
			s3c_bat_info.polling_interval = POLLING_INTERVAL_TEST;			
			if (s3c_bat_info.polling) {
				del_timer_sync(&polling_timer);
			}
			mod_timer(&polling_timer, jiffies + 
				msecs_to_jiffies(s3c_bat_info.polling_interval));
			s3c_bat_status_update(
				&s3c_power_supplies_test[CHARGER_BATTERY]);
		} else {
			s3c_bat_info.polling_interval = POLLING_INTERVAL;		
			if (s3c_bat_info.polling) {
				del_timer_sync(&polling_timer);
			}
			mod_timer(&polling_timer,jiffies + 
				msecs_to_jiffies(s3c_bat_info.polling_interval));
			s3c_bat_status_update(
				&s3c_power_supplies_test[CHARGER_BATTERY]);
		}
		dev_info(dev, "%s: batt_test_mode = %d\n", __func__, x);
		break;
#endif /* __TEST_MODE_INTERFACE__ */
#ifdef __FUEL_GAUGES_IC__
	case BATT_RESET_SOC:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x == 1)
				fg_reset_soc();
			ret = count;
		}
		dev_info(dev, "%s: Reset SOC:%d\n", __func__, x);
		break;
#endif /* __FUEL_GAUGES_IC__ */
#ifdef LPM_MODE
	case CHARGING_MODE_BOOTING:
		if (sscanf(buf, "%d\n", &x) == 1) {
			charging_mode_set(x);
			ret = count;
		}
		dev_info(dev, "%s: CHARGING_MODE_BOOTING:%d\n", __func__, x);
		break;		
#endif
#ifdef __TEMPERATURE_TEST__
	case BATT_TEMP_TEST:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x > 2)
				x = 2;
			else if (x < 0)
				x = 0;
			batt_temp_test_mode = x;
			if (x != 0)	// not auto test
				start_time_test = 0;	// initialize start time
			ret = count;
		}
		break;
#endif
#ifdef __MANUAL_TEMP_TEST__
	case MANUAL_TEMP_ADC:
		if (sscanf(buf, "%d\n", &x) == 1) {
			manual_temp_adc = x;
			force_update = 1;
			ret = count;
		}
		break;
#endif
#ifdef __SOC_TEST__
	case SOC_TEST:
		if (sscanf(buf, "%d\n", &x) == 1) {
			soc_test = x;
			force_update = 1;
			ret = count;
		}
		break;
#endif
#ifdef __TEMP_BLOCK_EXCEPTION__
	case BATT_CHARGINGBLOCK_CLEAR:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if ((s3c_bat_info.device_state & USE_BROWSER) && !(x & USE_BROWSER))
			{
				s3c_bat_use_browser(0);	// browser off
			}
			else if (!(s3c_bat_info.device_state & USE_BROWSER) && (x & USE_BROWSER))
			{
				s3c_bat_use_browser(1);	// browser on
			}
			else
			{
				s3c_bat_info.device_state = x;	//s3c_bat_info.bat_info.batt_chargingblock_clear= x;
			}
			ret = count;
		}
		break;
#endif /* __TEMP_BLOCK_EXCEPTION__ */
	case TALK_WCDMA:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x == 1)
				s3c_bat_info.device_state = s3c_bat_info.device_state | USE_TALK_WCDMA;
			else
				s3c_bat_info.device_state = s3c_bat_info.device_state & (~USE_TALK_WCDMA);
			ret = count;
		}
		break;
	case DATA_CALL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_use_data_call(x);
			ret = count;
		}
		break;
      	case BATT_SLATE_MODE:
			if (sscanf(buf, "%d\n", &x) == 1) {
				s3c_bat_info.bat_info.batt_slate_mode = x;
				ret = count;
			}
				dev_info(dev, "%s: batt_slate_mode = %d\n", __func__, x);
			break;

        default:
                ret = -EINVAL;
        }       

	return ret;
}

static enum power_supply_property s3c_battery_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
};

static enum power_supply_property s3c_power_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *supply_list[] = {
	"battery",
};

static struct power_supply s3c_power_supplies[] = {
	{
		.name = "battery",
		.type = POWER_SUPPLY_TYPE_BATTERY,
		.properties = s3c_battery_properties,
		.num_properties = ARRAY_SIZE(s3c_battery_properties),
		.get_property = s3c_bat_get_property,
	},
	{
		.name = "usb",
		.type = POWER_SUPPLY_TYPE_USB,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = s3c_power_properties,
		.num_properties = ARRAY_SIZE(s3c_power_properties),
		.get_property = s3c_power_get_property,
	},
	{
		.name = "ac",
		.type = POWER_SUPPLY_TYPE_MAINS,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = s3c_power_properties,
		.num_properties = ARRAY_SIZE(s3c_power_properties),
		.get_property = s3c_power_get_property,
	},
};

static int s3c_cable_status_update(int status)
{
	int ret = 0;
	charger_type_t source = CHARGER_BATTERY;

	printk("[BATT] %s \n", __func__);

	if(!s3c_battery_initial)
		return -EPERM;

	switch(status) {
	case CHARGER_BATTERY:
		printk("%s: cable NOT PRESENT\n", __func__);
		s3c_bat_info.bat_info.charging_source = CHARGER_BATTERY;
		break;
	case CHARGER_USB:
		printk("%s: cable USB\n", __func__);
		if(s3c_bat_info.bat_info.batt_slate_mode){
			s3c_bat_info.bat_info.charging_source = CHARGER_BATTERY;			
		}else{
			s3c_bat_info.bat_info.charging_source = CHARGER_USB;
		}		
		break;
	case CHARGER_AC:
		printk("%s: cable AC\n", __func__);
		s3c_bat_info.bat_info.charging_source = CHARGER_AC;
		break;
	case CHARGER_DISCHARGE:
		printk("%s: Discharge\n", __func__);
		s3c_bat_info.bat_info.charging_source = CHARGER_DISCHARGE;
		break;
	default:
		printk("%s: Not supported status\n", __func__);
		ret = -EINVAL;
	}
	source = s3c_bat_info.bat_info.charging_source;

//			if (source == CHARGER_USB || source == CHARGER_AC || source == CHARGER_DISCHARGE) {
				if (curent_device_type != PM_CHARGER_NULL) {	// hanapark_DF21 (from S1)
                wake_lock(&vbus_wake_lock);
        } else {
                /* give userspace some time to see the uevent and update
                 * LED state or whatnot...
                 */
		if(!maxim_chg_status())
			wake_lock_timeout(&vbus_wake_lock, HZ * 5);
        }
        /* if the power source changes, all power supplies may change state */
        power_supply_changed(&s3c_power_supplies[CHARGER_BATTERY]);
	printk("%s: call power_supply_changed\n", __func__);
	return ret;
}

static void s3c_bat_status_update(struct power_supply *bat_ps)
{
	int old_level, old_temp, old_is_full;
	dev_dbg(dev, "%s ++\n", __func__);

	if(!s3c_battery_initial)
		return;

	mutex_lock(&work_lock);
	old_temp = s3c_bat_info.bat_info.batt_temp;
	old_level = s3c_bat_info.bat_info.level; 
	old_is_full = s3c_bat_info.bat_info.batt_is_full;
	s3c_bat_info.bat_info.batt_temp = s3c_get_bat_temp(bat_ps);

	s3c_bat_info.bat_info.level = s3c_get_bat_level(bat_ps);
	if (!s3c_bat_info.bat_info.charging_enabled &&
			!s3c_bat_info.bat_info.batt_is_full) {
		if (s3c_bat_info.bat_info.level > old_level)
			s3c_bat_info.bat_info.level = old_level;
	}
	s3c_bat_info.bat_info.batt_vol = s3c_get_bat_vol(bat_ps);

	if (s3c_bat_check_v_f() == 0)
		charging_stop_without_magic_number();	// hanapark_DF01

	if (old_level != s3c_bat_info.bat_info.level 
			|| old_temp != s3c_bat_info.bat_info.batt_temp
			|| old_is_full != s3c_bat_info.bat_info.batt_is_full
			|| force_update) {
		force_update = 0;
		power_supply_changed(bat_ps);
		dev_dbg(dev, "%s: call power_supply_changed\n", __func__);
	}

	mutex_unlock(&work_lock);
	dev_dbg(dev, "%s --\n", __func__);
}

static unsigned int s3c_bat_check_v_f(void)
{
	unsigned int rc = 0;
	int adc = 0;
	static int jig_status = 0;

	if (FSA9480_Get_JIG_OnOff_Status() == 1)
	{
		if (jig_status == 0)
			printk("%s: JIG cable inserted \n", __func__);
		jig_status = 1;
		s3c_set_bat_health(POWER_SUPPLY_HEALTH_GOOD);
		return 1;
	}
	else if (jig_status == 1)
	{
		jig_status = 0;
		printk("%s: JIG cable removed \n", __func__);
	}
	
	adc = s3c_bat_get_adc_data(S3C_ADC_V_F);
	s3c_bat_info.bat_info.batt_v_f_adc = adc;

//	dev_info(dev, "%s: V_F ADC = %d\n", __func__, adc);

	if (adc <= BATT_VF_MAX && adc >= BATT_VF_MIN) {
		 //s3c_set_bat_health(POWER_SUPPLY_HEALTH_GOOD);
		rc = 1;
	} else {
		dev_info(dev, "%s: Unauthorized battery!\n", __func__);
		s3c_set_bat_health(POWER_SUPPLY_HEALTH_UNSPEC_FAILURE);
		rc = 0;
	}

	return rc;
}

static void s3c_cable_check_status(void)
{
	charger_type_t status = 0;
	printk("[BATT] %s \n", __func__);

	mutex_lock(&work_lock);

	if (maxim_chg_status()) {		
		if (s3c_get_bat_health() != POWER_SUPPLY_HEALTH_GOOD) {
			printk("%s: Unhealth battery state! (%d) \n", __func__, s3c_get_bat_health());
			status = CHARGER_DISCHARGE;
			s3c_set_chg_en(0);
			goto __end__;
		}

		if (get_usb_power_state())
			status = CHARGER_USB;
		else
			status = CHARGER_AC;
                if((status == CHARGER_USB)&&(s3c_bat_info.bat_info.batt_slate_mode)){
			        status = CHARGER_BATTERY;
			        s3c_set_chg_en(0);					
		        } else
			        s3c_set_chg_en(1);	
		s3c_bat_info.bat_info.decimal_point_level = 1;	// lobat pwroff
#ifdef CONFIG_KERNEL_DEBUG_SEC		
		kernel_sec_set_upload_magic_number();
#endif

		printk("%s: status : %s\n", __func__, 
				(status == CHARGER_USB) ? "USB" : "AC");
	} else {
		status = CHARGER_BATTERY;
		s3c_set_chg_en(0);
		printk("%s: No charger!\n", __func__);
	}
__end__:
	s3c_cable_status_update(status);
	mutex_unlock(&work_lock);
}

int s3c_bat_is_in_call(void)	// hanapark_DF25
{
	if (s3c_bat_info.device_state & 0x01)
		return 1;	// in call
	else
		return 0;
}

void low_battery_power_off(void)	// lobat pwroff (sleep interrupt)
{
	if (s3c_bat_info.bat_info.decimal_point_level == 0)
		return ;	// already requested...

	s3c_bat_info.bat_info.level = 0;
	s3c_bat_info.bat_info.decimal_point_level = 0;
#ifdef CONFIG_KERNEL_DEBUG_SEC		
	kernel_sec_clear_upload_magic_number();
#endif

	force_update = 1;
	schedule_work(&bat_work);
	mod_timer(&polling_timer, jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
}

static void low_battery_power_off_polling(void)	// lobat pwroff
{
	if (s3c_bat_info.bat_info.decimal_point_level == 0)
		return ;	// already requested...

	s3c_bat_info.bat_info.level = 0;
	s3c_bat_info.bat_info.decimal_point_level = 0;
#ifdef CONFIG_KERNEL_DEBUG_SEC		
	kernel_sec_clear_upload_magic_number();
#endif

	wake_lock_timeout(&low_batt_wake_lock, HZ * LOW_BATT_COUNT);

	force_update = 1;
	schedule_work(&bat_work);
	mod_timer(&polling_timer, jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
}

static void s3c_check_low_batt(void)	// lobat pwroff
{
	static int cnt = 0;
	int low_batt_vol = LOW_BATT_COND_VOLTAGE;	// 3400mV

	if (s3c_bat_info.bat_info.decimal_point_level == 0)
		return ;	// already requested...

	if (s3c_bat_info.device_state & 0x1)	// compensation for voice call
	{
		low_batt_vol -= 100;	// 3300mV
	}

	if (/*(!maxim_chg_status()) &&*/ (s3c_bat_info.bat_info.level <= LOW_BATT_COND_LEVEL) &&
		(s3c_bat_info.bat_info.batt_vol /*+ 35*/ <= low_batt_vol))
	{
#ifdef __TEST_MODE_INTERFACE__
		if (s3c_bat_info.bat_info.batt_test_mode == 1)	// test mode (interval 1 sec)
			cnt++;
		else	// non-test mode (interval 2 sec)
#endif
			cnt += 2;
		printk(KERN_EMERG "%s: low battery power-off cnt = %d\n", __func__, cnt);

		if (cnt > LOW_BATT_COUNT)
		{
			low_battery_power_off_polling();
			cnt = 0;
		}
	}
	else
	{
		cnt = 0;
//		printk(KERN_EMERG "%s: cnt = %d (initialized)\n", __func__, cnt);
	}
}

static void s3c_bat_work(struct work_struct *work)
{
	dev_dbg(dev, "%s\n", __func__);

	s3c_bat_status_update(
			&s3c_power_supplies[CHARGER_BATTERY]);

	if (!charging_mode_get())	// except LPM mode
		s3c_check_low_batt(); // lobat pwroff
}

static void s3c_cable_work(struct work_struct *work)
{
	dev_dbg(dev, "%s\n", __func__);
	s3c_cable_check_status();
}

#ifdef CONFIG_PM
static int s3c_bat_suspend(struct platform_device *pdev, 
		pm_message_t state)
{
	dev_info(dev, "%s\n", __func__);

	set_low_bat_interrupt(1);	// lobat pwroff

	if (s3c_bat_info.polling)
		del_timer_sync(&polling_timer);

	flush_scheduled_work();
	return 0;
}

static int s3c_bat_resume(struct platform_device *pdev)
{
	dev_info(dev, "%s\n", __func__);
//	wake_lock(&vbus_wake_lock);	// hanapark_DF21 (from S1)
	
	set_low_bat_interrupt(0);	// lobat pwroff

	schedule_work(&bat_work);
//	schedule_work(&cable_work);

	if (s3c_bat_info.polling)
		mod_timer(&polling_timer,
			  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));

	return 0;
}
#else
#define s3c_bat_suspend NULL
#define s3c_bat_resume NULL
#endif /* CONFIG_PM */

static void polling_timer_func(unsigned long unused)
{
	pr_debug("s3c_battery : %s\n", __func__);
	schedule_work(&bat_work);

	mod_timer(&polling_timer,
		  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
}

void s3c_cable_changed(void)
{
//	printk("[BATT] %s \n", __func__);

	if (!s3c_battery_initial)
		return ;

	s3c_bat_info.bat_info.batt_is_full = 0;

	schedule_work(&cable_work);
	/*
	 * Wait a bit before reading ac/usb line status and setting charger,
	 * because ac/usb status readings may lag from irq.
	 */
	mod_timer(&polling_timer,
		  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
}

void s3c_cable_charging(void)
{
#if 0
	//int chg_ing = gpio_get_value(gpio_chg_ing);
	//dev_info(dev, "%s: irq=0x%x, gpio_chg_ing=%d\n", __func__, irq, chg_ing);

	if (!s3c_battery_initial)
		return IRQ_HANDLED;
#ifndef __DISABLE_CHG_ING_INTR__
/*
	if (chg_ing && !gpio_get_value(gpio_ta_connected) &&
			s3c_bat_info.bat_info.charging_enabled &&
			s3c_get_bat_health() == POWER_SUPPLY_HEALTH_GOOD) {
*/
	if (s3c_bat_info.bat_info.charging_enabled &&
			s3c_get_bat_health() == POWER_SUPPLY_HEALTH_GOOD) {
		s3c_set_chg_en(0);
		s3c_bat_info.bat_info.batt_is_full = 1;
		force_update = 1;
	}

	schedule_work(&bat_work);
	/*
	 * Wait a bit before reading ac/usb line status and setting charger,
	 * because ac/usb status readings may lag from irq.
	 */
	mod_timer(&polling_timer,
		  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
#endif /* __DISABLE_CHG_ING_INTR__ */
#endif
}

static void battery_early_suspend(struct early_suspend *h)
{
	u32 con;

	/*hsmmc clock disable*/
	con = readl(S5P_CLKGATE_IP2);
	con &= ~(S5P_CLKGATE_IP2_HSMMC3|S5P_CLKGATE_IP2_HSMMC2|S5P_CLKGATE_IP2_HSMMC1 \
		|S5P_CLKGATE_IP2_HSMMC0);
	writel(con, S5P_CLKGATE_IP2);

	/*g3d clock disable*/
	con = readl(S5P_CLKGATE_IP0);
	con &= ~S5P_CLKGATE_IP0_G3D;
	writel(con, S5P_CLKGATE_IP0);

	/*power gating*/
	con = readl(S5P_NORMAL_CFG);
	con &= ~(S5PC110_POWER_DOMAIN_G3D|S5PC110_POWER_DOMAIN_MFC|S5PC110_POWER_DOMAIN_TV \
		|S5PC110_POWER_DOMAIN_CAM|S5PC110_POWER_DOMAIN_AUDIO);
	writel(con , S5P_NORMAL_CFG);

	/*usb clock disable*/
//	s3c_usb_cable(0);
}

static void battery_late_resume(struct early_suspend *h)
{
	// do nothing!
}


static int __devinit s3c_bat_probe(struct platform_device *pdev)
{
	int i;
	int ret = 0;

	dev = &pdev->dev;
	dev_info(dev, "%s\n", __func__);

	s3c_bat_info.present = 1;
	s3c_bat_info.polling = 1;
	s3c_bat_info.polling_interval = POLLING_INTERVAL;
	s3c_bat_info.device_state = 0;

	s3c_bat_info.bat_info.batt_vol_adc_aver = 0;
#ifdef __TEST_MODE_INTERFACE__
	s3c_bat_info.bat_info.batt_vol_aver = 0;
	s3c_bat_info.bat_info.batt_temp_aver = 0;
	s3c_bat_info.bat_info.batt_temp_adc_aver = 0;
	s3c_bat_info.bat_info.batt_v_f_adc = 0;

	s3c_bat_info.bat_info.batt_test_mode = 0;
 	s3c_power_supplies_test = s3c_power_supplies;
#endif /* __TEST_MODE_INTERFACE__ */
	s3c_bat_info.bat_info.batt_id = 0;
	s3c_bat_info.bat_info.batt_vol = 0;
	s3c_bat_info.bat_info.batt_vol_adc = 0;
	s3c_bat_info.bat_info.batt_vol_adc_cal = 0;
	s3c_bat_info.bat_info.batt_temp = 0;
	s3c_bat_info.bat_info.batt_temp_adc = 0;
	s3c_bat_info.bat_info.batt_temp_adc_cal = 0;
	s3c_bat_info.bat_info.batt_current = 0;
	s3c_bat_info.bat_info.level = 100;
	s3c_bat_info.bat_info.charging_source = CHARGER_BATTERY;
	s3c_bat_info.bat_info.charging_enabled = 0;
	s3c_bat_info.bat_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;

#ifdef __FUEL_GAUGES_IC__
	s3c_bat_info.bat_info.decimal_point_level = 1;	// lobat pwroff
#endif
        s3c_bat_info.bat_info.batt_slate_mode = 0;
	memset(adc_sample, 0x00, sizeof adc_sample);

	batt_max = BATT_CAL + BATT_MAXIMUM;
	batt_full = BATT_CAL + BATT_FULL;
	batt_safe_rech = BATT_CAL + BATT_SAFE_RECHARGE;
	batt_almost = BATT_CAL + BATT_ALMOST_FULL;
	batt_high = BATT_CAL + BATT_HIGH;
	batt_medium = BATT_CAL + BATT_MED;
	batt_low = BATT_CAL + BATT_LOW;
	batt_critical = BATT_CAL + BATT_CRITICAL;
	batt_min = BATT_CAL + BATT_MINIMUM;
	batt_off = BATT_CAL + BATT_OFF;

	INIT_WORK(&bat_work, s3c_bat_work);
	INIT_WORK(&cable_work, s3c_cable_work);

	/* init power supplier framework */
	for (i = 0; i < ARRAY_SIZE(s3c_power_supplies); i++) {
		ret = power_supply_register(&pdev->dev, 
				&s3c_power_supplies[i]);
		if (ret) {
			dev_err(dev, "Failed to register"
					"power supply %d,%d\n", i, ret);
			goto __end__;
		}
	}

	/* create sec detail attributes */
	s3c_bat_create_attrs(s3c_power_supplies[CHARGER_BATTERY].dev);

	if (s3c_bat_info.polling) {
		dev_dbg(dev, "%s: will poll for status\n", 
				__func__);
		setup_timer(&polling_timer, polling_timer_func, 0);
		mod_timer(&polling_timer,
			  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
	}

	setup_timer(&use_wimax_timer, use_wimax_timer_func, 0);	// hanapark_Victory_DF29
	setup_timer(&use_data_call_timer, use_data_call_timer_func, 0);	// hanapark_Victory_DF29
	setup_timer(&use_browser_timer, use_browser_timer_func, 0);	// hanapark_Victory_DF29

	s3c_battery_initial = 1;
	force_update = 0;

	#ifdef __FUEL_GAUGES_IC__
	fuel_gauge_rcomp();
	#endif
	
	s3c_bat_status_update(
			&s3c_power_supplies[CHARGER_BATTERY]);

		s3c_bat_check_v_f();
	s3c_cable_check_status();

	/* Request IRQ */ 
	MAX8998_IRQ_init();

	if(charging_mode_get())
	{
		battery = kzalloc(sizeof(struct battery_driver), GFP_KERNEL);
#ifdef CONFIG_HAS_EARLYSUSPEND
		battery->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
		battery->early_suspend.suspend = battery_early_suspend;
		battery->early_suspend.resume = battery_late_resume;
		register_early_suspend(&battery->early_suspend);
#endif
	}

__end__:
	return ret;
}

static int __devexit s3c_bat_remove(struct platform_device *pdev)
{
	int i;
	dev_info(dev, "%s\n", __func__);

	if (s3c_bat_info.polling)
		del_timer_sync(&polling_timer);

	for (i = 0; i < ARRAY_SIZE(s3c_power_supplies); i++) {
		power_supply_unregister(&s3c_power_supplies[i]);
	}
 
	return 0;
}

static struct platform_driver s3c_bat_driver = {
	.driver.name	= DRIVER_NAME,
	.driver.owner	= THIS_MODULE,
	.probe		= s3c_bat_probe,
	.remove		= __devexit_p(s3c_bat_remove),
	.suspend	= s3c_bat_suspend,
	.resume		= s3c_bat_resume,
};

static int __init s3c_bat_init(void)
{
	pr_info("%s\n", __func__);

	wake_lock_init(&vbus_wake_lock, WAKE_LOCK_SUSPEND, "vbus_present");
	wake_lock_init(&low_batt_wake_lock, WAKE_LOCK_SUSPEND, "low_batt_detected");	// lobat pwroff

#ifdef __FUEL_GAUGES_IC__
	fg_init();
#endif /* __FUEL_GAUGES_IC__ */
	return platform_driver_register(&s3c_bat_driver);
}

static void __exit s3c_bat_exit(void)
{
	pr_info("%s\n", __func__);
#ifdef __FUEL_GAUGES_IC__
	fg_exit();
#endif /* __FUEL_GAUGES_IC__ */
	platform_driver_unregister(&s3c_bat_driver);
}


#ifdef __MANUAL_TEMP_TEST__
#define USE_MODULE_TIMEOUT	(0*60*1000)
#else
#define USE_MODULE_TIMEOUT	(10*60*1000)	// 10 min (DG05_FINAL: 1 min -> 10 min)
#endif

static void use_wimax_timer_func(unsigned long unused)
{
	s3c_bat_info.device_state = s3c_bat_info.device_state & (~USE_WIMAX);
	printk("%s: OFF (0x%x) \n", __func__, s3c_bat_info.device_state);
}

int s3c_bat_use_wimax(int onoff)	
{
	if (onoff)
	{	
		del_timer_sync(&use_wimax_timer);
		s3c_bat_info.device_state = s3c_bat_info.device_state | USE_WIMAX;
		printk("%s: ON (0x%x) \n", __func__, s3c_bat_info.device_state);
	}
	else
	{
		mod_timer(&use_wimax_timer, jiffies + msecs_to_jiffies(USE_MODULE_TIMEOUT));
	}

	return s3c_bat_info.device_state;
}
EXPORT_SYMBOL(s3c_bat_use_wimax);

static void use_browser_timer_func(unsigned long unused)
{
	s3c_bat_info.device_state = s3c_bat_info.device_state & (~USE_BROWSER);
	printk("%s: OFF (0x%x) \n", __func__, s3c_bat_info.device_state);
}

static int s3c_bat_use_browser(int onoff)	
{
	if (onoff)
	{	
		del_timer_sync(&use_browser_timer);
		s3c_bat_info.device_state = s3c_bat_info.device_state | USE_BROWSER;
		printk("%s: ON (0x%x) \n", __func__, s3c_bat_info.device_state);
	}
	else
	{
		mod_timer(&use_browser_timer, jiffies + msecs_to_jiffies(USE_MODULE_TIMEOUT));
	}

	return s3c_bat_info.device_state;
}

static int data_call_off_request = 0;	// DG09

static void use_data_call_timer_func(unsigned long unused)
{
	s3c_bat_info.device_state = s3c_bat_info.device_state & (~USE_DATA_CALL);
	printk("%s: OFF (0x%x) \n", __func__, s3c_bat_info.device_state);
}

static int s3c_bat_use_data_call(int onoff)	
{
	if (onoff)
	{
		data_call_off_request = 0;
		del_timer_sync(&use_data_call_timer);
		s3c_bat_info.device_state = s3c_bat_info.device_state | USE_DATA_CALL;
		printk("%s: ON (0x%x) \n", __func__, s3c_bat_info.device_state);
	}
	else
	{
		if (data_call_off_request == 0)
		{
			data_call_off_request = 1;
			mod_timer(&use_data_call_timer, jiffies + msecs_to_jiffies(USE_MODULE_TIMEOUT));
			printk("%s: OFF waiting (0x%x) \n", __func__, s3c_bat_info.device_state);
		}
	}

	return s3c_bat_info.device_state;
}


late_initcall(s3c_bat_init);
module_exit(s3c_bat_exit);

MODULE_AUTHOR("Minsung Kim <ms925.kim@samsung.com>");
MODULE_DESCRIPTION("S3C6410 battery driver");
MODULE_LICENSE("GPL");
