#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/irq.h>

#include <mach/hardware.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/stat.h>
#include <linux/ioctl.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/wakelock.h>

#include "ags04_i2c.h"
#include "ags04_dev.h"
#include "common.h"

/*****************IOCTLS******************/
/*magic no*/
#define AGS04_IOC_MAGIC  0xF4
/*max seq no*/
#define AGS04_IOC_NR_MAX 7 

#define AGS04_IOC_START_NORMAL_OP           _IO(AGS04_IOC_MAGIC, 0)
#define AGS04_IOC_START_PATTERN_OP         _IO(AGS04_IOC_MAGIC, 1)
#define AGS04_IOC_I2C_TEST          _IOR(AGS04_IOC_MAGIC, 2, int)
#define AGS04_IOC_RND_CS_TEST          _IOR(AGS04_IOC_MAGIC, 3, int)
#define AGS04_IOC_INT_PIN_TEST          _IOR(AGS04_IOC_MAGIC, 4, int)
#define AGS04_IOC_VDD_PIN_TEST          _IOR(AGS04_IOC_MAGIC, 5, int)
#define AGS04_IOC_READ_CNT          _IOR(AGS04_IOC_MAGIC, 6, int)

/*****************************************/

static int GRIP_INT_GPIO ;

int ags04_status= NOT_DETECTED ;
int pattern_int = 0; // 0 is normal, 1 is pattern interrupt 
int count_table[3] = {0}; 
unsigned short irq_flag = 0;


struct class *gripsensor_class;
EXPORT_SYMBOL(gripsensor_class);

struct device *switch_cmd;
EXPORT_SYMBOL(switch_cmd);

/*Work struct*/
static struct work_struct ags04_ws ;

/*ISR*/
static irqreturn_t ags04_isr( int irq, void *unused );
/*file operatons*/
static int ags04_open (struct inode *, struct file *);
static int ags04_release (struct inode *, struct file *);
static int ags04_ioctl(struct inode *, struct file *, unsigned int,  unsigned long);
// SERI:OK: add read function, will be used instead of ioctl
static ssize_t ags04_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);


/* wake lock */
struct wake_lock ags_wake_lock;

static struct file_operations ags04_fops =
{
    .owner = THIS_MODULE,
    .open = ags04_open,
    .ioctl = ags04_ioctl,
    .release = ags04_release,
    .read = ags04_read,
};

static struct miscdevice ags04_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "grip",
    .fops = &ags04_fops,
};

int read_count(int* rnd, int* ch1, int* ch3 )
{

    int ret = 0;
    int RndCount = 0;
	int Ch1Count = 0;
	int Ch3Count = 0;
    unsigned char data[6] = {0};
    int cnt;
    long present_data = 0;


        RndCount = Ch1Count = Ch3Count = 0;

      for(cnt= 0; cnt <50; )
      {
        if((ret = i2c_read_byte(0x40, data, 6))<0)
        {
          printk(KERN_ERR "%s: read_count_percent_register is failed.\n", __FUNCTION__);
        }

        if((((data[0] & 0xFF) == (data[5] & 0xFF)) && ((data[0] & 0xFF) != 0)))
        {
          present_data = data[3] * 256;           // MSB present data for RND & ch1,2,3,4.
          present_data = present_data + data[4]; // LSB present data for RND & ch1,2,3,4.
          if (data[0] == 0x01)
          {
            RndCount = present_data ;
          }
		  else if (data[0] == 0x02)
          {
            Ch1Count = present_data ;
          }
          else if (data[0] == 0x08)
          {
            Ch3Count = present_data ;
          }
          cnt++;
        }

        if((RndCount > 0) && (Ch1Count >0) && (Ch3Count > 0))
          cnt = 50;
      }

      *rnd = RndCount;
      *ch1 = Ch1Count;
      *ch3 = Ch3Count;

      return 0;

}

