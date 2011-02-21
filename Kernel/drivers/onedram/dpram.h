/****************************************************************************
**
** COPYRIGHT(C) : Samsung Electronics Co.Ltd, 2006-2010 ALL RIGHTS RESERVED
**
** AUTHOR       : Kim, Geun-Young <geunyoung.kim@samsung.com>			@LDK@
**                                                                      @LDK@
****************************************************************************/

#ifndef __DPRAM_H__

#if !defined(CONFIG_DPRAM_VERSION_CHECK)
#elif defined(CONFIG_DPRAM_1_2_0) 
#include "dpram_1.2.0.h"
#elif defined(CONFIG_DPRAM_1_1_0) 
#include "dpram_1.1.0.h"
#elif defined(CONFIG_DPRAM_1_0_0) //etinum.victory.dpram
#include "dpram_1.0.0.h"
#else
#error "dpram spec is not defined, check the dpram overview document"
#endif /**/

#endif	/* __DPRAM_H__ */

