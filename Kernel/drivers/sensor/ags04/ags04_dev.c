
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#include "ags04_regs.h"
#include "ags04.h"
#include "ags04_dev.h"
#include "common.h"
#include <mach/hardware.h>
#include <mach/gpio.h>

#include <mach/param.h>

#define I2C_M_WR    0

#define TEST_FAIL -1
#define TEST_OK  0

extern int pattern_int;

extern struct wake_lock ags_wake_lock;
enum
{
    eTRUE,
    eFALSE,
} dev_struct_status_t ;

typedef struct
{
	struct mutex lock ;
	struct i2c_client const* client ;
	unsigned short valid ;
	int status ;
} ags04_device_t ;

static ags04_device_t    ags04_dev =
{
    .client= NULL,
    .valid=  eFALSE,
} ;

#define SENSOR_INIT_TABLE 7
unsigned sensor_init_table[SENSOR_INIT_TABLE][2]=
{
  {0x01, 0x05}, //channel enable
#if defined (CONFIG_ARIES_VER_B3)    
  {0x0C, 0x33},//{0x0C, 0x46}, //calibration speed control  
  {0x04, 0x03},//{0x04, 0x02}, //sensitivity setting
#else
  {0x0C, 0x46},//calibration speed control  
  {0x04, 0x02},//sensitivity setting
#endif
  {0x06, 0x03}, //sensitivity setting
  {0x03, 0x20}, //normal mode
  {0x02, 0x4D}, //soft reset
  {0x02, 0x4C}, //enable
};

#define PATTERN_INT_TABLE 12
#define PATTERN_1
unsigned pattern_int_table[PATTERN_INT_TABLE][2] =
{
    {0x0E, 0x05}, //output expiration enable
    {0x03, 0x21}, //pattern interrupt mode
    {0x1B, 0x00}, //pattern time setting
    {0x1C, 0x07},
    {0x1D, 0x04}, //pattern number setting
#if defined (PATTERN_1)
    {0x20, 0x00}, //pattern data setting
    {0x21, 0x05},
    {0x22, 0x00},
    {0x23, 0x05},
#elif defined (PATTERN_2)
    {0x20, 0x00}, //pattern data setting
    {0x21, 0x05},
    {0x22, 0x01},
    {0x23, 0x05},
#else
    {0x20, 0x00}, //pattern data setting
    {0x21, 0x05},
    {0x22, 0x04},
    {0x23, 0x05},
#endif
    {0x1A, 0x03}, //pattern mode set & enable
    {0x02, 0x4D}, //soft reset
    {0x02, 0x4C}, //enable
};

#define NORMAL_OP_TABLE 6
unsigned normal_op_table[NORMAL_OP_TABLE][2] =
{
    {0x0E, 0x05}, //output expiration enable
    {0x03, 0x20}, //normal touch operation 
    {0x16, 0x00}, //sensing burst period set
    {0x1A, 0x00}, //patter mode reset & disable
    {0x02, 0x4D}, //soft reset
    {0x02, 0x4C}, //enable
};

/*extern functions*/
/**********************************************/
/*All the exported functions which view or modify the device
  state/data, do i2c com will have to lock the mutex before 
  doing so*/
/**********************************************/
int ags04_dev_init(struct i2c_client *client );
int ags04_dev_exit(void);

void ags04_dev_mutex_init(void);

int ags04_dev_set_status( int, int ) ;
int ags04_dev_get_status( void ) ;

void ags04_dev_work_func( struct work_struct* ) ;

int i2c_read(unsigned char, unsigned char *);
int i2c_write(unsigned char, unsigned char);
int i2c_read_byte(unsigned char , unsigned char *, int );

int start_pattern_operation(void);
int start_normal_operation(void);

int write_sensing_duty(unsigned char ctrl_regsb, unsigned char reset);
int calculate_count(int* rnd, int* ch1, int* ch3 );


/**********************************************/

