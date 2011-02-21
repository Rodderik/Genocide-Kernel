/* linux/drivers/video/samsung/s3cfb_ams397g201.h
 *
 * MIPI-DSI based AMS397G201 AMOLED LCD Panel definitions. 
 *
 * Copyright (c) 2009 Samsung Electronics
 * InKi Dae <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S3CFB_AMS397G201_H
#define _S3CFB_AMS397G201_H

extern void ams397g201_init(void);
extern void ams397g201_set_link(void *pd, unsigned int dsim_base,
	unsigned char (*cmd_write) (unsigned int dsim_base, unsigned int data0,
	    unsigned int data1, unsigned int data2),
	unsigned char (*cmd_read) (unsigned int dsim_base, unsigned int data0,
	    unsigned int data1, unsigned int data2));

#endif //_S3CFB_AMS397G201_H

