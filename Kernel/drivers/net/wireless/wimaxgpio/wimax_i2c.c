#include "wimax_i2c.h"

#include <linux/delay.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/vmalloc.h>

#if defined (CONFIG_MACH_QUATTRO) //cky 20100224
#include <mach/instinctq.h>	//cky 20100111 headers for quattro
#elif defined (CONFIG_MACH_ARIES)
#include <mach/gpio-jupiter.h>	// Yongha for Victory WiMAX 20100208
#endif

#include "wimaxgpio.h"
#include "firmware.h"
#include "../wimax/wimax_plat.h"

int Wimax730_init(void);
void Wimax730_exit(void);
int Wimax730_read(int offset, char *rxData, int length);
int Wimax730_write(int offset, char *rxData, int length);

void switch_eeprom_ap(void);	//cky 20100527

#if 1	//cky 20100527 rev08 fix
extern unsigned int system_rev;
unsigned int EEPROM_SCL = 0;
unsigned int EEPROM_SDA = 0;
#else
//extern int hw_rev_adc;	//cky 20100510 for HW rev checking
#endif

#define	WIMAX_BOOTIMAGE_PATH "/system/etc/wimax_boot.bin"
#define	MAX_WIMAXBOOTIMAGE_SIZE	6000	// 6k bytes
#define	MAX_BOOT_WRITE_LENGTH			128
#define	WIMAX_BOOT_OVER128_CHECK
#define dim(x)		( sizeof(x)/sizeof(x[0]) )

typedef struct _IMAGE_DATA_Tag {	
	unsigned int    uiSize;
	unsigned int    uiWorkAddress;
	unsigned int 	uiOffset;
	struct mutex	lock;	// sangam : For i2c read write lock
	unsigned char * pImage;
} IMAGE_DATA, *PIMAGE_DATA;

IMAGE_DATA g_stWiMAXBootImage;

// I2C
void WiMAX_I2C_WriteByte(char c, int len)
{
	int i;
	char level;

	// 8 bits
	for (i = 0; i < len; i++)
	{
		gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
		
		level = (c >> (len - i - 1)) & 0x1;
		s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
		gpio_set_value(EEPROM_SDA, level);
		
		udelay(1);
		gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
		udelay(2);
	}
}

char WiMAX_I2C_ReadByte(int len)
{
	char val = 0;
	int j;

	// 8 bits
	for (j = 0; j < len; j++)
	{
		// read data
		gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
		s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(0));
		udelay(1);
		gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
		udelay(2);
		val |= (gpio_get_value(EEPROM_SDA) << (len - j - 1));
	}

	return val;
}

void WiMAX_I2C_Init(void)
{
	int DEVICE_ADDRESS = 0x50;
	
	// initial
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);
	udelay(2);

	// start bit
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_LOW);
	udelay(2);

	WiMAX_I2C_WriteByte(DEVICE_ADDRESS, 7);	// send 7 bits address
}

void WiMAX_I2C_Deinit(void)
{
	// stop
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	udelay(1);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_LOW);
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(1);
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);

	mdelay(10);
	
}

void WiMAX_I2C_Ack(void)
{
	// ack
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	udelay(1);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_LOW);
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(1);
}

void WiMAX_I2C_NoAck(void)
{
	// no ack
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	udelay(1);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(1);
}

int WiMAX_I2C_CheckAck(void)
{
	char val;
	
	// ack
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(0));
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(2);
	val = gpio_get_value(EEPROM_SDA);

	if (val == 1)
	{
		DumpDebug("EEPROM ERROR: NO ACK!!");
		return -1;
	}

	return 0;
}

void WiMAX_I2C_WriteCmd(void)
{
	// write cmd
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_LOW);	// 0
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(2);

	WiMAX_I2C_CheckAck();
}

void WiMAX_I2C_ReadCmd(void)
{
	// read cmd
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	gpio_set_value(EEPROM_SDA, GPIO_LEVEL_HIGH);	// 1
	udelay(1);
	gpio_set_value(EEPROM_SCL, GPIO_LEVEL_HIGH);
	udelay(2);

	WiMAX_I2C_CheckAck();
}

