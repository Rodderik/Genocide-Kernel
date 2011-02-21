/*
 * linux/drivers/power/s3c6410_battery.h
 *
 * Battery measurement code for S3C6410 platform.
 *
 * Copyright (C) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define DRIVER_NAME	"sec-battery"

/*
 * Spica Rev00 board Battery Table
 */
#define BATT_CAL		2447	/* 3.60V */

#define BATT_MAXIMUM		406	/* 4.176V */
#define BATT_FULL		353	/* 4.10V  */
#define BATT_SAFE_RECHARGE 353	/* 4.10V */
#define BATT_ALMOST_FULL	188 /* 3.8641V */	//322	/* 4.066V */
#define BATT_HIGH		112 /* 3.7554V */ 		//221	/* 3.919V */
#define BATT_MED		66 /* 3.6907V */ 		//146	/* 3.811V */
#define BATT_LOW		43 /* 3.6566V */		//112	/* 3.763V */
#define BATT_CRITICAL		8 /* 3.6037V */ 	//(74)	/* 3.707V */
#define BATT_MINIMUM		(-28) /* 3.554V */	//(38)	/* 3.655V */
#define BATT_OFF		(-128) /* 3.4029V */	//(-103)	/* 3.45V  */

/*
 * Aries Rev02 board Temperature Table
 */

const int temper_table[][2] =  {
	{ 107, 	-70 },
	{ 109, 	-60 },
	{ 111, 	-50 },
	{ 113, 	-40 },
	{ 115, 	-30 },
	{ 117, 	-20 },
	{ 119, 	-10 },
	{ 121, 	0 },
	{ 123, 	10 },
	{ 125, 	20 },
	{ 127, 	30 },
	{ 129, 	40 },
	{ 131, 	50 },
	{ 133, 	60 },
	{ 135, 	70 },
	{ 137, 	80 },
	{ 139, 	90 },
	{ 141, 	100 },
	{ 143, 	110 },
	{ 145, 	120 },
	{ 147, 	130 },
	{ 149, 	140 },
	{ 151, 	150 },
	{ 153, 	160 },
	{ 155, 	170 },
	{ 157, 	180 },	
	{ 159, 	190 },
	{ 161, 	200 },
	{ 163, 	210 },
	{ 165, 	220 },
	{ 167, 	230 },
	{ 169, 	240 },
	{ 171, 	250 },
	{ 173, 	260 },
	{ 175, 	270 },
	{ 177, 	280 },
	{ 179, 	290 },
	{ 181, 	300 },
	{ 183, 	310 },
	{ 185, 	320 },
	{ 187, 	330 },
	{ 189, 	340 },
	{ 191, 	350 },
	{ 193, 	360 },
	{ 195, 	370 },
	{ 197, 	380 },
	{ 199, 	390 },
	{ 201, 	400 },
	{ 203, 	410 },
	{ 205, 	420 },
	{ 207, 	430 },
	{ 209, 	440 },
	{ 211, 	450 },
	{ 213, 	460 },
	{ 215, 	470 },
	{ 217,	480 },
	{ 219,	490 },
	{ 221,	500 },
	{ 223,	510 },
	{ 225,	520 },
	{ 227,	530 },
	{ 229,	540 },
	{ 231,	550 },
	{ 233,	560 },
	{ 235,	570 },
	{ 237,	580 },
	{ 239,	590 },
	{ 241,	600 },
	{ 243,	610 },
	{ 245,	620 },
	{ 247,	630 },
	{ 249,	640 },
	{ 251,	650 },
	{ 253,	660 },
	{ 255,	670 },
};

#define TEMP_HIGH_BLOCK			215	//211 (ECIM #2857624)
#define TEMP_HIGH_RECOVER		195	//192 (ECIM #2857624)
#define TEMP_LOW_BLOCK			110	//108 (ECIM #2857624)
#define TEMP_LOW_RECOVER		118	//115 (ECIM #2857624)

#define TEMP_HIGH_BLOCK_LPM			208
#define TEMP_HIGH_RECOVER_LPM		197
#define TEMP_LOW_BLOCK_LPM			121
#define TEMP_LOW_RECOVER_LPM		122


/*
 * Aries Rev02 board ADC channel
 */
typedef enum s3c_adc_channel {
	// hanapark_Victory
	S3C_ADC_TEMPERATURE = 1,
	S3C_ADC_CHG_CURRENT = 2,
	S3C_ADC_V_F = 4,
	S3C_ADC_HW_VERSION = 7,
	S3C_ADC_VOLTAGE = 8,
	ENDOFADC
} adc_channel_type;


/******************************************************************************
 * Battery driver features
 * ***************************************************************************/

#define __ADJUST_RECHARGE_ADC__
#define __TEST_MODE_INTERFACE__
#define __FUEL_GAUGES_IC__ 
#define __CHECK_CHG_CURRENT__
#define __ADJUST_ADC_VALUE__
#define __TEMP_BLOCK_EXCEPTION__

//#define __TEMPERATURE_TEST__
//#define __SOC_TEST__
//#define __FULL_CHARGE_TEST__
//#define __MANUAL_TEMP_TEST__

/*****************************************************************************/

#ifdef __FULL_CHARGE_TEST__
#define TOTAL_CHARGING_TIME	(1*60*HZ)	/* 1 min */
#else
#define TOTAL_CHARGING_TIME	(5*60*60*HZ)	/* 5 hours */
#endif
#define TOTAL_RECHARGING_TIME	(2*60*60*HZ)	/* 2 hours */

#ifdef __ADJUST_RECHARGE_ADC__
#define BATT_RECHARGE_CODE	0	// hanapark (fix compile error)
#define BATT_RECHARGE_COUNT	20
#endif

#ifdef __FUEL_GAUGES_IC__
#define FULL_CHARGE_COND_VOLTAGE	4000
#define RECHARGE_COND_VOLTAGE		4110	// 2010.05.08.
#define RECHARGE_COND_VOLTAGE_BACKUP		4000

#define LOW_BATT_COUNT	30
#define LOW_BATT_COND_VOLTAGE		3400
#define LOW_BATT_COND_LEVEL			0
#endif /* __FUEL_GAUGES_IC__ */

#define BATT_VF_MIN	0	//12	// hanapark_Victory
#define BATT_VF_MAX	50	//30	// hanapark_Victory

#ifdef __CHECK_CHG_CURRENT__
#define CURRENT_OF_FULL_CHG  91	// 2010.05.08.
#define CHG_CURRENT_COUNT		20
#endif