int start_pattern_operation(void)
{
  int ret = 0, cnt = 0;
  unsigned char ctrl_regsc=0, ctrl_regsb=0;

  pattern_int = 1; // 0 is normal, 1 is pattern interrupt 

  for(cnt = 0 ; cnt < PATTERN_INT_TABLE ; cnt++)
  {
    ctrl_regsc= pattern_int_table[cnt][0];
    ctrl_regsb= pattern_int_table[cnt][1];

    if( (ret = i2c_write( ctrl_regsc, ctrl_regsb )) < 0 )
    {
      printk(KERN_ERR "%s: register %d failed, %d\n", __FUNCTION__, cnt, ret );
    }
    printk(KERN_DEBUG "%d, a:0x%2x, d:0x%2x\n",cnt + 1 , ctrl_regsc, ctrl_regsb);
    udelay( 1000 ) ;
  }

  return ret;
}

int start_normal_operation(void)
{
  int ret = 0, cnt = 0;
  unsigned char ctrl_regsc=0, ctrl_regsb=0;

  pattern_int = 0; // 0 is normal, 1 is pattern interrupt 
  
  for(cnt = 0 ; cnt < NORMAL_OP_TABLE ; cnt++)
  {
    ctrl_regsc= normal_op_table[cnt][0];
    ctrl_regsb= normal_op_table[cnt][1];

    if( (ret = i2c_write( ctrl_regsc, ctrl_regsb )) < 0 )
    {
      printk(KERN_ERR "%s: register %d failed, %d\n", __FUNCTION__, cnt, ret );
    }
    printk(KERN_DEBUG "%d, a:0x%2x, d:0x%2x\n",cnt + 1 , ctrl_regsc, ctrl_regsb);
    udelay( 1000 ) ;
  }

  return ret;
}

int grip_i2c_test(void)
{
  int ret = 0;
  unsigned char data;

  if( (ret = i2c_write( 0x05, 0x1F )) < 0 )
  {
    printk(KERN_ERR "%s: i2c_write failed, %d\n", __FUNCTION__, ret );
  }

  mdelay(10);

  if( (ret = i2c_read( 0x05, &data )) < 0 )
  {
      printk(KERN_ERR "%s: Reading I2C data is failed.\n", __FUNCTION__);
   }

  if((data != 0x1F) || ret < 0)
    ret = TEST_FAIL;
  else
    ret = TEST_OK;

  return ret;
}

int grip_rnd_cs_test(void)
{
  int ret = 0;
  int rnd, ch1, ch3;

  calculate_count(&rnd, &ch1, &ch3);
  printk(KERN_DEBUG "%s: rndcnt :%d , ch1cnt:%d , ch3cnt:%d \n", __FUNCTION__, rnd, ch1, ch3);

  if((rnd < 1500) || (ch1 < 1500) || (ch3 < 1500))
    return TEST_FAIL;

  if((rnd > 3200) || (ch1 > 3200) || (ch3 > 3200))
    return TEST_FAIL;

  ret = TEST_OK;

  return ret;
}

int grip_pin_test(unsigned short test)
{
  int ret = 0, cnt = 0, value = -1, i2c_ret = 0;
  unsigned char dutydata=0, rndduty=0, chduty=0, tmp=0;
  
  //change interrupt mode
  if( (i2c_ret = i2c_write( 0x03, 0x22 )) < 0 ) 
  {
    printk(KERN_ERR "%s: i2c_write failed, %d\n", __FUNCTION__, i2c_ret );
  }

  //read dutydata
  if( (i2c_ret = i2c_read( 0x10, &dutydata )) < 0 )
  {
    printk(KERN_ERR "%s: Set 0x1%d failed, %d\n", __FUNCTION__, cnt, i2c_ret );
  }

  rndduty =(dutydata&0xF0)/16;
  chduty = (dutydata&0x0F);

  if(chduty < 2)
  {
    chduty = chduty + 2;
    tmp = (rndduty*16) + chduty;
    write_sensing_duty(tmp, 1);
  }

  tmp = (rndduty*16) + (chduty-2);

  write_sensing_duty(tmp, 0);

  mdelay( 200 ) ;

  if(test == INT_PIN) //int pin test
  {
    value = gpio_get_value(GPIO_GRIP_INT);
    printk(KERN_DEBUG "%s: INT_PIN value %d\n", __FUNCTION__, value );
  }

  gpio_direction_output(GPIO_GRIP_SDA_28V, 1);
  gpio_set_value(GPIO_GRIP_SDA_28V, 0);

  gpio_direction_output(GPIO_GRIP_SCL_28V, 1);
  gpio_set_value(GPIO_GRIP_SCL_28V, 0);

  mdelay( 100 ) ;
  
  if(test == VDD_PIN) //vdd&gnd pin test
  {
    value = gpio_get_value(GPIO_GRIP_INT);
    printk(KERN_DEBUG "%s: VDD_PIN value %d\n", __FUNCTION__, value );
  }

  if(value != 0)
    ret = TEST_FAIL;
  else
    ret = TEST_OK;

/*resiger restore*/
  if( (i2c_ret = i2c_write( 0x03, 0x20 )) < 0 ) //change interrupt mode
  {
    printk(KERN_ERR "%s: i2c_write failed, %d\n", __FUNCTION__, i2c_ret );
  }

  if( (i2c_ret = write_sensing_duty(dutydata, 1)) < 0 )
  {
    printk(KERN_ERR "%s: write_sensing_duty failed, %d\n", __FUNCTION__, i2c_ret );
  }

  return ret;
}