void WiMAX_I2C_EEPROMAddress(short addr)
{
	char buf[2] = {0};

	buf[0] = addr & 0xff;
	buf[1] = (addr >> 8) & 0xff;

	// send 2 bytes mem address
	WiMAX_I2C_WriteByte(buf[1], 8);
	WiMAX_I2C_CheckAck();

	WiMAX_I2C_WriteByte(buf[0], 8);
	WiMAX_I2C_CheckAck();
}

void WiMAX_I2C_WriteBuffer(char *data, int len)
{
	int i;

	for(i = 0; i < len; i++)
	{
		WiMAX_I2C_WriteByte(data[i], 8);
		WiMAX_I2C_CheckAck();
	}
}

void WiMAX_EEPROM_Write(unsigned short addr, unsigned char *data, int length)
{
	// write buffer
	WiMAX_I2C_Init();
	WiMAX_I2C_WriteCmd();
	WiMAX_I2C_EEPROMAddress(addr);
	WiMAX_I2C_WriteBuffer(data, length);
	WiMAX_I2C_Deinit();
}

void WiMAX_EEPROM_Read(unsigned short addr, unsigned char *data, int length)
{
	int i = 0;
	
	// read buffer
	WiMAX_I2C_Init();
	WiMAX_I2C_WriteCmd();
	WiMAX_I2C_EEPROMAddress(addr);

	WiMAX_I2C_Init();
	WiMAX_I2C_ReadCmd();
	for (i = 0; i < length; i++)
	{
		*(data + i) = WiMAX_I2C_ReadByte(8);	// 1 byte data

		if ((i + 1) == length) WiMAX_I2C_NoAck();	// no ack final data
		else WiMAX_I2C_Ack();
	}
	WiMAX_I2C_Deinit();
}

int LoadWiMaxBootImage(void)
{	
	//unsigned long dwImgSize;		
	struct file *fp;	
	int read_size = 0;

	fp = klib_fopen(WIMAX_BOOTIMAGE_PATH, O_RDONLY, 0);

	if(fp)
	{
		DumpDebug("LoadWiMAXBootImage ..");
		g_stWiMAXBootImage.pImage = (char *) vmalloc(MAX_WIMAXBOOTIMAGE_SIZE);//__get_free_pages(GFP_KERNEL, buforder);
		if(!g_stWiMAXBootImage.pImage)
		{
			DumpDebug("Error: Memory alloc failure.");
			klib_fclose(fp);
			return -1;
		}

		memset(g_stWiMAXBootImage.pImage, 0, MAX_WIMAXBOOTIMAGE_SIZE);				
		read_size = klib_flen_fcopy(g_stWiMAXBootImage.pImage, MAX_WIMAXBOOTIMAGE_SIZE, fp);
//		read_size = klib_fread(g_stWiMAXBootImage.pImage, dwImgSize, fp);
		g_stWiMAXBootImage.uiSize = read_size;
		g_stWiMAXBootImage.uiWorkAddress = 0;			
		g_stWiMAXBootImage.uiOffset = 0;
		mutex_init(&g_stWiMAXBootImage.lock);

		klib_fclose(fp);
	}
	else {
		DumpDebug("Error: WiMAX image file open failed");
		return -1;
	}
	return 0;
}

void WriteRev(void)
{
	//char buf[256] = {0};
	//int len = 32;
	//int i = 0;
	int val = 0;
	
	// HW rev check
	if (system_rev == 0)	// rev00
	{
		val = 0x00;
	}
	else if (system_rev == 1)	// rev01
	{
		val = 0x01;
	}
	else if (system_rev == 2)	// rev02, rev03, rev04, /* rev05 */
	{
		val = 0x03;
	}
	else if (system_rev == 6)	// rev06 -> rev05
	{
		val = 0x05;
	}
	else if (system_rev >= 7)	// rev07 & rev08 & rev09 & rev10
	{
		val = system_rev;	//cky 20100527 rev09 & rev10 use same rev07 firmware
	}
	else
	{
		DumpDebug("Unknown system_rev value: %d", system_rev);
		val = 0x0;
	}
	DumpDebug("HW REV: %d (%d)", val, system_rev);

	// swap
	val = be32_to_cpu(val);
	
	WiMAX_EEPROM_Write(0x7080, (char *)(&val), 4);
	
	//WiMAX_EEPROM_Read(0x7064, buf, len);
	//for (i = 0; i < len; i++)
	//{
	//	DumpDebug("0x%02x", buf[i]);
	//}
}

