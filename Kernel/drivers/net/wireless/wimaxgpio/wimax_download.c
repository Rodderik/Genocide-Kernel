#include <linux/kernel.h>
#include <linux/module.h>
#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
#include <linux/i2c/maximi2c.h>
#include <linux/i2c/pmic.h>
#endif
#include <linux/i2c.h>

#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
#include <mach/instinctq.h>	//cky 20100111 headers for quattro
#elif defined (CONFIG_MACH_ARIES)
#include <mach/gpio-jupiter.h>	// Yongha for Victory WiMAX 20100208
#endif
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#ifdef PREFIX
#undef PREFIX
#endif
#define PREFIX "Wimax730: "

#define EEPROM_I2C_ADDR 0xA0

static struct i2c_driver Wimax730_driver;
static struct i2c_client *Wimax730_i2c_client = NULL;


#if 0
static unsigned short Wimax730_normal_i2c[] = { (EEPROM_I2C_ADDR >> 1), I2C_CLIENT_END };
static unsigned short Wimax730_ignore[] = { I2C_CLIENT_END };
static unsigned short Wimax730_probe[] = { (EEPROM_I2C_ADDR >> 1), I2C_CLIENT_END };


static struct i2c_client_address_data Wimax730_addr_data = {
	.normal_i2c = Wimax730_normal_i2c,
	.ignore		= Wimax730_ignore,
	.probe		= Wimax730_probe,
};
#endif

static void Wimax730_translate_offset(unsigned *offset)
{
	unsigned i;

	i = *offset >> 16;
	*offset &= 0xffff;
}

int Wimax730_read(int offset, char *rxData, int length)
{
	struct i2c_msg msg[2];
	u8 msgbuf[2];
	int status, i;

	memset(msg, 0, sizeof(msg));

	Wimax730_translate_offset(&offset);

	i = 0;

	msgbuf[i++] = offset >> 8;
	msgbuf[i++] = offset;

	msg[0].addr = Wimax730_i2c_client->addr;
	msg[0].buf = msgbuf;
	msg[0].len = i;

	msg[1].addr = Wimax730_i2c_client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = rxData;
	msg[1].len = length;

	status = i2c_transfer(Wimax730_i2c_client->adapter, msg, 2);

	if (status == 2)
		return length;
	else if (status >= 0)
		return -EIO;
	else
		return status;


}

int Wimax730_write(int offset, char *txData, int length)
{
	struct i2c_msg msg;
	unsigned char buf[130] = {0};
	int i = 0;
	
	msg.addr = Wimax730_i2c_client->addr;
	msg.flags = 0;

	/* msg.buf is u8 and casts will mask the values */
	msg.buf = buf;
	msg.buf[i++] = offset >> 8;
	msg.buf[i++] = offset;
	memcpy(&msg.buf[i], txData, length);
	msg.len = i + length;

	if( i2c_transfer(Wimax730_i2c_client->adapter, &msg, 1) == 1) {
		return length;
	} else {
		printk("i2c_transfer fail\n");
		return -EIO;
	}	
}

static int Wimax730_probe(struct i2c_adapter *adap, int addr, int kind)
{
	struct i2c_client *c;
	int ret;
	c = kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	memset(c, 0, sizeof(struct i2c_client));	

	strncpy(c->name, Wimax730_driver.driver.name, I2C_NAME_SIZE);
	c->addr = addr;
	c->adapter = adap;
	c->driver = &Wimax730_driver;

	Wimax730_i2c_client = c;

error:
	return ret;
}

#if 0
static int Wimax730_attach_adapter(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &Wimax730_addr_data, Wimax730_attach);
}
#endif

static int Wimax730_remove(struct i2c_client *client)
{
	i2c_set_clientdata(client, NULL);
	return 0;
}


static int Wimax730_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	return 0;
}

static struct i2c_driver Wimax730_driver = {
	.driver = {
		.name = "Wimax730",
	},
//	.id = EEPROM_I2C_ADDR, 
	//.attach_adapter = Wimax730_attach_adapter,
	//.detach_client = Wimax730_detach_client,
	.probe = Wimax730_probe,
	.remove = Wimax730_remove,
	.command = Wimax730_command
};

int Wimax730_init(void)
{
	int ret;
	ret = i2c_add_driver(&Wimax730_driver);
	return ret;
}

void Wimax730_exit(void)
{
	i2c_del_driver(&Wimax730_driver);
}


