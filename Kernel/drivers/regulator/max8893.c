/*
 * max8893.c  --  Voltage and current regulation for the Maxim 8893
 *
 * Copyright (C) 2009 Samsung Electronics
 * Inchul.Im <inchul.im@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/max8893.h>
#include <linux/mutex.h>

#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/hardware.h>
#include <mach/gpio.h>

#define DBG(fmt...)
//#define DBG(fmt...) printk(fmt)

#if 1//CONFIG_ARIES_VER_B1... pfe

/* Registers */
#define MAX8893_REG_ONOFF 0x00
#define MAX8893_REG_DISCHARGE	0x01
#define MAX8893_REG_LSTIME	0x02
#define MAX8893_REG_DVSRAMP	0x03
#define MAX8893_REG_BUCK	0x04
#define MAX8893_REG_LDO1	0x05
#define MAX8893_REG_LDO2	0x06
#define MAX8893_REG_LDO3	0x07
#define MAX8893_REG_LDO4	0x08
#define MAX8893_REG_LDO5	0x09
//#define MAX8998_REG_SVER	0x46
//#define MAX8998_REG_NREV	0x47

struct max8893_data {
	struct i2c_client	*client;
	struct device		*dev;

	struct mutex		mutex;

	int			num_regulators;
	struct regulator_dev	**rdev;
};


static u8 max8893_cache_regs[] = {
	0x01, 0xFF, 0x08, 0x09, 0x02, //0x01
	0x02, 0x0E, 0x11, 0x19, 0x16, //0x05
};
/*
static u8 max8893_cache_regs[] = {
	0x01, 0xFF, 0x08, 0x09, 0x0A, //0x01
	0x0D, 0x12, 0x0E, 0x16, 0x0A, //0x05
};
*/

struct i2c_client	*max8893_client;

struct max8893_data *client_8893data_p = NULL;

static int max8893_i2c_cache_read(struct i2c_client *client, u8 reg, u8 *dest)
{
	*dest = max8893_cache_regs[reg];

	DBG("func =%s : reg = %d, value = %x\n",__func__,reg, max8893_cache_regs[reg]);
 
	return 0;
}

static int max8893_i2c_read(struct i2c_client *client, u8 reg, u8 *dest)
{
	int ret;

	DBG("func =%s :: Start!!!\n",__func__);  

	ret = i2c_smbus_read_byte_data(client, reg);

	DBG("func =%s : i2c_smbus_read_byte_data -> return data = %d\n",__func__, ret);  
 
	if (ret < 0)
		return -EIO;

	max8893_cache_regs[reg] = ret;

	DBG("func =%s : reg = %d, value = %x\n",__func__,reg, ret);  

	*dest = ret & 0xff;
	return 0;
}

static int max8893_i2c_write(struct i2c_client *client, u8 reg, u8 value)
{
	DBG("func =%s : reg = %d, value = %x\n",__func__,reg, value); 

	max8893_cache_regs[reg] = value;
	return i2c_smbus_write_byte_data(client, reg, value);
}

static u8 max8893_read_reg(struct max8893_data *max8893, u8 reg)
{

	u8 val = 0;
	DBG("func =%s called for reg= %x\n",__func__,reg);

#ifndef CONFIG_CPU_FREQ
	mutex_lock(&max8893->mutex);
#endif
	max8893_i2c_cache_read(max8893->client, reg, &val);
#ifndef CONFIG_CPU_FREQ
	mutex_unlock(&max8893->mutex);
#endif

	return val;
}

static int max8893_write_reg(struct max8893_data *max8893, u8 value, u8 reg)
{

	DBG("func =%s called for reg= %x, data=%x\n",__func__,reg,value);

#ifndef CONFIG_CPU_FREQ
	mutex_lock(&max8893->mutex);
#endif

	max8893_i2c_write(max8893->client, reg, value);
#ifndef CONFIG_CPU_FREQ
	mutex_unlock(&max8893->mutex);
#endif
	return 0;
}