int CheckCert(void)
{
	char buf[256] = {0};
	int len = 4;
	int i = 0;

	WiMAX_EEPROM_Read(0x5800, buf, len);

	if (buf[0] == 0x43 && buf[1] == 0x65 && buf[2] == 0x72 && buf[3] == 0x74)	// Cert
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int CheckCal(void)
{
	char buf[256] = {0};
	int len = 8;
	int i = 0;

	WiMAX_EEPROM_Read(0x5400, buf, len);

	if (buf[0] == 0xff && buf[1] == 0xff && buf[2] == 0xff && buf[3] == 0xff
		&& buf[4] == 0xff	&& buf[5] == 0xff && buf[6] == 0xff && buf[7] == 0xff)
	{
		return -1;
	}
	
	return 0;
}

void WiMAX_EEPROM_Fix(void)
{
        unsigned short ucsize=MAX_BOOT_WRITE_LENGTH;
        unsigned char *buffer;
        char buf[16] = {0};
        int len = 16;
        int i;

        //[WGPIO] EEPROM dump [0x0000] = 
        //[WiMAX] 0a 00 00 ea 42 f2 a0 e3 82 f2 a0 e3 c2 f2 a0 e3
        //[WiMAX] d8 f0 9f e5 fe ff ff ea d4 f0 9f e5 d4 f0 9f e5
        char boot_image[32] = { 0x0a, 0x00, 0x00, 0xea, 0x42, 0xf2, 0xa0, 0xe3, 0x82, 0xf2, 0xa0, 0xe3, 0xc2, 0xf2, 0xa0, 0xe3,
                                                        0xd8, 0xf0, 0x9f, 0xe5, 0xfe, 0xff, 0xff, 0xea, 0xd4, 0xf0, 0x9f, 0xe5, 0xd4, 0xf0, 0x9f, 0xe5};

        int is_eeprom_error = 0;

        mutex_init(&g_stWiMAXBootImage.lock);

        // set SCL, SDA pin
        if (system_rev < 8)
        {
		EEPROM_SCL = GPIO_AP_SCL_18V;
                EEPROM_SDA = GPIO_AP_SDA_18V;
        }
        else
        {
                EEPROM_SCL = GPIO_AP_SDA_18V;   // rev08 fix
                EEPROM_SDA = GPIO_AP_SCL_18V;   // rev08 fix
        }

        // power on
        s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
        gpio_set_value(WIMAX_EN, GPIO_LEVEL_HIGH);
        mdelay(100);

        switch_eeprom_ap();

        s3c_gpio_cfgpin(EEPROM_SCL, S3C_GPIO_SFN(1));
        s3c_gpio_setpull(EEPROM_SCL, S3C_GPIO_PULL_NONE);
        s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
        s3c_gpio_setpull(EEPROM_SDA, S3C_GPIO_PULL_NONE);

        mutex_lock(&g_stWiMAXBootImage.lock);
#if 1
        WiMAX_EEPROM_Read(0, buf, len);
        for (i = 0; i < 16; i++)
        {
                if (!(buf[i] == 0xff || buf[i] == boot_image[i]))
                {
                        is_eeprom_error = 1;
                        break;
                }
        }
#else
        is_eeprom_error = 1;
#endif

        if (is_eeprom_error)
        {
                if(LoadWiMaxBootImage() < 0)
                {
                        DumpDebug("ERROR READ WIMAX BOOT IMAGE");
                        return;
                }
		g_stWiMAXBootImage.uiOffset = 0;

                while(g_stWiMAXBootImage.uiSize > g_stWiMAXBootImage.uiOffset)
                {
                        buffer =(unsigned char *)(g_stWiMAXBootImage.pImage+g_stWiMAXBootImage.uiOffset);
                        ucsize=MAX_BOOT_WRITE_LENGTH;

                        // write buffer
                        WiMAX_EEPROM_Write((unsigned short)g_stWiMAXBootImage.uiOffset, buffer, ucsize);

                        g_stWiMAXBootImage.uiOffset += MAX_BOOT_WRITE_LENGTH;

                        if((g_stWiMAXBootImage.uiSize - g_stWiMAXBootImage.uiOffset) < MAX_BOOT_WRITE_LENGTH)
                        {
                                buffer = (unsigned char *)(g_stWiMAXBootImage.pImage+g_stWiMAXBootImage.uiOffset);
                                ucsize = g_stWiMAXBootImage.uiSize - g_stWiMAXBootImage.uiOffset;
				  // write buffer
                                WiMAX_EEPROM_Write((unsigned short)g_stWiMAXBootImage.uiOffset, buffer, ucsize);

                                g_stWiMAXBootImage.uiOffset += MAX_BOOT_WRITE_LENGTH;
                        }
                }

        }
        mutex_unlock(&g_stWiMAXBootImage.lock);

        // power off
        s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
        gpio_set_value(WIMAX_EN, GPIO_LEVEL_LOW);

        mutex_destroy(&g_stWiMAXBootImage.lock);

}

void WiMAX_ReadBoot()
{
	short addr = 0;
	char buf[4096] = {0};
	int i, j;
	int len;
	unsigned int checksum = 0;
	
	mutex_init(&g_stWiMAXBootImage.lock);
	
	// set SCL, SDA pin
	if (system_rev < 8)
	{
		EEPROM_SCL = GPIO_AP_SCL_18V;
		EEPROM_SDA = GPIO_AP_SDA_18V;
	}
	else
	{
		EEPROM_SCL = GPIO_AP_SDA_18V;	// rev08 fix
		EEPROM_SDA = GPIO_AP_SCL_18V;	// rev08 fix
	}
	
	// power on
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_HIGH);
	mdelay(100);

	switch_eeprom_ap();
	
	s3c_gpio_cfgpin(EEPROM_SCL, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SCL, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SDA, S3C_GPIO_PULL_NONE);

	mutex_lock(&g_stWiMAXBootImage.lock);

	addr = 0x0;
	for (j = 0; j < 1; j++)	// read 4K
	{
		len = 4096;
		WiMAX_EEPROM_Read(addr, buf, len);
		{
			char print_buf[256] = {0};
			char chr[8] = {0};
			
			// dump packets
			unsigned char  *b = (unsigned char *)buf;
			DumpDebug("EEPROM dump [0x%04x] = ", addr);

			for (i = 0; i < len; i++) {
				checksum += b[i];
				sprintf(chr, " %02x", b[i]);
				strcat(print_buf, chr);
				if (((i + 1) != len) && (i % 16 == 15))
				{
					DumpDebug(print_buf);
					memset(print_buf, 0x0, 256);
				}
			}
			DumpDebug(print_buf);
		}
		addr += len;
	}

	DumpDebug("Checksum: 0x%08x", checksum);
	mutex_unlock(&g_stWiMAXBootImage.lock);

	// power off
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_LOW);

	mutex_destroy(&g_stWiMAXBootImage.lock);
}

