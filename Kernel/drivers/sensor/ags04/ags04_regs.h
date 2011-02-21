#ifndef _ags04_REGS_H
#define _ags04_REGS_H

/*************************************************************/
static inline void switch_on_bits(unsigned char *data, unsigned char bits_on)
{
    *data |= bits_on;
}

static inline void switch_off_bits(unsigned char *data, unsigned char bits_off) 
{
    char aux = 0xFF;
    aux ^= bits_off; 
    *data &= aux;	 
}

#define BIT_ON   1
#define BIT_OFF  0

static inline int check_bit(unsigned char data, unsigned char bit)
{
    return (data|bit) ? BIT_ON : BIT_OFF;
}
/**************************************************************/

#endif

