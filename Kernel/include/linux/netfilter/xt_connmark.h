#ifndef _XT_CONNMARK_H_target
#define _XT_CONNMARK_H_target

#include <linux/types.h>

/* Copyright (C) 2002,2004 MARA Systems AB <http://www.marasystems.com>
 * by Henrik Nordstrom <hno@marasystems.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#if defined CONFIG_S5PV210_VICTORY
enum {
	XT_CONNMARK_SET = 0,
	XT_CONNMARK_SAVE,
	XT_CONNMARK_RESTORE
};

struct xt_connmark_target_info {
	unsigned long mark;
	unsigned long mask;
	u_int8_t mode;
};

struct xt_connmark_tginfo1 {
	u_int32_t ctmark, ctmask, nfmask;
	u_int8_t mode;
};
#if defined(CONFIG_ARIES_VER_B2) 

struct xt_connmark_info {
	unsigned long mark, mask;
	u_int8_t invert;
};

struct xt_connmark_mtinfo1 {
	u_int32_t mark, mask;
	u_int8_t invert;
};
#endif
#elif defined CONFIG_S5PV210_ATLAS
struct xt_connmark_mtinfo1 {
	__u32 mark, mask;
	__u8 invert;
};
#endif
#endif /*_XT_CONNMARK_H_target*/