int read_count_percent_register(unsigned char* data)
{
  int cnt, ret = 0;
  unsigned char ctrl_regsc = 0;

  for(cnt = 0; cnt < 6; cnt++)
  {
    ctrl_regsc = 0x40 + cnt;
    if( (ret = i2c_read( ctrl_regsc, (data + cnt))) < 0 )
    {
        printk(KERN_ERR "%s: Reading I2C data is failed.%d\n", __FUNCTION__, ret);
     }
    //printk("%s: addr:0x%x, data:0x%x\n", __FUNCTION__,ctrl_regsc, *(data + cnt));
  }

  return ret;
}

int write_sensing_duty(unsigned char ctrl_regsb, unsigned char reset)
{
  int cnt, ret= 0;
  unsigned char ctrl_regsc = 0;
  for(cnt = 0 ; cnt < 6 ; cnt++)
  {
    ctrl_regsc = 0x10 + cnt;
    if( (ret = i2c_write( ctrl_regsc, ctrl_regsb )) < 0 )
    {
        printk(KERN_ERR "%s: Set 0x10 failed, %d\n", __FUNCTION__, ret );
    }

    //printk("%s: Set 0x%x addr, %d\, data:0x%x\n", __FUNCTION__,ctrl_regsc, ret,  ctrl_regsb);
    //udelay( 100 ) ;
  }

    if(reset)
    {
    // reset
    ctrl_regsb= 0x4D ;
    if( (ret = i2c_write( 0x02, ctrl_regsb )) < 0 )
    {
        printk(KERN_ERR "%s: Set Global option control register failed, %d\n", __FUNCTION__, ret );
    }
    udelay( 1000 ) ;

    ctrl_regsb= 0x4c ;
    if( (ret = i2c_write( 0x02, ctrl_regsb )) < 0 )
    {
        printk(KERN_ERR "%s: Set Global option control register failed, %d\n", __FUNCTION__, ret );
    }
      printk(KERN_DEBUG "%s: reset\n", __FUNCTION__);
    }
    return ret;
}

