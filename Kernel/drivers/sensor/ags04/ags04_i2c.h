#ifndef _ags04_I2C_DRV_H
#define _ags04_I2C_DRV_H
#include <linux/earlysuspend.h>
extern int ags04_i2c_drv_init(void);
extern void ags04_i2c_drv_exit(void);

struct ags04_data {
        struct i2c_client *client;
        struct input_dev *input_dev;
        struct work_struct ts_event_work;
        unsigned int irq;
        struct early_suspend    early_suspend;
};

#endif