void WiMAX_ReadEEPROM()
{
	short addr = 0;
	char buf[4096] = {0};
	int i, j;
	int len;
	unsigned int checksum = 0;

	mutex_init(&g_stWiMAXBootImage.lock);
	
	// set SCL, SDA pin
	if (system_rev < 8)
	{
		EEPROM_SCL = GPIO_AP_SCL_18V;
		EEPROM_SDA = GPIO_AP_SDA_18V;
	}
	else
	{
		EEPROM_SCL = GPIO_AP_SDA_18V;	// rev08 fix
		EEPROM_SDA = GPIO_AP_SCL_18V;	// rev08 fix
	}
	
	// power on
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_HIGH);
	mdelay(100);

	switch_eeprom_ap();
	
	s3c_gpio_cfgpin(EEPROM_SCL, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SCL, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SDA, S3C_GPIO_PULL_NONE);

	mutex_lock(&g_stWiMAXBootImage.lock);

	addr = 0x0;
	for (j = 0; j < 16; j++)	// read 64K
	{
		len = 4096;
		WiMAX_EEPROM_Read(addr, buf, len);
		{
			char print_buf[256] = {0};
			char chr[8] = {0};
			
			// dump packets
			unsigned char  *b = (unsigned char *)buf;
			DumpDebug("EEPROM dump [0x%04x] = ", addr);

			for (i = 0; i < len; i++) {
				checksum += b[i];
				sprintf(chr, " %02x", b[i]);
				strcat(print_buf, chr);
				if (((i + 1) != len) && (i % 16 == 15))
				{
					DumpDebug(print_buf);
					memset(print_buf, 0x0, 256);
				}
			}
			DumpDebug(print_buf);
		}
		addr += len;
	}

	DumpDebug("Checksum: 0x%08x", checksum);
	mutex_unlock(&g_stWiMAXBootImage.lock);

	// power off
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_LOW);

	mutex_destroy(&g_stWiMAXBootImage.lock);
}