int calculate_count(int* rnd, int* ch1, int* ch3 )
{

    int ret = 0;
    int RndCount = 0;
	int RndNum   = 0;

	//int ChCount  = 0;

	int Ch1Count = 0;
	int Ch1Num   = 0;
	int Ch3Count = 0;
	int Ch3Num   = 0;
    unsigned char data[6] = {0};
    int cnt;
    long present_data = 0;


        RndCount = RndNum = Ch1Count = Ch1Num = Ch3Count = Ch3Num = 0;

      for(cnt= 0; cnt <50; )
      {
        if((ret = i2c_read_byte(0x40, data, 6))<0)
        {
          printk(KERN_ERR "%s: read_count_percent_register is failed.\n", __FUNCTION__);
        }

        //printk("%s: 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x cnt:%d\n", __FUNCTION__, data[0], data[1], data[2], data[3], data[4], data[5], cnt);
        if((((data[0] & 0xFF) == (data[5] & 0xFF)) && ((data[0] & 0xFF) != 0)))
        {
          present_data = data[3] * 256;           // MSB present data for RND & ch1,2,3,4.
          present_data = present_data + data[4]; // LSB present data for RND & ch1,2,3,4.
          if (data[0] == 0x01)
          {
            RndNum++;
            RndCount = RndCount + present_data ;
          }
		  else if (data[0] == 0x02)
          {
            Ch1Num++;
            Ch1Count = Ch1Count + present_data ;
          }
          else if (data[0] == 0x08)
          {
            Ch3Num++;
            Ch3Count = Ch3Count + present_data ;
          }
          cnt++;
        }
      }

      RndCount = (int)(RndCount/RndNum);
      Ch1Count = (int)(Ch1Count/Ch1Num);
      Ch3Count = (int)(Ch3Count/Ch3Num);

      //ChCount = (Ch1Count + Ch3Count) / 2;

      *rnd = RndCount;
      *ch1 = Ch1Count;
      *ch3 = Ch3Count;
      printk(KERN_DEBUG "%s: RndCount:%d, RndNum:%d, Ch1Num:%d, Ch3Num:%d, Ch1Count:%d, Ch3Count:%d\n",__FUNCTION__,RndCount, RndNum, Ch1Num, Ch3Num, Ch1Count, Ch3Count);

      return 0;

}

int set_auto_duty(int *data)
{
    int cnt,cnt2, ret = 0;
    int RndCount = 0;
	int RndDuty  = 4;//7; //4;
	int ChCount  = 0;
	int ChDuty   = 4;//7; //4;
	int DutyData = 0;

    long SetCount = 50;

    int test_result = 1;

    unsigned char SetCountFlag = 0;
    unsigned char sFlag = 0;
    unsigned char lFlag = 0;

    int Ch1Count = 0, Ch3Count = 0;



    for(cnt2=0; cnt2 < 10; cnt2++)
    {
      calculate_count(&RndCount, &Ch1Count, &Ch3Count);

      ChCount = (Ch1Count + Ch3Count) / 2;

      if ((ChCount < (RndCount - SetCount)) && (lFlag == 0))
      {
        sFlag = 1;
        if (RndDuty != 0 || ChDuty != 7)
        {
          if (RndDuty != 0)
          {
            RndDuty--;
          }
          else
          {
            ChDuty++;
          }
        }
        else
        {
          if(SetCountFlag != 1){
            SetCount = (int)(ChCount /10);
            SetCountFlag = 1;
             }
          else
            test_result = -1;
        }
      }
      else if ((ChCount > (RndCount + SetCount)) && (sFlag == 0))
      {
        lFlag = 1;
        if (RndDuty != 7 || ChDuty != 0)
        {
          if (RndDuty != 7)
          {
            RndDuty++;
          }
          else
          {
            ChDuty--;
          }
        }
        else
        {
          if(SetCountFlag != 1){
            SetCount = (int)(ChCount /10);
            SetCountFlag =1;
         }
         else
            test_result = -1;
        }
      }
      else
      {
        cnt =10;
        test_result = 0; //PASS
      }

      DutyData = (RndDuty*16) + ChDuty;
      *data = DutyData;
      if((ret = write_sensing_duty(DutyData, 1)) < 0)
        printk(KERN_ERR "%s: write_sensing_duty is failed.\n", __FUNCTION__);

      printk(KERN_DEBUG "%s: DutyData 0x%x\n", __FUNCTION__, DutyData);
    }
  return ret;
}


void set_duty_register(void)
{
  int duty_value = 0, ret = 0;

  if (sec_get_param_value)
    sec_get_param_value(__GRIP_DUTY, &duty_value);
      
  printk(KERN_DEBUG "%s : read duty_value(0x%x)\n", __FUNCTION__, duty_value);

  if((duty_value > 0)  && (duty_value <= 0xff))
  {
    if((ret = write_sensing_duty(duty_value, 1)) < 0)
      printk(KERN_ERR "%s: write_sensing_duty is failed.\n", __FUNCTION__);

    printk(KERN_DEBUG "%s : next botting(0x%x)\n", __FUNCTION__, duty_value);
  }
  else
  {
    set_auto_duty(&duty_value);
    if((duty_value <= 0) || (duty_value > 0xff))
    {
      printk(KERN_ERR "%s : auto duty value(0x%x)\n", __FUNCTION__, duty_value);
      duty_value = 0;
    }

    if (sec_set_param_value)
      sec_set_param_value(__GRIP_DUTY, &duty_value);

    printk(KERN_DEBUG "%s : first booting(0x%x)\n", __FUNCTION__, duty_value);
  }

  return;
}

