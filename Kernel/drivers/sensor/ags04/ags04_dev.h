#ifndef _ags04_DEV_H
#define _ags04_DEV_H

#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include "ags04.h"

extern int ags04_status;
/*Function prototypes*/
extern int ags04_dev_init(struct i2c_client *); // dev.c
extern int ags04_dev_exit(void);	// dev.c

extern void ags04_dev_mutex_init(void); // dev.c

extern int ags04_dev_set_status( int, int ) ;
extern int ags04_dev_get_status( void ) ;

extern void ags04_dev_work_func( struct work_struct* ) ;

extern int i2c_read(unsigned char, unsigned char *);
extern int i2c_write(unsigned char, unsigned char);
extern int i2c_read_byte(unsigned char , unsigned char *, int );

extern int start_pattern_operation(void);
extern int start_normal_operation(void);

extern int grip_i2c_test(void);
extern int grip_rnd_cs_test(void);
extern int grip_pin_test(unsigned short test);

#define INT_PIN 1
#define VDD_PIN 2

#endif