void WiMAX_RraseEEPROM()
{
	short addr = 0;
	char buf[128] = {0};
	int i;
	
	mutex_init(&g_stWiMAXBootImage.lock);
	
	// set SCL, SDA pin
	if (system_rev < 8)
	{
		EEPROM_SCL = GPIO_AP_SCL_18V;
		EEPROM_SDA = GPIO_AP_SDA_18V;
	}
	else
	{
		EEPROM_SCL = GPIO_AP_SDA_18V;	// rev08 fix
		EEPROM_SDA = GPIO_AP_SCL_18V;	// rev08 fix
	}
	
	// power on
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_HIGH);
	mdelay(100);

	switch_eeprom_ap();
	
	s3c_gpio_cfgpin(EEPROM_SCL, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SCL, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SDA, S3C_GPIO_PULL_NONE);

	mutex_lock(&g_stWiMAXBootImage.lock);

	for (i = 0; i < 128; i++)
	{
		buf[i] = 0xff;
	}
	
	for (i = 0; i < 512; i++)	// clear all EEPROM
	{
		DumpDebug("ERASE [0x%04x]\n", i * 128);
		WiMAX_EEPROM_Write(128 * i, buf, 128);
	}

	mutex_unlock(&g_stWiMAXBootImage.lock);

	// power off
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_LOW);

	mutex_destroy(&g_stWiMAXBootImage.lock);
}

//
// Write boot image to EEPROM
//
#if defined (CONFIG_MACH_ARIES)	//cky 20100225
void WIMAX_BootInit(void)
{
	unsigned short ucsize=MAX_BOOT_WRITE_LENGTH;
	unsigned char *buffer;

	if(LoadWiMaxBootImage() < 0)
	{
		DumpDebug("ERROR READ WIMAX BOOT IMAGE");
		return 0;
	}

	// set SCL, SDA pin
	if (system_rev < 8)
	{
		EEPROM_SCL = GPIO_AP_SCL_18V;
		EEPROM_SDA = GPIO_AP_SDA_18V;
	}
	else
	{
		EEPROM_SCL = GPIO_AP_SDA_18V;	// rev08 fix
		EEPROM_SDA = GPIO_AP_SCL_18V;	// rev08 fix
	}
	
	// power on
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_HIGH);
	mdelay(100);

	switch_eeprom_ap();
	
	s3c_gpio_cfgpin(EEPROM_SCL, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SCL, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SDA, S3C_GPIO_PULL_NONE);
	
	g_stWiMAXBootImage.uiOffset = 0;
	mutex_lock(&g_stWiMAXBootImage.lock);
	
	while(g_stWiMAXBootImage.uiSize > g_stWiMAXBootImage.uiOffset)
	{
		buffer =(unsigned char *)(g_stWiMAXBootImage.pImage+g_stWiMAXBootImage.uiOffset);
		ucsize=MAX_BOOT_WRITE_LENGTH;

		// write buffer
		WiMAX_EEPROM_Write((unsigned short)g_stWiMAXBootImage.uiOffset, buffer, ucsize);
		
		g_stWiMAXBootImage.uiOffset += MAX_BOOT_WRITE_LENGTH;

		if((g_stWiMAXBootImage.uiSize - g_stWiMAXBootImage.uiOffset) < MAX_BOOT_WRITE_LENGTH)
		{
			buffer = (unsigned char *)(g_stWiMAXBootImage.pImage+g_stWiMAXBootImage.uiOffset);
			ucsize = g_stWiMAXBootImage.uiSize - g_stWiMAXBootImage.uiOffset;

			// write buffer
			WiMAX_EEPROM_Write((unsigned short)g_stWiMAXBootImage.uiOffset, buffer, ucsize);
			
			g_stWiMAXBootImage.uiOffset += MAX_BOOT_WRITE_LENGTH;				
		}
	}

	WriteRev();	// write hw rev to eeprom
	
	mutex_unlock(&g_stWiMAXBootImage.lock);

	// power off
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_LOW);

	mutex_destroy(&g_stWiMAXBootImage.lock);
	
	DumpDebug("EEPROM WRITING DONE.");

}