static const int ldo13_8893_voltage_map[] = {
	1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300, 2400, 2500,
	2600, 2700, 2800, 2900, 3000, 3100, 3200, 3300, 
};

static const int ldo2_8893_voltage_map[] = {
	1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 
	2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900, 3000, 3100, 
	3200, 3300,
};

static const int ldo45_8893_voltage_map[] = {
	 800,  900, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700,
	1800, 1900, 2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700,
	2800, 2900, 3000, 3100, 3200, 3300, 
};

static const int buck_8893_voltage_map[] = {
	 800,  900, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700,
	1800, 1900, 2000, 2100, 2200, 2300, 2400,
};

static const int *ldo_8893_voltage_map[] = {
	NULL,
	ldo13_8893_voltage_map,	/* LDO1 */
	ldo2_8893_voltage_map,	/* LDO2 */
	ldo13_8893_voltage_map,	/* LDO3 */
	ldo45_8893_voltage_map,	/* LDO4 */
	ldo45_8893_voltage_map,	/* LDO5 */
 buck_8893_voltage_map,	/* BUCK */
};

static int max8893_get_ldo(struct regulator_dev *rdev)
{
	return rdev_get_id(rdev);
}

static int max8893_ldo_is_enabled(struct regulator_dev *rdev)
{
	struct max8893_data *max8893 = rdev_get_drvdata(rdev);
	int ldo = max8893_get_ldo(rdev);
	int value, shift;

	if (ldo <= MAX8893_LDO5) { /*LDO1~LDO5*/
		value = max8893_read_reg(max8893, MAX8893_REG_ONOFF);
		shift = 6 - ldo;
	} else { /*BUCK*/
		value = max8893_read_reg(max8893, MAX8893_REG_ONOFF);
		shift = 7;
	}
	return (value >> shift) & 0x1;
}

static int max8893_ldo_enable(struct regulator_dev *rdev)
{
	struct max8893_data *max8893 = rdev_get_drvdata(rdev);
	int ldo = max8893_get_ldo(rdev);
	int value, shift;

	DBG("func =%s called for regulator = %d\n",__func__,ldo);

	if (ldo <= MAX8893_LDO5) { /*LDO1~LDO5*/
		value = max8893_read_reg(max8893, MAX8893_REG_ONOFF);
		shift = 6 - ldo;
		value |= (1 << shift);
		max8893_write_reg(max8893, value, MAX8893_REG_ONOFF);
	} else { /*BUCK*/
		value = max8893_read_reg(max8893, MAX8893_REG_ONOFF);
		shift = 7;
		value |= (1 << shift);
		max8893_write_reg(max8893, value, MAX8893_REG_ONOFF);
	}
 
	return 0;
}

static int max8893_ldo_disable(struct regulator_dev *rdev)
{
	struct max8893_data *max8893 = rdev_get_drvdata(rdev);
	int ldo = max8893_get_ldo(rdev);
	int value, shift;
	
	DBG("func =%s called for regulator = %d\n",__func__,ldo);

	if (ldo <= MAX8893_LDO5) { /*LDO1~LDO5*/
		value = max8893_read_reg(max8893, MAX8893_REG_ONOFF);
		shift = 6 - ldo;
		value &= ~(1 << shift);
		max8893_write_reg(max8893, value, MAX8893_REG_ONOFF);
	} else { /*BUCK*/
		value = max8893_read_reg(max8893, MAX8893_REG_ONOFF);
		shift = 7;
		value &= ~(1 << shift);
		max8893_write_reg(max8893, value, MAX8893_REG_ONOFF);
	}

	return 0;
}