int ags04_dev_init(struct i2c_client *client )
{
    int ret = 0;
    int cnt = 0;

    /*these have to be initialized to 0*/	 
    unsigned char ctrl_regsc=0, ctrl_regsb=0;
    trace_in() ;

    mutex_lock(&(ags04_dev.lock));   
    ags04_dev.client= client ; 
	//printk( "ags04_dev.client: 0x%x\n", ags04_dev.client->addr ) ;

    for(cnt = 0 ; cnt < SENSOR_INIT_TABLE; cnt++)
    {
      ctrl_regsc= sensor_init_table[cnt][0];
      ctrl_regsb= sensor_init_table[cnt][1];

      if( (ret = i2c_write( ctrl_regsc, ctrl_regsb )) < 0 )
      {
          printk(KERN_ERR "%s: register %d failed, %d\n", __FUNCTION__, cnt, ret );
      }
      printk(KERN_DEBUG "%d, a:0x%2x, d:0x%2x\n",cnt + 1 , ctrl_regsc, ctrl_regsb);
      udelay( 1000 ) ;
    }

   /* if((ret = write_sensing_duty(0x74), 1) < 0)
      printk("%s: write_sensing_duty is failed.\n", __FUNCTION__);*/

    set_duty_register();
    //set_auto_duty();

    pattern_int = 0; // 0 is normal, 1 is pattern interrupt 
    
    if(pattern_int) //pattern interrupt operating mode
    {
      ret = start_pattern_operation();
    }
    else
    {
      ret = start_normal_operation();
    }

    mutex_unlock(&(ags04_dev.lock));   	 

    trace_out() ;
    return ret;
}

int ags04_dev_exit(void)
{
    int ret = 0;
    trace_in() ;
	 
    mutex_lock(&(ags04_dev.lock));   

    ags04_dev.client = NULL;	

    ags04_dev.valid = eFALSE;

    mutex_unlock(&(ags04_dev.lock)); 

    trace_out() ;
    return ret;
}

void ags04_dev_mutex_init(void)
{
    trace_in() ;
    
    mutex_init(&(ags04_dev.lock));
    
    trace_out() ;
}

int ags04_dev_set_status( int module, int param ) 
{
	trace_in() ;

    /*
    if( param == STANDBY ) 
    {
        ags04_status= ags04_status & (~(1 << module)) ;
    }
    else if( param == NORMAL )
    {
    	ags04_status= ags04_status | ( 1 << module ) ;
    }
    else
    {
        printk( "ags04_dev_get_status() Fault argument!!! module= %d, param=%d\n", module, param  ) ;
    }
    */

	trace_out() ;
    return ags04_dev.status ;
}

int ags04_dev_get_status( void )
{
    trace_in() ;

    trace_out() ;
    return ags04_dev.status ;
}

