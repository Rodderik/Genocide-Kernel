#include <linux/delay.h>
#include <linux/i2c.h>

/* Slave address */
#define MAX17040_SLAVE_ADDR	0x6D

/* Register address */
#define VCELL0_REG			0x02
#define VCELL1_REG			0x03
#define SOC0_REG			0x04
#define SOC1_REG			0x05
#define MODE0_REG			0x06
#define MODE1_REG			0x07
#define RCOMP0_REG			0x0C
#define RCOMP1_REG			0x0D
#define CMD0_REG			0xFE
#define CMD1_REG			0xFF

#define __ADVANCED_SOC_VALUE__	// hanapark

static struct i2c_driver fg_i2c_driver;
static struct i2c_client *fg_i2c_client = NULL;


#if defined(CONFIG_ARIES_NTT)
unsigned int FGPureSOC = 0;
unsigned int prevFGSOC = 0;
unsigned int fg_zero_count = 0;
#endif

int fuel_guage_init = 0;
EXPORT_SYMBOL(fuel_guage_init);

#if (defined CONFIG_ARIES_VER_B0 ) || (defined CONFIG_ARIES_VER_B1) || (defined CONFIG_ARIES_VER_B2) || (defined CONFIG_ARIES_VER_B3)
unsigned short fg_ignore[] = {I2C_CLIENT_END };
static unsigned short fg_normal_i2c[] = {I2C_CLIENT_END };
static unsigned short fg_probe[] = { 14, (MAX17040_SLAVE_ADDR >> 1), I2C_CLIENT_END }; // hanapark_Victory : change ID 9->14 (mach-jupiter.c)
#else
static unsigned short fg_normal_i2c[] = { I2C_CLIENT_END };
//static unsigned short fg_normal_i2c[] = {MAX17040_SLAVE_ADDR>>1, I2C_CLIENT_END};
static unsigned short fg_ignore[] = { I2C_CLIENT_END };
static unsigned short fg_probe[] = { 4, (MAX17040_SLAVE_ADDR >> 1), I2C_CLIENT_END };
//static unsigned short fg_probe[] = {I2C_CLIENT_END};
#endif
static struct i2c_client_address_data fg_addr_data = {
	.normal_i2c	= fg_normal_i2c,
	.ignore		= fg_ignore,
	.probe		= fg_probe,
};


struct fg_state {
	struct i2c_client *client;
};


struct fg_state *fg_state;


static int is_reset_soc = 0;

static int fg_i2c_read(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret;
	u8 buf[1];
	struct i2c_msg msg[2];

	buf[0] = reg; 

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret != 2) 
		return -EIO;

	*data = buf[0];
	
	return 0;
}

static int fg_i2c_write(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret;
	u8 buf[3];
	struct i2c_msg msg[1];

	buf[0] = reg;
	buf[1] = *data;
	buf[2] = *(data + 1);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 3;
	msg[0].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret != 1) 
		return -EIO;

	return 0;
}

unsigned int fg_read_vcell(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
	u32 vcell = 0;

	if (fg_i2c_read(client, VCELL0_REG, &data[0]) < 0) {
		pr_err("%s: Failed to read VCELL0\n", __func__);
		return -1;
	}
	if (fg_i2c_read(client, VCELL1_REG, &data[1]) < 0) {
		pr_err("%s: Failed to read VCELL1\n", __func__);
		return -1;
	}
	vcell = ((((data[0] << 4) & 0xFF0) | ((data[1] >> 4) & 0xF)) * 125)/100;
	//pr_info("%s: VCELL=%d\n", __func__, vcell);
	return vcell;
}

unsigned int fg_read_soc(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 data[2];
#ifdef __ADVANCED_SOC_VALUE__	// hanapark
	unsigned int adj_soc = 100;
#endif

	if (fg_i2c_read(client, SOC0_REG, &data[0]) < 0) {
		pr_err("%s: Failed to read SOC0\n", __func__);
		return -1;
	}
	if (fg_i2c_read(client, SOC1_REG, &data[1]) < 0) {
		pr_err("%s: Failed to read SOC1\n", __func__);
		return -1;
	}

	if (is_reset_soc) {
		pr_info("%s: Reseting SOC\n", __func__);
		return -1;
	} else {
#ifndef __ADVANCED_SOC_VALUE__	// hanapark
		if (data[0])
			return data[0];
#else
		if (data[0])
		{
			adj_soc = (((data[0]*10) - 15) * 100) / (950 - 15);	// hanapark_Atlas
			if (adj_soc > 100)
				adj_soc = 100;
			else if (adj_soc < 0)
				adj_soc = 0;
			//pr_info("%s: Real SOC = %d, Adjusted SOC = %d\n", __func__, data[0], adj_soc);
			return adj_soc;
		}
		else	// 0%
			return data[0];
#endif	/* __ADVANCED_SOC_VALUE__ */
	}
}

unsigned int fg_reset_soc(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 rst_cmd[2];
	s32 ret = 0;

	is_reset_soc = 1;
	/* Quick-start */
	rst_cmd[0] = 0x40;
	rst_cmd[1] = 0x00;

	ret = fg_i2c_write(client, MODE0_REG, rst_cmd);
	if (ret)
		pr_info("%s: failed reset SOC(%d)\n", __func__, ret);

	msleep(500);
	is_reset_soc = 0;
	return ret;
}

void fuel_gauge_rcomp(void)
{
	struct i2c_client *client = fg_i2c_client;
	u8 rst_cmd[2];
	s32 ret = 0;

	rst_cmd[0] = 0xC7;	// hanapark_Victory
	rst_cmd[1] = 0x1F;	// hanapark_Victory (ATHD 4% -> 1%)

	ret = fg_i2c_write(client, RCOMP0_REG, rst_cmd);
	if (ret)
		pr_info("%s: failed fuel_gauge_rcomp(%d)\n", __func__, ret);
	
	//msleep(500);
}

static int fg_i2c_remove(struct i2c_client *client)
{
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;
	fg_i2c_client = client;
	return 0;
}

static int fg_i2c_probe(struct i2c_client *client,  const struct i2c_device_id *id)
{
	struct fg_state *fg;

	fg = kzalloc(sizeof(struct fg_state), GFP_KERNEL);
	if (fg == NULL) {		
		printk("failed to allocate memory \n");
		return -ENOMEM;
	}
	
	fg->client = client;
	i2c_set_clientdata(client, fg);
	
	/* rest of the initialisation goes here. */
	
	printk("Fuel guage attach success!!!\n");

	fg_i2c_client = client;

	fuel_guage_init = 1;

	return 0;
}

static const struct i2c_device_id fg_device_id[] = {
	{"fuelgauge", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, fg_device_id);

static struct i2c_driver fg_i2c_driver = {
	.driver = {
		.name = "Fuel Gauge I2C",
		.owner = THIS_MODULE,
	},
	.probe	= fg_i2c_probe,
	.remove	= fg_i2c_remove,
	.id_table	= fg_device_id,
};

int fg_init(void)
{
	int ret;

	ret = i2c_add_driver(&fg_i2c_driver);
	if (ret)
		pr_err("%s: Can't add fg i2c drv\n", __func__);

	return ret;
}

void fg_exit(void)
{
	i2c_del_driver(&fg_i2c_driver);
}

