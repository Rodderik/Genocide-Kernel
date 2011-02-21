#ifndef __WIMAX_I2C_H__
#define __WIMAX_I2C_H__

#if 0
void WiMAX_I2C_WriteByte(char c, int len);
char WiMAX_I2C_ReadByte(int len);
void WiMAX_I2C_Init(void);
void WiMAX_I2C_Deinit(void);
void WiMAX_I2C_Ack(void);
void WiMAX_I2C_NoAck(void);
int WiMAX_I2C_CheckAck(void);
void WiMAX_I2C_WriteCmd(void);
void WiMAX_I2C_ReadCmd(void);
void WiMAX_I2C_EEPROMAddress(short addr);
void WiMAX_I2C_WriteBuffer(char *data, int len);
#endif

// export function
void WIMAX_BootInit(void);
void WIMAX_Write_Rev(void);
int WIMAX_Check_Cert(void);
int WIMAX_Check_Cal(void);
void WiMAX_EEPROM_Fix(void);
void WiMAX_ReadBoot(void);
void WiMAX_ReadEEPROM(void);
void WiMAX_RraseEEPROM(void);

#endif	//__WIMAX_I2C_H__
