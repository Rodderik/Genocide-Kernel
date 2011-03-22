/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/nt35580.h>

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000

static unsigned short brightness_setting_table[] = {
	0x051, 0x17f,
	ENDDEF, 0x0000
};

const unsigned short nt35580_SEQ_DISPLAY_ON[] = {
	0x029,
	ENDDEF, 0x0000
};

const unsigned short nt35580_SEQ_DISPLAY_OFF[] = {
	0x028,
	SLEEPMSEC,	27, /* more than 25ms */
	ENDDEF, 0x0000
};

const unsigned short nt35580_SEQ_SETTING[] = {
	/* SET_PIXEL_FORMAT */
	0x3A,
	0x177,	/* 24 bpp */
	/* RGBCTRL */
	0x3B,
	/* RGB Mode1, DE is sampled at the rising edge of PCLK,
	* P-rising edge, EP- low active, HSP-low active, VSP-low active */
	0x107,
	0x10A,
	0x10E,
	0x10A,
	0x10A,
	/* SET_HORIZONTAL_ADDRESS (Frame Memory Area define) */
	0x2A,
	0x100,
	0x100,
	0x101,	/* 480x800 */
	0x1DF,	/* 480x800 */
	/* SET_VERTICAL_ADDRESS  (Frame Memory Area define) */
	0x2B,
	0x100,
	0x100,
	0x103,	/* 480x800 */
	0x11F,	/* 480x800 */
	/* SET_ADDRESS_MODE */
	0x36,
	0x1D4,
	SLEEPMSEC, 30,	/* recommend by Sony-LCD, */
	/* SLPOUT */
	0x11,
	SLEEPMSEC, 155, /* recommend by Sony */
	/* WRCTRLD-1 */
	0x55,
	0x100,	/* CABC Off   1: UI-Mode, 2:Still-Mode, 3:Moving-Mode */
	/* WRCABCMB */
	0x5E,
	/* Minimum Brightness Value Setting 0:the lowest, 0xFF:the highest */
	0x100,
	/* WRCTRLD-2 */
	0x53,
	/* BCTRL(1)-PWM Output Enable, A(0)-LABC Off,
	* DD(1)-Enable Dimming Function Only for CABC,
	* BL(1)-turn on Backlight Control without dimming effect */
	0x12C,
	ENDDEF, 0x0000
};

const unsigned short nt35580_SEQ_SLEEP_IN[] = {
	0x010,
	SLEEPMSEC, 155,  /* more than 150ms */
	ENDDEF, 0x0000
};

struct s5p_panel_data_tft herring_panel_data_tft = {
	.seq_set = nt35580_SEQ_SETTING,
	.sleep_in = nt35580_SEQ_SLEEP_IN,
	.display_on = nt35580_SEQ_DISPLAY_ON,
	.display_off = nt35580_SEQ_DISPLAY_OFF,
	.brightness_set = brightness_setting_table,
};