#if 0
void ags04_dev_work_func(void )
{
    int ret = 0; // ryun
    unsigned char data;
       
    trace_in();

   if( (ret = i2c_read( 0x00, &data )) < 0 )
    {
        printk(KERN_ERR "%s: Reading I2C data is failed.\n", __FUNCTION__);
     }

    data &= 0x05;
        printk(KERN_DEBUG "%s, GRIP interrupt!!\t data:0x%x\t", __FUNCTION__, data);

    if(pattern_int)
    {
      if(ags04_status == ALL_DETECTED)
      {
        ags04_status = NOT_DETECTED ;
        printk(KERN_DEBUG ": ags04_status is %d\n", ags04_status ) ;
      }
      else
      {
        ags04_status = ALL_DETECTED;
        printk(KERN_DEBUG ": ags04_status is %d\n", ags04_status ) ;
      }
    }
    else
    {
      switch(data)
      {
        case 1:
          ags04_status= CS1_DETECTED ;
          printk(KERN_DEBUG "CS1 Detected\n" ) ;
          break;
        case 4:
          ags04_status= CS3_DETECTED ;
          printk(KERN_DEBUG "CS3 Detected\n" ) ;
          break;
        case 5:
          ags04_status= ALL_DETECTED ;
          printk(KERN_DEBUG "ALL Detected\n" ) ;
          break;
        default:
          ags04_status= NOT_DETECTED ;
          printk(KERN_DEBUG "NOT Detected\n" ) ;
          break;
        }
    }

        /* call wake lock */
    wake_lock_timeout(&ags_wake_lock,2*HZ);
    printk(KERN_INFO "[GRIP] wake_lock_timeout : 2*HZ \n");
    ags04_enable_int();
    trace_out();
}

#endif

static irqreturn_t ags04_isr( int irq, void *unused )
{
//	int ret = 0; // ryun
//	unsigned char data;
    trace_in(); 

    ags04_disable_int() ;
    
    schedule_work(&ags04_ws);

    //read_count(&count_table[0], &count_table[1], &count_table[2]);
    //printk(KERN_DEBUG "%s: rndcnt :%d , ch1cnt:%d , ch3cnt:%d \n", __FUNCTION__, count_table[0], count_table[1], count_table[2]);

#if 0
    if( (ret = i2c_read( 0x00, &data )) < 0 )
    {
        printk(KERN_ERR "%s: Reading I2C data is failed.\n", __FUNCTION__);
     }

    data &= 0x05;
	printk(KERN_DEBUG "%s, GRIP interrupt!!\t data:0x%x\t", __FUNCTION__, data);

    if(pattern_int)
    {
      if(ags04_status == ALL_DETECTED)
      {
        ags04_status = NOT_DETECTED ;
        printk(KERN_DEBUG ": ags04_status is %d\n", ags04_status ) ;
      }
      else
      {
        ags04_status = ALL_DETECTED;
        printk(KERN_DEBUG ": ags04_status is %d\n", ags04_status ) ;
      }
    }
    else
    {
      switch(data)
      {
        case 1:
          ags04_status= CS1_DETECTED ;
    	  printk(KERN_DEBUG "CS1 Detected\n" ) ;
          break;
        case 4:
          ags04_status= CS3_DETECTED ;
    	  printk(KERN_DEBUG "CS3 Detected\n" ) ;
          break;
        case 5:
          ags04_status= ALL_DETECTED ;
          printk(KERN_DEBUG "ALL Detected\n" ) ;
          break;
        default:
          ags04_status= NOT_DETECTED ;
    	  printk(KERN_DEBUG "NOT Detected\n" ) ;
          break;
        }
    }
    
	/* call wake lock */
    wake_lock_timeout(&ags_wake_lock,2*HZ);
    printk(KERN_INFO "[GRIP] wake_lock_timeout : 2*HZ \n");
#endif
    trace_out();

    return IRQ_HANDLED;
}

void ags04_enable_int(void)
{
	trace_in() ;
	
    printk(KERN_DEBUG "%s, INTERRUPT ENABLED\n", __FUNCTION__ );
    irq_flag = 0;
    enable_irq( GRIP_IRQ);

    trace_out() ;
}

void ags04_disable_int(void)
{
	trace_in() ;

    printk(KERN_DEBUG "%s, INTERRUPT DISABLED\n", __FUNCTION__ );
    disable_irq_nosync( GRIP_IRQ);
    irq_flag = 1;
    trace_out() ;
}
static int ags04_open (struct inode *inode, struct file *filp)
{
    int ret = 0;
    trace_in() ; 
    
    nonseekable_open( inode, filp ) ;

    trace_out() ;
    return ret;
}

