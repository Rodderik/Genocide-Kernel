#ifndef _ags04_MAIN_H
#define _ags04_MAIN_H

#define GPIO_LEVEL_LOW      0
#define GPIO_LEVEL_HIGH     1

#define GRIP_IRQ           IRQ_EINT(29)
#define GRIP_I2C_EN     GPIO_GRIP_I2C_EN

// SERI:OK: setting logic to more possitive one
#define NOT_DETECTED    0// 1
#define CS1_DETECTED       1 // 0
#define CS3_DETECTED    2
#define ALL_DETECTED 3

extern void ags04_enable_int( void ) ;
extern void ags04_disable_int( void ) ;

extern int read_count(int* rnd, int* ch1, int* ch3 );

#endif

