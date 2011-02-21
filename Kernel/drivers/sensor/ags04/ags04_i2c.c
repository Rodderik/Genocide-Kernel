
#include <linux/kernel.h>
#include <linux/i2c.h>

#include "ags04_dev.h"
#include "common.h"
#include "ags04.h"
#include "ags04_i2c.h"

/*extern functions*/
int ags04_i2c_drv_init(void);
void ags04_i2c_drv_exit(void);

/*static functions*/
static int ags04_probe (struct i2c_client *client,const struct i2c_device_id *id);
static int ags04_remove(struct i2c_client *);
static int ags04_suspend(struct i2c_client *, pm_message_t mesg);
static int ags04_resume(struct i2c_client *);


/*I2C Setting*/
#define AGS04_I2C_ADDRESS      0xD4

static struct i2c_driver ags04_i2c_driver;

static const struct i2c_device_id ags04_i2c_idtable[] = {
	{ "ags04_driver", 0 },
	{},
};

MODULE_DEVICE_TABLE(i2c, ags04_i2c_idtable);

static struct i2c_driver ags04_i2c_driver =
{
    .driver = {
        .name = "ags04_driver",
        .owner= THIS_MODULE,
    },

    .id_table   = ags04_i2c_idtable,
    .probe = ags04_probe,
    .remove = ags04_remove,
    .suspend = ags04_suspend,
    .resume = ags04_resume,
};

static int ags04_probe (struct i2c_client *client,const struct i2c_device_id *id)
{
	struct ags04_data *data;
	struct device *dev = &client->dev;
    	int ret = 0;

        trace_in();

        data = kzalloc(sizeof(struct ags04_data), GFP_KERNEL);
        if (data == NULL) {
		printk("failed to allocate memory:%s",__FUNCTION__);
                return -ENOMEM;
        }

    	if( strcmp(client->name, "ags04_driver" ) != 0 )
    	{
        	ret = -1;
        	printk(KERN_ERR "%s: device not supported", __FUNCTION__ );
        	return ret ;
    	}
    	else if( ( ags04_dev_init(client)) < 0 )
    	{
    		ret= -1 ;
        	printk(KERN_ERR "%s: ags04_dev_init failed", __FUNCTION__ );
    	}

    	trace_out() ;
    	return ret;
}

static int ags04_remove(struct i2c_client *client)
{

	struct ags04_data *data = i2c_get_clientdata(client);
	
	kfree(data);
	
	return 0;

}

static int ags04_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int ret = 0;
    trace_in() ;
	   
    if( strcmp(client->name, "ags04_driver" ) != 0 )
    {
        ret = -1;
        printk(KERN_ERR "%s: ags04_suspend: device not supported", __FUNCTION__ );
    }

    trace_out() ; 
    return ret;
}

static int ags04_resume(struct i2c_client *client)
{
    int ret = 0;
    trace_in() ;
	   
    if( strcmp(client->name, "ags04_driver" ) != 0 )
    {
        ret = -1;
        printk(KERN_ERR "%s: ags04_resume: device not supported", __FUNCTION__ );
    }
    trace_out() ; 
    return ret;
}

int ags04_i2c_drv_init(void)
{	
    int ret = 0;
    trace_in() ;

    ret = i2c_add_driver(&ags04_i2c_driver);

    if ( ret < 0 ) 
    {
        printk("%s:***************  ags04 i2c_add_driver failed,*************** ret : %d\n", __FUNCTION__, ret );
    }

    trace_out() ; 
    return ret;
}

void ags04_i2c_drv_exit(void)
{
    trace_in() ;

    i2c_del_driver(&ags04_i2c_driver);

    trace_out() ; 
}