static int ags04_release (struct inode *inode, struct file *filp)
{
    trace_in() ;
    
    trace_out() ;
    return 0;
}


static int ags04_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,  unsigned long arg)
{
    int ret = 0, size = 0, err = 0;

    if(_IOC_TYPE(cmd) != AGS04_IOC_MAGIC)
      return -EINVAL;

    if(_IOC_NR(cmd) >= AGS04_IOC_NR_MAX)
      return -EINVAL;

    size = _IOC_SIZE(cmd);
    
    if(size)
    {
      err = 0;
      if(_IOC_DIR(cmd) & _IOC_READ)
        err = access_ok(VERIFY_WRITE, (void*)arg, size);
      else if(_IOC_DIR(cmd & _IOC_WRITE))
        err = access_ok(VERIFY_READ, (void*)arg, size);

      if(!err)
        return err;
    }
      
    trace_in() ;

    switch(cmd)
    {
      case AGS04_IOC_START_NORMAL_OP:
        if((ret = start_normal_operation()) < 0)
          printk(KERN_ERR "%s : AGS04_IOC_START_NORMAL_OP is failed. ret(%d)", __FUNCTION__, ret);
        break;
        
      case AGS04_IOC_START_PATTERN_OP:
        if((ret = start_pattern_operation()) < 0)
          printk(KERN_ERR "%s : AGS04_IOC_START_PATTERN_OP is failed. ret(%d)", __FUNCTION__, ret);
        break;
        
      case AGS04_IOC_I2C_TEST:
        if((ret = grip_i2c_test()) < 0)
          printk(KERN_ERR "%s : AGS04_IOC_I2C_TEST is failed. ret(%d)", __FUNCTION__, ret);
        err = copy_to_user((void *)arg, (void *)&ret, sizeof(int));
        break;

      case AGS04_IOC_RND_CS_TEST:
        if((ret = grip_rnd_cs_test()) < 0)
          printk(KERN_ERR "%s : AGS04_IOC_RND_CS_TEST is failed. ret(%d)", __FUNCTION__, ret);
        err = copy_to_user((void *)arg, (void *)&ret, sizeof(int));        
        break;

      case AGS04_IOC_INT_PIN_TEST:
        if((ret = grip_pin_test(INT_PIN)) < 0)
          printk(KERN_ERR "%s : AGS04_IOC_INT_PIN_TEST is failed. ret(%d)", __FUNCTION__, ret);
        err = copy_to_user((void *)arg, (void *)&ret, sizeof(int));        
        break;

      case AGS04_IOC_VDD_PIN_TEST:
        if((ret = grip_pin_test(VDD_PIN)) < 0)
          printk(KERN_ERR "%s : AGS04_IOC_VDD_PIN_TEST is failed. ret(%d)", __FUNCTION__, ret);
        err = copy_to_user((void *)arg, (void *)&ret, sizeof(int));        
        break;

      case AGS04_IOC_READ_CNT  :
        err = copy_to_user((void *)arg, (void *)&count_table, sizeof(count_table));        
        break;
        

      default:
        err = -EINVAL;
        break;
    }

    trace_out() ;
    return err;
}

// SERI:OK: read function
static ssize_t ags04_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    static unsigned short wait_cnt  = 0;
    char value = ags04_status;
    
	put_user(value, buf);
    //printk("%s : ags04_status(%d)\n", __FUNCTION__, ags04_status);

    if(irq_flag == 1)
    {
      wait_cnt++;
      if(wait_cnt == 2)
        {
          //printk("%s flag : %d, status : %d, I`m in !!!!!\n", __FUNCTION__, irq_flag, ags04_status);
          ags04_enable_int();
          wait_cnt = 0;
        }
    }
    //printk("%s flag : %d, status : %d\n", __FUNCTION__, irq_flag, ags04_status);

	return 1;
}

static ssize_t grip_read_cnt_show(struct device *dev, struct device_attribute *attr, char *buf)
{
  sprintf(buf, "rnd:%d ch1:%d ch3:%d\n", count_table[0], count_table[1], count_table[2]);
}


static DEVICE_ATTR(grip_read_cnt,0666, grip_read_cnt_show, NULL);


/************************************************************/

