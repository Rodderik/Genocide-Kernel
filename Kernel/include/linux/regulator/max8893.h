/*
 * max8893.h  --  Voltage regulation for the Maxim 8893
 *
 * Copyright (C) 2009 Samsung Electrnoics
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

#ifndef REGULATOR_MAX8893
#define REGULATOR_MAX8893

#include <linux/regulator/machine.h>


#if 1//CONFIG_ARIES_VER_B1... pfe
enum {
	MAX8893_LDO1 = 1,
	MAX8893_LDO2,
	MAX8893_LDO3,
	MAX8893_LDO4,
	MAX8893_LDO5,
	MAX8893_BUCK,
};

/**
 * max8893_subdev_data - regulator data
 * @id: regulator Id
 * @initdata: regulator init data (contraints, supplies, ...)
 */
struct max8893_subdev_data {
	int		id;
	struct regulator_init_data	*initdata;
};

/**
 * max8893_platform_data - platform data for max8998
 * @num_regulators: number of regultors used
 * @regulators: regulator used
 */
struct max8893_platform_data {
	int num_regulators;
	struct max8893_subdev_data *regulators;
};

int max8893_ldo_is_enabled_direct(int ldo);
int max8893_ldo_disable_direct(int ldo);
int max8893_ldo_enable_direct(int ldo);
int max8893_onoff_disable_direct(int bit_position);
int max8893_onoff_enable_direct(int bit_position);
int max8893_onoff_set_direct(int value);
int max8893_onoff_get_direct(void);
int max8893_ldo_set_voltage_direct(int ldo, int min_uV, int max_uV);


#endif//CONFIG_ARIES_VER_B1... pfe
#endif