void ags04_dev_work_func( struct work_struct* work )
{
	trace_in() ;
    mutex_lock(&(ags04_dev.lock));   
   // ags04_disable_int() ; 

    int ret = 0; // ryun
    unsigned char data;
       
    trace_in();

   if( (ret = i2c_read( 0x00, &data )) < 0 )
    {
        printk("%s: Reading I2C data is failed.\n", __FUNCTION__);
     }

    data &= 0x05;
        printk("%s, GRIP interrupt!!\t data:0x%x\t", __FUNCTION__, data);

    if(pattern_int)
    {
      if(ags04_status == ALL_DETECTED)
      {
        ags04_status = NOT_DETECTED ;
        printk(": ags04_status is %d\n", ags04_status ) ;
      }
      else
      {
        ags04_status = ALL_DETECTED;
        printk(": ags04_status is %d\n", ags04_status ) ;
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

    printk(KERN_DEBUG "ags04_dev_work_func()\n" ) ;
    ags04_enable_int() ; 
    mutex_unlock(&(ags04_dev.lock)); 
	trace_out() ;
}
/*i2c read function*/
/*ags04_dev.client should be set before calling this function.
   If ags04_dev.valid = eTRUE then ags04_dev.client will b valid
   This function should be called from the functions in this file. The 
   callers should check if ags04_dev.valid = eTRUE before
   calling this function. If it is eFALSE then this function should not
   be called. Init function is the only exception*/
int i2c_read(unsigned char reg, unsigned char *data)
{
    int ret = 0;
    struct i2c_msg msg[1];
    unsigned char aux[1];
    trace_in() ;

    msg[0].addr	= ags04_dev.client->addr;
    msg[0].flags = I2C_M_WR;
    msg[0].len = 1;
    aux[0] = reg;
    msg[0].buf = aux;

    ret = i2c_transfer(ags04_dev.client->adapter, msg, 1);

    if( ret < 0 )
    {
        printk(KERN_ERR "%s: i2c_read->i2c_transfer 1 failed\n", __FUNCTION__ );
    }
    else 
    {
        msg[0].flags = I2C_M_RD;
        msg[0].len   = 1;
        msg[0].buf   = data;
        ret = i2c_transfer(ags04_dev.client->adapter, msg, 1);

        if( ret < 0 )
        {
            printk(KERN_ERR "%s: i2c_read->i2c_transfer 2 failed\n", __FUNCTION__ );
        }
    }

/*
    gpio_set_value( GRIP_I2C_EN, GPIO_LEVEL_HIGH );
    udelay( 200 ) ;
    */

    trace_out() ;
    return ret;
}

int i2c_read_byte(unsigned char reg, unsigned char *data, int size)
{
    int ret = 0;
    struct i2c_msg msg[1];
    unsigned char aux[1];
    trace_in() ;

    msg[0].addr	= ags04_dev.client->addr;
    msg[0].flags = I2C_M_WR;
    msg[0].len = 1;
    aux[0] = reg;
    msg[0].buf = aux;

    ret = i2c_transfer(ags04_dev.client->adapter, msg, 1);

	
    if( ret < 0 )
    {
        printk(KERN_ERR "%s: i2c_read->i2c_transfer 1 failed\n", __FUNCTION__ );
    }
    else 
    {
        msg[0].flags = I2C_M_RD;
        msg[0].len   = size;
        msg[0].buf   = data;
        ret = i2c_transfer(ags04_dev.client->adapter, msg, 1);

        if( ret < 0 )
        {
            printk(KERN_ERR "%s: i2c_read->i2c_transfer 2 failed\n", __FUNCTION__ );
        }
    }

/*
    gpio_set_value( GRIP_I2C_EN, GPIO_LEVEL_HIGH );
    udelay( 200 ) ;
    */

    trace_out() ;
    return ret;
}


/*i2c write function*/
/*ags04_dev.client should be set before calling this function.
   If ags04_dev.valid = eTRUE then ags04_dev.client will b valid
   This function should be called from the functions in this file. The 
   callers should check if ags04_dev.valid = eTRUE before
   calling this function. If it is eFALSE then this function should not
   be called. Init function is the only exception*/
int i2c_write( unsigned char reg, unsigned char data )
{
    int ret = 0;
    struct i2c_msg msg[1];
    unsigned char buf[2];
    trace_in() ;

    msg[0].addr	= ags04_dev.client->addr;
    msg[0].flags = I2C_M_WR;
    msg[0].len = 2;

    buf[0] = reg;
    buf[1] = data;
    msg[0].buf = buf;

    ret = i2c_transfer(ags04_dev.client->adapter, msg, 1);

    if( ret < 0 )
    {
        printk(KERN_ERR "%s: i2c_write->i2c_transfer failed\n", __FUNCTION__ );
    }
    

/*
    gpio_set_value( GRIP_I2C_EN, GPIO_LEVEL_HIGH ) ;
    udelay( 200 ) ;
    */

    trace_out() ;
    return ret;
}


