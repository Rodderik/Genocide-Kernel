/*
 * Machine dependent access functions for RTC registers.
 */
 
 /*---------------------------------------------------------------------------*
 *                                                                           *
 * This program is free software; you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License version 2 as         *
 * published by the Free Software Foundation.                                *
 *                                                                           *
 *---------------------------------------------------------------------------*/
 
#ifndef _ASM_MC146818RTC_H
#define _ASM_MC146818RTC_H

#include <linux/io.h>
#if defined CONFIG_S5PV210_VICTORY
#include <mach/victory/irqs.h>
#elif defined CONFIG_S5PV210_ATLAS
#include <mach/atlas/irqs.h>
#endif
#ifndef RTC_PORT
#define RTC_PORT(x)	(0x70 + (x))
#define RTC_ALWAYS_BCD	1	/* RTC operates in binary mode */
#endif

/*
 * The yet supported machines all access the RTC index register via
 * an ISA port access but the way to access the date register differs ...
 */
#define CMOS_READ(addr) ({ \
outb_p((addr),RTC_PORT(0)); \
inb_p(RTC_PORT(1)); \
})
#define CMOS_WRITE(val, addr) ({ \
outb_p((addr),RTC_PORT(0)); \
outb_p((val),RTC_PORT(1)); \
})

#endif /* _ASM_MC146818RTC_H */