int __init ags04_driver_init(void)
{
    int ret = 0;
    unsigned int val = 0;
    trace_in() ;

    /*Initilize the ags04 dev mutex*/
    ags04_dev_mutex_init();

    /*Add the i2c driver*/
    if ( (ret = ags04_i2c_drv_init() < 0) ) 
    {
         goto MISC_IRQ_DREG;
    }

/* gpiot setting */
    s3c_gpio_setpull(GPIO_GRIP_INT, S3C_GPIO_PULL_UP); 

#if 0
    /*Add the input device driver*/
    input_dev= input_allocate_device() ;

    if( !input_dev ) {
    	printk( "%s, Input_allocate_deivce failed!, ret= %d\n",	__FUNCTION__, ret ) ;
    	return -ENODEV ;
    }

    set_bit( EV_ABS, input_dev->evbit ) ;
    set_bit( EV_SYN, input_dev->evbit ) ;

    //input_set_abs_params( input_dev, ABS_GAS, 0, 4095, 0, 0 ) ;ABS_PRESSURE
    input_set_abs_params( input_dev, ABS_PRESSURE, 0, 1, 0, 0 ) ;

    input_dev->name= "grip" ;

    ret= input_register_device( input_dev ) ;
    if( ret ) {
    	printk( "%s, Unable to register Input device: %s, ret= %d\n", __FUNCTION__,  input_dev->name, ret ) ;
    	return ret ;
    }
#endif

    /*misc device registration*/
    if( (ret = misc_register(&ags04_misc_device)) < 0 )
    {
        printk(KERN_ERR "%s, ags04_driver_init misc_register failed, ret= %d\n", __FUNCTION__, ret );
        goto MISC_DREG;
    }
    
    INIT_WORK(&ags04_ws, (void (*)(struct work_struct*))ags04_dev_work_func ) ;

	/* set sysfs for light sensor */

	gripsensor_class = class_create(THIS_MODULE, "gripsensor");
	if (IS_ERR(gripsensor_class))
		pr_err("Failed to create class(gripsensor)!\n");

	switch_cmd = device_create(gripsensor_class, NULL, 0, NULL, "switch");
	if (IS_ERR(switch_cmd))
		pr_err("Failed to create device(switch_cmd_dev)!\n");

	if (device_create_file(switch_cmd, &dev_attr_grip_read_cnt) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_grip_read_cnt.attr.name);

    /*set interrupt service routine*/
    set_irq_type ( GRIP_IRQ, IRQ_TYPE_EDGE_FALLING );
    enable_irq_wake ( GRIP_IRQ );

    if( (ret = request_irq(GRIP_IRQ, ags04_isr,0 , "ags04", (void *)NULL)) < 0 )
    {
        printk("%s, request_irq failed %d, ret= %d\n",__FUNCTION__, GRIP_IRQ, ret );
    }

    GRIP_INT_GPIO= GRIP_IRQ;
    printk(KERN_DEBUG "%s, GRIP_INT_GPIO: %d\n",__FUNCTION__, GRIP_IRQ ) ;

    ags04_disable_int() ;

    udelay( 500 ) ;

    ags04_enable_int() ;

    /* wake lock init */
    wake_lock_init(&ags_wake_lock, WAKE_LOCK_SUSPEND, "ags_wake_lock");

    trace_out() ;
    return ret;

MISC_IRQ_DREG:
/*
    free_irq(ags04_IRQ, (void *)NULL);
    */

MISC_DREG:
    misc_deregister(&ags04_misc_device);
		
    trace_out() ;
    return ret; 
}


void __exit ags04_driver_exit(void)
{
    trace_in() ;
		  
    /*Delete the i2c driver*/
    ags04_i2c_drv_exit();

    free_irq( GRIP_IRQ, (void *)NULL);
    
#if 0
    input_unregister_device( input_dev ) ;
#endif

    /*misc device deregistration*/
    misc_deregister(&ags04_misc_device);
    /* wake lock destroy */
    wake_lock_destroy(&ags_wake_lock);

    trace_out() ;
}

module_init(ags04_driver_init);
module_exit(ags04_driver_exit);

MODULE_AUTHOR("joon.c.baek <joon.c.baek@samsung.com>");
MODULE_DESCRIPTION("ags04 accel driver");
MODULE_LICENSE("GPL");