void WIMAX_Write_Rev(void)
{
	mutex_init(&g_stWiMAXBootImage.lock);
	
	// set SCL, SDA pin
	if (system_rev < 8)
	{
		EEPROM_SCL = GPIO_AP_SCL_18V;
		EEPROM_SDA = GPIO_AP_SDA_18V;
	}
	else
	{
		EEPROM_SCL = GPIO_AP_SDA_18V;	// rev08 fix
		EEPROM_SDA = GPIO_AP_SCL_18V;	// rev08 fix
	}
	
	// power on
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_HIGH);
	mdelay(100);

	switch_eeprom_ap();
	
	s3c_gpio_cfgpin(EEPROM_SCL, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SCL, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SDA, S3C_GPIO_PULL_NONE);

	mutex_lock(&g_stWiMAXBootImage.lock);

	WriteRev();	// write hw rev to eeprom

	mutex_unlock(&g_stWiMAXBootImage.lock);

	// power off
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_LOW);

	mutex_destroy(&g_stWiMAXBootImage.lock);
	
	DumpDebug("REV WRITING DONE.");
}

int WIMAX_Check_Cert(void)
{
	int ret;
	
	mutex_init(&g_stWiMAXBootImage.lock);
	
	// set SCL, SDA pin
	if (system_rev < 8)
	{
		EEPROM_SCL = GPIO_AP_SCL_18V;
		EEPROM_SDA = GPIO_AP_SDA_18V;
	}
	else
	{
		EEPROM_SCL = GPIO_AP_SDA_18V;	// rev08 fix
		EEPROM_SDA = GPIO_AP_SCL_18V;	// rev08 fix
	}
	
	// power on
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_HIGH);
	mdelay(100);

	switch_eeprom_ap();
	
	s3c_gpio_cfgpin(EEPROM_SCL, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SCL, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SDA, S3C_GPIO_PULL_NONE);

	mutex_lock(&g_stWiMAXBootImage.lock);

	ret = CheckCert();	// check cert

	mutex_unlock(&g_stWiMAXBootImage.lock);

	// power off
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_LOW);

	mutex_destroy(&g_stWiMAXBootImage.lock);
	
	DumpDebug("CHECK CERT DONE. (%d)", ret);

	return ret;
}

int WIMAX_Check_Cal(void)
{
	int ret;
	
	mutex_init(&g_stWiMAXBootImage.lock);
	
	// set SCL, SDA pin
	if (system_rev < 8)
	{
		EEPROM_SCL = GPIO_AP_SCL_18V;
		EEPROM_SDA = GPIO_AP_SDA_18V;
	}
	else
	{
		EEPROM_SCL = GPIO_AP_SDA_18V;	// rev08 fix
		EEPROM_SDA = GPIO_AP_SCL_18V;	// rev08 fix
	}
	
	// power on
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_HIGH);
	mdelay(100);

	switch_eeprom_ap();
	
	s3c_gpio_cfgpin(EEPROM_SCL, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SCL, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(EEPROM_SDA, S3C_GPIO_SFN(1));
	s3c_gpio_setpull(EEPROM_SDA, S3C_GPIO_PULL_NONE);

	mutex_lock(&g_stWiMAXBootImage.lock);

	ret = CheckCal();	// check cert

	mutex_unlock(&g_stWiMAXBootImage.lock);

	// power off
	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_LOW);

	mutex_destroy(&g_stWiMAXBootImage.lock);
	
	DumpDebug("CHECK CAL DONE. (%d)", ret);

	return ret;
}