static int max8893_ldo_get_voltage(struct regulator_dev *rdev)
{
	struct max8893_data *max8893 = rdev_get_drvdata(rdev);
	int ldo = max8893_get_ldo(rdev);
	int value, shift = 0, mask = 0xff, reg;

	DBG("func =%s called for regulator = %d\n",__func__,ldo);

	if (ldo == MAX8893_LDO1) {
		reg  = MAX8893_REG_LDO1;
	} else if (ldo == MAX8893_LDO2) {
		reg  = MAX8893_REG_LDO2;
	} else if (ldo == MAX8893_LDO3) {
		reg  = MAX8893_REG_LDO3;
	} else if (ldo == MAX8893_LDO4) {
		reg  = MAX8893_REG_LDO4;
	} else if (ldo == MAX8893_LDO5) {
		reg  = MAX8893_REG_LDO5;
	} else if (ldo == MAX8893_BUCK) {
		reg  = MAX8893_REG_BUCK;
	}

	value = max8893_read_reg(max8893, reg); 
//	value >>= shift;
//	value &= mask;

	return 1000 * ldo_8893_voltage_map[ldo][value];
	
}

static int max8893_ldo_set_voltage(struct regulator_dev *rdev,
				int min_uV, int max_uV)
{
	struct max8893_data *max8893 = rdev_get_drvdata(rdev);
	int ldo = max8893_get_ldo(rdev);
	int value, shift = 0, mask = 0xff, reg;
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	const int *vol_map = ldo_8893_voltage_map[ldo];
	int i = 0;
 
	DBG("func =%s called for regulator = %d, min_v=%d, max_v=%d\n",__func__,ldo,min_uV,max_uV);

	if (min_vol < 800 ||
	    max_vol > 3300)
		return -EINVAL;

	while (vol_map[i]) {
		if (vol_map[i] >= min_vol)
			break;
		i++;
	}

 value = i;
	
	DBG("Found voltage=%d, i = %x\n",vol_map[i], i);

	if (!vol_map[i])
		return -EINVAL;

	if (vol_map[i] > max_vol)
		return -EINVAL;

	if (ldo == MAX8893_LDO1) {
		reg  = MAX8893_REG_LDO1;
	} else if (ldo == MAX8893_LDO2) {
		reg  = MAX8893_REG_LDO2;
	} else if (ldo == MAX8893_LDO3) {
		reg  = MAX8893_REG_LDO3;
	} else if (ldo == MAX8893_LDO4) {
		reg  = MAX8893_REG_LDO4;
	} else if (ldo == MAX8893_LDO5) {
		reg  = MAX8893_REG_LDO5;
	} else if (ldo == MAX8893_BUCK) {
		reg  = MAX8893_REG_BUCK;
	}

//	value = max8893_read_reg(max8893, reg);
//	value &= ~mask;
//	value |= (i << shift);
	max8893_write_reg(max8893, value, reg);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
//DIRECT APIS
int max8893_ldo_is_enabled_direct(int ldo)
{
	struct max8893_data *max8893 = client_8893data_p;
	int value, shift;
 
	if((ldo < MAX8893_LDO1) || (ldo > MAX8893_BUCK))
	{
		printk("ERROR: Invalid argument passed\n");
		return -EINVAL;
	}
 
	DBG("func =%s called for regulator = %d\n",__func__,ldo);

	if (ldo <= MAX8893_LDO5) { /*LDO1~LDO5*/
		value = max8893_read_reg(max8893, MAX8893_REG_ONOFF);
		shift = 6 - ldo;
	} else { /*BUCK*/
		value = max8893_read_reg(max8893, MAX8893_REG_ONOFF);
		shift = 7;
	}
 
	return (value >> shift) & 0x1;
}
EXPORT_SYMBOL_GPL(max8893_ldo_is_enabled_direct);

int max8893_ldo_enable_direct(int ldo)
{
	struct max8893_data *max8893 = client_8893data_p;
	int value, shift;

	if((ldo < MAX8893_LDO1) || (ldo > MAX8893_BUCK))
	{
		printk("ERROR: Invalid argument passed\n");
		return -EINVAL;
	}

	DBG("func =%s called for regulator = %d\n",__func__,ldo);

    //Thomas Ryu 20100409
    //(MSB)EBUK:1, ELS:X,ELDO1:0,ELDO2:0,ELDO3:X,ELDO4:0,ELDO5:0,EUSB:X(LSB)
    if(ldo == MAX8893_BUCK)
    {
      shift = 7;
    }
    else
    {
      shift = 6 - ldo;
    }

	value = max8893_read_reg(max8893, MAX8893_REG_ONOFF);
	value |= (1 << shift);
	max8893_write_reg(max8893, value, MAX8893_REG_ONOFF);

	return 0;
}
EXPORT_SYMBOL_GPL(max8893_ldo_enable_direct);

int max8893_ldo_disable_direct(int ldo)
{
	struct max8893_data *max8893 = client_8893data_p;
	int value, shift;

	if((ldo < MAX8893_LDO1) || (ldo > MAX8893_BUCK))
	{
		printk("ERROR: Invalid argument passed\n");
		return -EINVAL;
	}

	DBG("func =%s called for regulator = %d\n",__func__,ldo);


    //Thomas Ryu 20100409
    //(MSB)EBUK:1, ELS:X,ELDO1:0,ELDO2:0,ELDO3:X,ELDO4:0,ELDO5:0,EUSB:X(LSB)
    if(ldo == MAX8893_BUCK)
    {
      shift = 7;
    }
    else
    {
      shift = 6 - ldo;
    }

	value = max8893_read_reg(max8893, MAX8893_REG_ONOFF);
	value &= ~(1 << shift);
	max8893_write_reg(max8893, value, MAX8893_REG_ONOFF);
	
	return 0;
}

EXPORT_SYMBOL_GPL(max8893_ldo_disable_direct);

int max8893_onoff_disable_direct(int bit_position)
{
	struct max8893_data *max8893 = client_8893data_p;
    int value;

	DBG("func =%s called bit_position = %d\n",__func__,bit_position);

    //(MSB)EBUK:1, ELS:X,ELDO1:0,ELDO2:0,ELDO3:X,ELDO4:0,ELDO5:0,EUSB:X(LSB)
	value = max8893_read_reg(max8893, MAX8893_REG_ONOFF);
	value &= ~(1 << bit_position);
	max8893_write_reg(max8893, value, MAX8893_REG_ONOFF);

	return 0;
}

EXPORT_SYMBOL_GPL(max8893_onoff_disable_direct);

int max8893_onoff_enable_direct(int bit_position)
{
	struct max8893_data *max8893 = client_8893data_p;
    int value;

	DBG("func =%s called bit position = %d\n",__func__,bit_position);

    //(MSB)EBUK:1, ELS:X,ELDO1:0,ELDO2:0,ELDO3:X,ELDO4:0,ELDO5:0,EUSB:X(LSB)
	value = max8893_read_reg(max8893, MAX8893_REG_ONOFF);
	value |= (1 << bit_position);
	max8893_write_reg(max8893, value, MAX8893_REG_ONOFF);

	return 0;
}

EXPORT_SYMBOL_GPL(max8893_onoff_enable_direct);

int max8893_onoff_get_direct(void)
{
	struct max8893_data *max8893 = client_8893data_p;
	int value;

    //(MSB)EBUK:1, ELS:X,ELDO1:0,ELDO2:0,ELDO3:X,ELDO4:0,ELDO5:0,EUSB:X(LSB)
	value = max8893_read_reg(max8893, MAX8893_REG_ONOFF);
	DBG("func =%s called value = 0x%x\n",__func__,value);
	return value;
}

EXPORT_SYMBOL_GPL(max8893_onoff_get_direct);

int max8893_onoff_set_direct(int value)
{
	struct max8893_data *max8893 = client_8893data_p;

    DBG("func =%s value = %d\n",__func__,value);
	max8893_write_reg(max8893, value, MAX8893_REG_ONOFF);

    return 0;
}

EXPORT_SYMBOL_GPL(max8893_onoff_set_direct);

int max8893_ldo_set_voltage_direct(int ldo,
				int min_uV, int max_uV)
{
	struct max8893_data *max8893 = client_8893data_p;
	int value, shift = 0, mask = 0xff, reg;
	int min_vol = min_uV / 1000, max_vol = max_uV / 1000;
	const int *vol_map = ldo_8893_voltage_map[ldo];
	int i = 0;

	if((ldo < MAX8893_LDO1) || (ldo > MAX8893_BUCK))
	{
		printk("ERROR: Invalid argument passed\n");
		return -EINVAL;
	}
 
	DBG("func =%s called for regulator = %d, min_v=%d, max_v=%d\n",__func__,ldo,min_uV,max_uV);

	if (min_vol < 800 ||
	    max_vol > 3300)
		return -EINVAL;

	while (vol_map[i]) {
		if (vol_map[i] >= min_vol)
			break;
		i++;
	}

 value = i;
	
	DBG("Found voltage=%d, i = %x\n",vol_map[i], i);

	if (!vol_map[i])
		return -EINVAL;

	if (vol_map[i] > max_vol)
		return -EINVAL;

	if (ldo == MAX8893_LDO1) {
		reg  = MAX8893_REG_LDO1;
	} else if (ldo == MAX8893_LDO2) {
		reg  = MAX8893_REG_LDO2;
	} else if (ldo == MAX8893_LDO3) {
		reg  = MAX8893_REG_LDO3;
	} else if (ldo == MAX8893_LDO4) {
		reg  = MAX8893_REG_LDO4;
	} else if (ldo == MAX8893_LDO5) {
		reg  = MAX8893_REG_LDO5;
	} else if (ldo == MAX8893_BUCK) {
		reg  = MAX8893_REG_BUCK;
	}

//	value = max8893_read_reg(max8893, reg);
//	value &= ~mask;
//	value |= (i << shift);
	max8893_write_reg(max8893, value, reg);

	return 0;
}

EXPORT_SYMBOL_GPL(max8893_ldo_set_voltage_direct);


static struct regulator_ops max8893_ldo_ops = {
//	.list_voltage	= max8998_ldo_list_voltage,
	.is_enabled	= max8893_ldo_is_enabled,
	.enable		= max8893_ldo_enable,
	.disable	= max8893_ldo_disable,
	.get_voltage	= max8893_ldo_get_voltage,
	.set_voltage	= max8893_ldo_set_voltage,
};

static struct regulator_desc regulators_8893[] = {
	{
		.name		= "8893_LDO1",
		.id		= MAX8893_LDO1,
		.ops		= &max8893_ldo_ops,
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "8893_LDO2",
		.id		= MAX8893_LDO2,
		.ops		= &max8893_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo23_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "8893_LDO3",
		.id		= MAX8893_LDO3,
		.ops		= &max8893_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo23_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "8893_LDO4",
		.id		= MAX8893_LDO4,
		.ops		= &max8893_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "8893_LDO5",
		.id		= MAX8893_LDO5,
		.ops		= &max8893_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	}, {
		.name		= "8893_BUCK",
		.id		= MAX8893_BUCK,
		.ops		= &max8893_ldo_ops,
	//	.n_voltages	= ARRAY_SIZE(ldo45679_voltage_map),
		.type		= REGULATOR_VOLTAGE,
		.owner		= THIS_MODULE,
	},
};

static int __devinit max8893_pmic_probe(struct i2c_client *client,
				const struct i2c_device_id *i2c_id)
{
	struct regulator_dev **rdev;
	struct max8893_platform_data *pdata = client->dev.platform_data;
	struct max8893_data *max8893;
	int i = 0, id, ret;	
	int gpio_value=0;

	DBG("func =%s :: Start!!!\n",__func__);

	if (!pdata)
		return -EINVAL;

	max8893 = kzalloc(sizeof(struct max8893_data), GFP_KERNEL);
	if (!max8893)
		return -ENOMEM;

	max8893->rdev = kzalloc(sizeof(struct regulator_dev *) * (pdata->num_regulators + 1), GFP_KERNEL);
	if (!max8893->rdev) {
		kfree(max8893);
		return -ENOMEM;
	}

	max8893->client = client;
	max8893->dev = &client->dev;
	mutex_init(&max8893->mutex);

	max8893_client=client;

	max8893->num_regulators = pdata->num_regulators;
	for (i = 0; i < pdata->num_regulators; i++) {

		DBG("regulator_register called for ldo=%d\n",pdata->regulators[i].id);
		id = pdata->regulators[i].id - MAX8893_LDO1;

		max8893->rdev[i] = regulator_register(&regulators_8893[id],
			max8893->dev, pdata->regulators[i].initdata, max8893);

		ret = IS_ERR(max8893->rdev[i]);
		if (ret)
			dev_err(max8893->dev, "regulator init failed\n");
	}

	rdev = max8893->rdev;

	max8893_i2c_read(client, MAX8893_REG_ONOFF, (u8 *) &ret);
	max8893_i2c_read(client, MAX8893_REG_DISCHARGE, (u8 *) &ret);
	max8893_i2c_read(client, MAX8893_REG_LSTIME, (u8 *) &ret);
 	max8893_i2c_read(client, MAX8893_REG_DVSRAMP, (u8 *) &ret);
	max8893_i2c_read(client, MAX8893_REG_BUCK, (u8 *) &ret);
	max8893_i2c_read(client, MAX8893_REG_LDO1, (u8 *) &ret);
	max8893_i2c_read(client, MAX8893_REG_LDO2, (u8 *) &ret);
 	max8893_i2c_read(client, MAX8893_REG_LDO3, (u8 *) &ret);
	max8893_i2c_read(client, MAX8893_REG_LDO4, (u8 *) &ret);
 	max8893_i2c_read(client, MAX8893_REG_LDO5, (u8 *) &ret);  
   
	i2c_set_clientdata(client, max8893);
	client_8893data_p = max8893; // store 8893 client data to be used later

	DBG("func =%s :: End!!!\n",__func__);
 
	return 0;
}


static int __devexit max8893_pmic_remove(struct i2c_client *client)
{
	struct max8893_data *max8893 = i2c_get_clientdata(client);
	struct regulator_dev **rdev = max8893->rdev;
	int i;

	for (i = 0; i <= max8893->num_regulators; i++)
		if (rdev[i])
			regulator_unregister(rdev[i]);
	kfree(max8893->rdev);
	kfree(max8893);
	i2c_set_clientdata(client, NULL);
	client_8893data_p = NULL;

	return 0;
}

static const struct i2c_device_id max8893_ids[] = {
	{ "max8893", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, max8893_ids);

static struct i2c_driver max8893_pmic_driver = {
	.probe		= max8893_pmic_probe,
	.remove		= __devexit_p(max8893_pmic_remove),
	.driver		= {
		.name	= "max8893",
	},
	.id_table	= max8893_ids,
};

static int __init max8893_pmic_init(void)
{
	return i2c_add_driver(&max8893_pmic_driver);
}
subsys_initcall(max8893_pmic_init);

static void __exit max8893_pmic_exit(void)
{
	i2c_del_driver(&max8893_pmic_driver);
};
module_exit(max8893_pmic_exit);

MODULE_DESCRIPTION("MAXIM 8893 voltage regulator driver");
MODULE_AUTHOR("PFe");
MODULE_LICENSE("GPL");
#endif//CONFIG_ARIES_VER_B1... pfe