#else
void WIMAX_BootInit(void)
{
	int ret = 0;
	unsigned char ucReadBuffer[MAX_BOOT_WRITE_LENGTH];
	unsigned char ucBootCmp[MAX_BOOT_WRITE_LENGTH] = 
		            {0x0A,0x00,0x00,0xEA,0x42,0xF2,0xA0,0xE3,0x82,0xF2,0xA0,0xE3,0xC2,0xF2,0xA0,0xE3};

#ifdef WIMAX_BOOT_OVER128_CHECK
	unsigned char ucReadBuffer_over128[MAX_BOOT_WRITE_LENGTH];
	unsigned char ucBootCmp_over128[MAX_BOOT_WRITE_LENGTH] = 
					{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
#endif

	unsigned char ucWriteBuffer[MAX_BOOT_WRITE_LENGTH];
	unsigned char *buffer;
	unsigned short ucsize=MAX_BOOT_WRITE_LENGTH;

	s3c_gpio_cfgpin(WIMAX_EN, S3C_GPIO_SFN(1));
	//temp s3c_gpio_cfgpin(GPIO_WIMAX_I2C_CON, S3C_GPIO_SFN(1));

	gpio_set_value(WIMAX_EN, GPIO_LEVEL_HIGH);
	mdelay(10);
	//temp gpio_set_value(GPIO_WIMAX_I2C_CON, GPIO_LEVEL_HIGH);
	mdelay(300);

	Wimax730_init();

	Wimax730_read(0x0, ucReadBuffer, 16); // offset, buffer, len

#ifdef WIMAX_BOOT_OVER128_CHECK
	memset( ucReadBuffer_over128, 0, dim(ucReadBuffer_over128) );
	Wimax730_read( 0x80, ucReadBuffer_over128, 16 );
#endif

	// if boot was already written , pass the write process
	if( strncmp( ucReadBuffer, ucBootCmp, 16 ) == 0 )
	{
#if !defined(WIMAX_BOOT_OVER128_CHECK) // org
		ret = 1;
		goto exit;
#else
//		ddd( ucReadBuffer_over128, 16, "READBUFFER_128" );
		if( strncmp( ucReadBuffer_over128, ucBootCmp_over128, 16 ) == 0 )
		{
			DumpDebug("Error first 128 bytes have valid values, but the values over 128 have invalid - all FFs");
			// fall throught..
		}
		else
		{
			DumpDebug("EEPROM has valid boot code");
			// EEPROM has valid boot code
			ret = 1;
			goto exit;
		}
#endif
	}

	if(LoadWiMaxBootImage() < 0)
	{
		ret = 1;
		goto exit;
	}

	g_stWiMAXBootImage.uiOffset = 0;
	mutex_lock(&g_stWiMAXBootImage.lock);
	
	while(g_stWiMAXBootImage.uiSize > g_stWiMAXBootImage.uiOffset)
	{
		buffer =(unsigned char *)(g_stWiMAXBootImage.pImage+g_stWiMAXBootImage.uiOffset);
		ucsize=MAX_BOOT_WRITE_LENGTH;

		ret = Wimax730_write((unsigned short)g_stWiMAXBootImage.uiOffset, buffer, ucsize);
		if(ret <= 0) 
			break;		

		g_stWiMAXBootImage.uiOffset += MAX_BOOT_WRITE_LENGTH;
		//clk_busy_wait(2000);
		mdelay(2);

		if((g_stWiMAXBootImage.uiSize - g_stWiMAXBootImage.uiOffset) < MAX_BOOT_WRITE_LENGTH)
		{
		//	clk_busy_wait(2000);
			mdelay(2);
			buffer = (unsigned char *)(g_stWiMAXBootImage.pImage+g_stWiMAXBootImage.uiOffset);
			ucsize = g_stWiMAXBootImage.uiSize - g_stWiMAXBootImage.uiOffset;

			ret = Wimax730_write((unsigned short)g_stWiMAXBootImage.uiOffset, buffer, ucsize);
			if(ret <= 0)
				break;
			
			g_stWiMAXBootImage.uiOffset += MAX_BOOT_WRITE_LENGTH;				
		}
	}

	mutex_unlock(&g_stWiMAXBootImage.lock);
exit:
	gpio_set_value(WIMAX_EN, GPIO_LEVEL_LOW);
	//temp gpio_set_value(GPIO_WIMAX_I2C_CON, GPIO_LEVEL_LOW);

	Wimax730_exit()	;	
	return;
	
}
#endif



