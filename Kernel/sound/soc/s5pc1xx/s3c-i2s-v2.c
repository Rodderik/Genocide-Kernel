/* sound/soc/s3c24xx/s3c-i2c-v2.c
 *
 * ALSA Soc Audio Layer - I2S core for newer Samsung SoCs.
 *
 * Copyright (c) 2006 Wolfson Microelectronics PLC.
 *	Graeme Gregory graeme.gregory@wolfsonmicro.com
 *	linux@wolfsonmicro.com
 *
 * Copyright (c) 2008, 2007, 2004-2005 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/io.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <mach/dma.h>
#include <mach/regs-clock.h>
#include <mach/regs-audss.h>
#include "s3c-i2s-v2.h"
#include "s3c-dma.h"

#undef S3C_IIS_V2_SUPPORTED

#if defined(CONFIG_CPU_S3C2412) || defined(CONFIG_CPU_S3C2413)
#define S3C_IIS_V2_SUPPORTED
#endif

#if defined(CONFIG_PLAT_S3C64XX) || defined(CONFIG_PLAT_S5P)
#define S3C_IIS_V2_SUPPORTED
#endif
//#define pr_debug(fmt ...) printk(fmt) 
#define S3C_I2S_DEBUG_CON 0
static bool clk_en_dis_play  = 0;
static bool clk_en_dis_rec  = 0;
static bool value_saved = false;
static inline struct s3c_i2sv2_info *to_info(struct snd_soc_dai *cpu_dai)
{
	return cpu_dai->private_data;
}

#define bit_set(v, b) (((v) & (b)) ? 1 : 0)

#if S3C_I2S_DEBUG_CON
static void dbg_showcon(const char *fn, u32 con)
{
	printk(KERN_DEBUG "%s: LRI=%d, TXFEMPT=%d, RXFEMPT=%d, TXFFULL=%d, RXFFULL=%d\n", fn,
	       bit_set(con, S3C_IISCON_LRINDEX),
	       bit_set(con, S3C_IISCON_TXFIFO_EMPTY),
	       bit_set(con, S3C_IISCON_RXFIFO_EMPTY),
	       bit_set(con, S3C_IISCON_TXFIFO_FULL),
	       bit_set(con, S3C_IISCON_RXFIFO_FULL));

	printk(KERN_DEBUG "%s: PAUSE: TXDMA=%d, RXDMA=%d, TXCH=%d, RXCH=%d\n",
	       fn,
	       bit_set(con, S3C_IISCON_TXDMA_PAUSE),
	       bit_set(con, S3C_IISCON_RXDMA_PAUSE),
	       bit_set(con, S3C_IISCON_TXCH_PAUSE),
	       bit_set(con, S3C_IISCON_RXCH_PAUSE));
	printk(KERN_DEBUG "%s: ACTIVE: TXDMA=%d, RXDMA=%d, IIS=%d\n", fn,
	       bit_set(con, S3C_IISCON_TXDMA_ACTIVE),
	       bit_set(con, S3C_IISCON_RXDMA_ACTIVE),
	       bit_set(con, S3C_IISCON_IIS_ACTIVE));
}
#else
static inline void dbg_showcon(const char *fn, u32 con)
{
}
#endif

static int s3c_i2s_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
        u32 iiscon, iisfic;
        u32 retval;
	struct s3c_i2sv2_info *i2s = to_info(dai);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		pr_debug("Entering..%s..for PLAYBACK\n",__func__);
                if(!clk_en_dis_play && !clk_en_dis_rec)
                {
//Clk enabling should be in following sequence..with iis0_ip clk enabled IIS power will be ON
                retval = clk_enable(i2s->iis0_ip_clk);
                clk_enable(i2s->i2sbusclk);
                clk_enable(i2s->i2sclk);
                if(value_saved == true)
                {
                __raw_writel(i2s->audss_clksrc,S5P_CLKSRC_AUDSS);
                __raw_writel(i2s->audss_clkdiv,S5P_CLKDIV_AUDSS);
                __raw_writel(i2s->audss_clkgate,S5P_CLKGATE_AUDSS);
                writel(i2s->iiscon, i2s->regs + S3C_IISCON);
                writel(i2s->iismod, i2s->regs + S3C_IISMOD);
                writel(i2s->iisfic, i2s->regs + S3C_IISFIC);
                writel(i2s->iispsr, i2s->regs + S3C_IISPSR);
                value_saved = false;
                }
                pr_debug("..Enabled all clocks finally..\n");
                }
                clk_en_dis_play =1 ;
		iiscon = readl(i2s->regs + S3C_IISCON);
                if(iiscon & S3C_IISCON_TXDMACTIVE)
                        return 0;

                iisfic = readl(i2s->regs + S3C_IISFIC);
                iisfic |= S3C_IISFIC_TFLUSH;
                writel(iisfic, i2s->regs + S3C_IISFIC);

                do{
                   cpu_relax();
		}while((__raw_readl(i2s->regs + S3C_IISFIC) >> 8) & 0x7f);

                iisfic = readl(i2s->regs + S3C_IISFIC);
                iisfic &= ~S3C_IISFIC_TFLUSH;
                writel(iisfic, i2s->regs + S3C_IISFIC);
	}
	else{
		pr_debug("Entering..%s..for RECORD\n",__func__);
                if(!clk_en_dis_rec && !clk_en_dis_play)
                {
                pr_debug("..Enabling clock finally..\n");
                retval = clk_enable(i2s->iis0_ip_clk);
                clk_enable(i2s->i2sbusclk);
                clk_enable(i2s->i2sclk);
                if(value_saved == true)
                {
                __raw_writel(i2s->audss_clksrc,S5P_CLKSRC_AUDSS);
                __raw_writel(i2s->audss_clkdiv,S5P_CLKDIV_AUDSS);
                __raw_writel(i2s->audss_clkgate,S5P_CLKGATE_AUDSS);
                writel(i2s->iiscon, i2s->regs + S3C_IISCON);
                writel(i2s->iismod, i2s->regs + S3C_IISMOD);
                writel(i2s->iisfic, i2s->regs + S3C_IISFIC);
                writel(i2s->iispsr, i2s->regs + S3C_IISPSR);
                value_saved = false;
                }
                }

                clk_en_dis_rec = 1 ;

		iiscon = readl(i2s->regs + S3C_IISCON);
                if(iiscon & S3C_IISCON_RXDMACTIVE)
                        return 0;

                iisfic = readl(i2s->regs + S3C_IISFIC);
                iisfic |= S3C_IISFIC_RFLUSH;
                writel(iisfic, i2s->regs + S3C_IISFIC);

                do{
                   cpu_relax();
		}while((__raw_readl(i2s->regs + S3C_IISFIC) >> 0) & 0x7f);

                iisfic = readl(i2s->regs + S3C_IISFIC);
                iisfic &= ~S3C_IISFIC_RFLUSH;
                writel(iisfic, i2s->regs + S3C_IISFIC);
	}
	return 0;	
}

static void s3c_i2s_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);
        
	pr_debug("Entering..%s..\n",__func__);
	
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
                pr_debug("Entering..%s..for PLAYBACK\n",__func__);
                clk_en_dis_play = 0 ;
        }
        else{
                pr_debug("Entering..%s..for RECORD\n",__func__);
                if(readl(i2s->regs + S3C_IISCON) & (1<<26))
                {
                pr_debug("\n rx overflow int in %s.. \n",__func__);
                writel(readl(i2s->regs + S3C_IISCON) | (1<<26),i2s->regs + S3C_IISCON); //clear rxfifo overflow interrupt
		writel(readl(i2s->regs + S3C_IISFIC) | (1<<7) , i2s->regs + S3C_IISFIC); //flush rx
		}

		clk_en_dis_rec = 0;
        }

       if((!clk_en_dis_play)&&(!clk_en_dis_rec))
        {
        //dump_i2s_sub();
	//dump_i2s();
	i2s->iiscon = readl(i2s->regs + S3C_IISCON);
        i2s->iismod = readl(i2s->regs + S3C_IISMOD);
        i2s->iisfic = readl(i2s->regs + S3C_IISFIC);
        i2s->iispsr = readl(i2s->regs + S3C_IISPSR);
        i2s->audss_clksrc = __raw_readl(S5P_CLKSRC_AUDSS);
        i2s->audss_clkdiv = __raw_readl(S5P_CLKDIV_AUDSS);
        i2s->audss_clkgate = __raw_readl(S5P_CLKGATE_AUDSS);
        value_saved = true;
//Don't change the clock disabling sequence..With iis0_ip clk disabled power will be gated for IIS
	clk_disable(i2s->i2sbusclk);
        clk_disable(i2s->i2sclk);
        clk_disable(i2s->iis0_ip_clk);
        pr_debug("..Disbaled all clock finally..\n");
        }
}

static int s3c_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
                                  int clk_id, unsigned int freq, int dir)
{
        struct clk *clk; 
        struct s3c_i2sv2_info *i2s = to_info(cpu_dai);
        u32 iismod = readl(i2s->regs + S3C_IISMOD);
        pr_debug("....inside...%s..@..%d..\n",__func__,__LINE__);
	
	
	switch (clk_id) {
        case S3C_CLKSRC_PCLK: /* IIS-IP is Master and derives its clocks from PCLK */
                if(!i2s->master)
                        return -EINVAL;
                iismod &= ~S3C_IISMOD_IMSMASK;
                iismod |= clk_id;
                i2s->clk_rate = clk_get_rate(i2s->iis0_ip_clk);
                break;

#ifdef USE_CLKAUDIO
        case S3C_CLKSRC_CLKAUDIO: /* IIS-IP is Master and derives its clocks from I2SCLKD2 */
                if(!i2s->master)
                        return -EINVAL;
                iismod &= ~S3C_IISMOD_IMSMASK;
                iismod |= clk_id;
                clk = clk_get(NULL, EPLLCLK);
                if (IS_ERR(clk)) {
                        printk("failed to get %s\n", EPLLCLK);
                        return -EBUSY;
                }
                clk_disable(clk);
                switch (freq) {
                case 8000:
                case 16000:
                case 32000:
                case 48000:
                case 64000:
                case 96000:
                        clk_set_rate(clk, 49152000);
                        break;
		case 11025:
                case 22050:
                case 44100:
                case 88200:
                default:
                        clk_set_rate(clk, 67738000);
                        break;
                }
                clk_enable(clk);
                i2s->clk_rate = clk_get_rate(i2s->i2sclk);
                printk("Setting FOUTepll to %dHz\n", i2s->clk_rate);
                clk_put(clk);
                break;
#endif

        case S3C_CLKSRC_SLVPCLK: /* IIS-IP is Slave, but derives its clocks from PCLK */
        case S3C_CLKSRC_I2SEXT:  /* IIS-IP is Slave and derives its clocks from the Codec Chip */
                iismod &= ~S3C_IISMOD_IMSMASK;
                iismod |= clk_id;
                //Operation clock for I2S logic selected as Audio Bus Clock
                iismod |= S3C_IISMOD_OPPCLK;

                clk = clk_get(NULL, EPLLCLK);
                if (IS_ERR(clk)) {
                        printk("failed to get %s\n", EPLLCLK);
                        return -EBUSY;
                }
                clk_disable(clk);
                clk_set_rate(clk, 67738000);
                clk_enable(clk);
                clk_put(clk);

                break;

        case S3C_CDCLKSRC_INT:
                iismod &= ~S3C_IISMOD_CDCLKCON;
                break;

        case S3C_CDCLKSRC_EXT:
                iismod |= S3C_IISMOD_CDCLKCON;
                break;

        default:
                return -EINVAL;
        }

        writel(iismod, i2s->regs + S3C_IISMOD);

        return 0;
}


/* Turn on or off the transmission path. */
static void s3c_snd_txctrl(struct s3c_i2sv2_info *i2s, int on)
{
	void __iomem *regs = i2s->regs;
	u32 fic, con, mod;

	pr_debug("%s(%d)\n", __func__, on);

	fic = readl(regs + S3C_IISFIC);
	con = readl(regs + S3C_IISCON);
	mod = readl(regs + S3C_IISMOD);

	pr_debug("%s: IIS: CON=%x MOD=%x FIC=%x\n", __func__, con, mod, fic);

	if (on) {
		con |= S3C_IISCON_TXDMACTIVE | S3C_IISCON_I2SACTIVE;
		con &= ~S3C_IISCON_TXDMAPAUSE;
		con &= ~S3C_IISCON_TXCHPAUSE;
		writel(con, regs + S3C_IISCON);
	} else {
		/* Note, we do not have any indication that the FIFO problems
		 * tha the S3C2410/2440 had apply here, so we should be able
		 * to disable the DMA and TX without resetting the FIFOS.
		 */
		if(!(con & S3C_IISCON_RXDMACTIVE)) /* Stop only if RX not active */
                        con &= ~S3C_IISCON_I2SACTIVE;

		con |=  S3C_IISCON_TXDMAPAUSE;
		con &= ~S3C_IISCON_TXDMACTIVE;
		con |=  S3C_IISCON_TXCHPAUSE;
		writel(con, regs + S3C_IISCON);
	}

	dbg_showcon(__func__, con);
	pr_debug("%s: IIS: CON=%x MOD=%x FIC=%x\n", __func__, con, mod, fic);
}

static void s3c_snd_rxctrl(struct s3c_i2sv2_info *i2s, int on)
{
	void __iomem *regs = i2s->regs;
	u32 fic, con, mod;

	pr_debug("%s(%d)\n", __func__, on);

	fic = readl(regs + S3C_IISFIC);
	con = readl(regs + S3C_IISCON);
	mod = readl(regs + S3C_IISMOD);

	pr_debug("%s: IIS: CON=%x MOD=%x FIC=%x\n", __func__, con, mod, fic);

	if (on) {
		con |= S3C_IISCON_RXDMACTIVE | S3C_IISCON_I2SACTIVE;
		con &= ~S3C_IISCON_RXDMAPAUSE;
		con &= ~S3C_IISCON_RXCHPAUSE;
		writel(con, regs + S3C_IISCON);
	} else {
		/* See txctrl notes on FIFOs. */
		if(!(con & S3C_IISCON_TXDMACTIVE)) /* Stop only if TX not active */
                        con &= ~S3C_IISCON_I2SACTIVE;

		con &= ~S3C_IISCON_RXDMACTIVE;
		con |=  S3C_IISCON_RXDMAPAUSE;
		con |=  S3C_IISCON_RXCHPAUSE;
		writel(con, regs + S3C_IISCON);
	}

	pr_debug("%s: IIS: CON=%x MOD=%x FIC=%x\n", __func__, con, mod, fic);
}

#define msecs_to_loops(t) (loops_per_jiffy / 1000 * HZ * t)

/*
 * Wait for the LR signal to allow synchronisation to the L/R clock
 * from the codec. May only be needed for slave mode.
 */
static int s3c_snd_lrsync(struct s3c_i2sv2_info *i2s)
{
	u32 iiscon;
	unsigned long loops = 50;

	pr_debug("Entered %s\n", __func__);

	while (--loops) {
		iiscon = readl(i2s->regs + S3C_IISCON);
		if (iiscon & S3C_IISCON_LRI)
			break;
	
		udelay(100);	
	}

	if (!loops) {
		printk(KERN_ERR "%s: timeout\n", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}

/*
 * Set S3C I2S DAI format
 */
static int s3c_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
			       unsigned int fmt)
{
	struct s3c_i2sv2_info *i2s = to_info(cpu_dai);
	u32 iismod;

	pr_debug("Entered %s\n", __func__);

	iismod = readl(i2s->regs + S3C_IISMOD);
	pr_debug("hw_params r: IISMOD: %x \n", iismod);

#if defined(CONFIG_CPU_S3C2412) || defined(CONFIG_CPU_S3C2413)
#define IISMOD_MASTER_MASK S3C_IISMOD_MASTER_MASK
#define IISMOD_SLAVE S3C_IISMOD_SLAVE
#define IISMOD_MASTER S3C_IISMOD_MASTER_INTERNAL
#endif
//#if defined(CONFIG_PLAT_S3C64XX) || defined(CONFIG_PLAT_S5P)
/* From Rev1.1 datasheet, we have two master and two slave modes:
 * IMS[11:10]:
 *	00 = master mode, fed from PCLK
 *	01 = master mode, fed from CLKAUDIO
 *	10 = slave mode, using PCLK
 *	11 = slave mode, using I2SCLK
 */
#define IISMOD_MASTER_MASK (1 << 11)
#define IISMOD_SLAVE (1 << 11)
#define IISMOD_MASTER (0 << 11)

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		i2s->master = 0;
		iismod &= ~IISMOD_MASTER_MASK;
		iismod |= IISMOD_SLAVE;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		i2s->master = 1;
		iismod &= ~IISMOD_MASTER_MASK;
		iismod |= IISMOD_MASTER;
		break;
	default:
		pr_err("unknwon master/slave format\n");
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
        case SND_SOC_DAIFMT_I2S:
                iismod &= ~S3C_IISMOD_MSB;
                break;
        case SND_SOC_DAIFMT_LEFT_J:
                iismod |= S3C_IISMOD_MSB;
                break;
        case SND_SOC_DAIFMT_RIGHT_J:
                iismod |= S3C_IISMOD_LSB;
                break;
        default:
                return -EINVAL;
        }

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
        case SND_SOC_DAIFMT_NB_NF:
                iismod &= ~S3C_IISMOD_LRP;
                break;
        case SND_SOC_DAIFMT_NB_IF:
                iismod |= S3C_IISMOD_LRP;
                break;
        case SND_SOC_DAIFMT_IB_IF:
        case SND_SOC_DAIFMT_IB_NF:
        default:
                printk("Inv-combo(%d) not supported!\n", fmt & SND_SOC_DAIFMT_FORMAT_MASK);
                return -EINVAL;
        }

	writel(iismod, i2s->regs + S3C_IISMOD);
	pr_debug("hw_params w: IISMOD: %x \n", iismod);
	return 0;
}

static int s3c_i2s_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *socdai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai_link *dai = rtd->dai;
	struct s3c_i2sv2_info *i2s = to_info(dai->cpu_dai);
	u32 iismod;

	pr_debug("Entered %s\n", __func__);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dai->cpu_dai->dma_data = i2s->dma_playback;
	else
		dai->cpu_dai->dma_data = i2s->dma_capture;

	switch(params_channels(params)) {
        case 1:
                if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
                        i2s->dma_playback->dma_size = 2;
                else
                        i2s->dma_capture->dma_size = 2;
                break;
        case 2:
                break;
        case 4:
                break;
        case 6:
                break;
        default:
                break;
        }


	/* Working copies of register */
	iismod = readl(i2s->regs + S3C_IISMOD);
	pr_debug("%s: r: IISMOD: %x\n", __func__, iismod);
	
	iismod &= ~(S3C_IISMOD_BLCMASK | S3C_IISMOD_BLCPMASK);
	/* Sample size */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		/* 8 bit sample, 16fs BCLK */
		iismod |= (S3C_IISMOD_8BIT | S3C_IISMOD_16FS | S3C_IISMOD_P8BIT);
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		/* 16 bit sample, 32fs BCLK */
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		/* 24 bit sample, 48fs BCLK */
		iismod |= (S3C_IISMOD_24BIT | S3C_IISMOD_48FS | S3C_IISMOD_P24BIT);
		break;
	}

	/* Set the IISMOD[25:24](BLC_P) to same value */
//	iismod &= ~(S5P_IISMOD_BLCPMASK);

	writel(iismod, i2s->regs + S3C_IISMOD);
	pr_debug("%s: w: IISMOD: %x\n", __func__, iismod);
	return 0;
}


static int s3c_i2s_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
        struct snd_soc_pcm_runtime *rtd = substream->private_data;
        struct s3c_i2sv2_info *i2s = to_info(rtd->dai->cpu_dai);
        u32 iismod = readl(i2s->regs + S3C_IISMOD);

        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
                if((iismod & S3C_IISMOD_TXRMASK) == S3C_IISMOD_RX){
                        iismod &= ~S3C_IISMOD_TXRMASK;
                        iismod |= S3C_IISMOD_TXRX;
                }
        }else{
                if((iismod & S3C_IISMOD_TXRMASK) == S3C_IISMOD_TX){
                        iismod &= ~S3C_IISMOD_TXRMASK;
                        iismod |= S3C_IISMOD_TXRX;
                }
        }

        writel(iismod, i2s->regs + S3C_IISMOD);
	pr_debug("%s: w: IISMOD: %x\n", __func__, iismod);
        return 0;
}


static int s3c_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
			       struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct s3c_i2sv2_info *i2s = to_info(rtd->dai->cpu_dai);
	int capture = (substream->stream == SNDRV_PCM_STREAM_CAPTURE);
	unsigned long irqs;
	int ret = 0;

	pr_debug("Entered %s\n", __func__);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (!i2s->master) {
			ret = s3c_snd_lrsync(i2s);
			if (ret)
				goto exit_err;
		}

		local_irq_save(irqs);//sayanta need to check usage of this..necessary at all or not.

		if (capture)
			s3c_snd_rxctrl(i2s, 1);
		else
			s3c_snd_txctrl(i2s, 1);

		local_irq_restore(irqs);
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		local_irq_save(irqs);//sayanta need to check usage of this...necessary at all or not

		if (capture)
			s3c_snd_rxctrl(i2s, 0);
		else
			s3c_snd_txctrl(i2s, 0);

		local_irq_restore(irqs);
		break;
	default:
		ret = -EINVAL;
		break;
	}

exit_err:
	return ret;
}

static void init_i2s(struct s3c_i2sv2_info *i2s)
{
        u32 iiscon, iismod, iisahb;

        writel(S3C_IISCON_I2SACTIVE | S3C_IISCON_SWRESET, i2s->regs + S3C_IISCON);

        iiscon  = readl(i2s->regs + S3C_IISCON);
        iismod  = readl(i2s->regs + S3C_IISMOD);
        iisahb  = S3C_IISAHB_DMARLD | S3C_IISAHB_DISRLDINT;

        /* Enable all interrupts to find bugs */
        iiscon |= S3C_IISCON_FRXOFINTEN;
        iismod &= ~S3C_IISMOD_OPMSK;
        iismod |= S3C_IISMOD_OPCCO;

        iiscon &= ~S3C_IISCON_FTXSURINTEN;
        iiscon |= S3C_IISCON_FTXURINTEN;
        iismod &= ~S3C_IISMOD_TXSLP;
        iisahb = 0;
        

        writel(iisahb, i2s->regs + S3C_IISAHB);
        writel(iismod, i2s->regs + S3C_IISMOD);
        writel(iiscon, i2s->regs + S3C_IISCON);
}


/*
 * Set S3C Clock dividers
 */
static int s3c_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai,
				  int div_id, int div)
{
	struct s3c_i2sv2_info *i2s = to_info(cpu_dai);
	u32 reg;

	pr_debug("%s(%p, %d, %d)\n", __func__, cpu_dai, div_id, div);
	switch (div_id) {
        case S3C_DIV_MCLK:
                reg = readl(i2s->regs + S3C_IISMOD) & ~S3C_IISMOD_RFSMASK;
                switch(div) {
                case 256: div = S3C_IISMOD_256FS; break;
                case 512: div = S3C_IISMOD_512FS; break;
                case 384: div = S3C_IISMOD_384FS; break;
                case 768: div = S3C_IISMOD_768FS; break;
                default: return -EINVAL;
                }
                writel(reg | div, i2s->regs + S3C_IISMOD);
                break;
        case S3C_DIV_BCLK:
                reg = readl(i2s->regs + S3C_IISMOD) & ~S3C_IISMOD_BFSMASK;
                switch(div) {
                case 16: div = S3C_IISMOD_16FS; break;
                case 24: div = S3C_IISMOD_24FS; break;
                case 32: div = S3C_IISMOD_32FS; break;
                case 48: div = S3C_IISMOD_48FS; break;
                default: return -EINVAL;
                }
                writel(reg | div, i2s->regs + S3C_IISMOD);
                break;
        case S3C_DIV_PRESCALER:
                reg = readl(i2s->regs + S3C_IISPSR) & ~S3C_IISPSR_PSRAEN;
                writel(reg, i2s->regs + S3C_IISPSR);
                reg = readl(i2s->regs + S3C_IISPSR) & ~S3C_IISPSR_PSVALA;
                div &= 0x3f;
                writel(reg | (div<<8) | S3C_IISPSR_PSRAEN, i2s->regs + S3C_IISPSR);
                break;
        default:
                return -EINVAL;
        }

	return 0;
}

/* default table of all avaialable root fs divisors */
static unsigned int iis_fs_tab[] = { 256, 512, 384, 768 };

int s3c_i2sv2_iis_calc_rate(struct s3c_i2sv2_rate_calc *info,
			    unsigned int *fstab,
			    unsigned int rate, struct clk *clk)
{
	unsigned long clkrate = clk_get_rate(clk);
	unsigned int div;
	unsigned int fsclk;
	unsigned int actual;
	unsigned int fs;
	unsigned int fsdiv;
	signed int deviation = 0;
	unsigned int best_fs = 0;
	unsigned int best_div = 0;
	unsigned int best_rate = 0;
	unsigned int best_deviation = INT_MAX;

	pr_debug("Input clock rate %ldHz\n", clkrate);

	if (fstab == NULL)
		fstab = iis_fs_tab;

	for (fs = 0; fs < ARRAY_SIZE(iis_fs_tab); fs++) {
		fsdiv = iis_fs_tab[fs];

		fsclk = clkrate / fsdiv;
		div = fsclk / rate;

		if ((fsclk % rate) > (rate / 2))
			div++;

		if (div <= 1)
			continue;

		actual = clkrate / (fsdiv * div);
		deviation = actual - rate;

		printk(KERN_DEBUG "%ufs: div %u => result %u, deviation %d\n",
		       fsdiv, div, actual, deviation);

		deviation = abs(deviation);

		if (deviation < best_deviation) {
			best_fs = fsdiv;
			best_div = div;
			best_rate = actual;
			best_deviation = deviation;
		}

		if (deviation == 0)
			break;
	}

	printk(KERN_DEBUG "best: fs=%u, div=%u, rate=%u\n",
	       best_fs, best_div, best_rate);

	info->fs_div = best_fs;
	info->clk_div = best_div;

	return 0;
}
EXPORT_SYMBOL_GPL(s3c_i2sv2_iis_calc_rate);

int s3c_i2sv2_probe(struct platform_device *pdev,
		    struct snd_soc_dai *dai,
		    struct s3c_i2sv2_info *i2s,
		    unsigned long base)
{
	struct device *dev = &pdev->dev;
	unsigned int iismod;

	i2s->dev = dev;

	/* record our i2s structure for later use in the callbacks */
	dai->private_data = i2s;
#if defined(CONFIG_PLAT_S5P)
	writel(((1<<0)|(1<<31)), i2s->regs + S3C_IISCON);
#endif

	/* Mark ourselves as in TXRX mode so we can run through our cleanup
	 * process without warnings. */
	iismod = readl(i2s->regs + S3C_IISMOD);
	iismod |= S3C_IISMOD_TXRX;
	init_i2s(i2s);
	s3c_snd_txctrl(i2s, 0);
	s3c_snd_rxctrl(i2s, 0);

	return 0;
}
EXPORT_SYMBOL_GPL(s3c_i2sv2_probe);
#ifdef CONFIG_PM
static int s3c_i2s_suspend(struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);
	
	if(value_saved	!= true){
	 	i2s->iiscon = readl(i2s->regs + S3C_IISCON);
        	i2s->iismod = readl(i2s->regs + S3C_IISMOD);
        	i2s->iisfic = readl(i2s->regs + S3C_IISFIC);
        	i2s->iispsr = readl(i2s->regs + S3C_IISPSR);
        	i2s->audss_clksrc = __raw_readl(S5P_CLKSRC_AUDSS);
        	i2s->audss_clkdiv = __raw_readl(S5P_CLKDIV_AUDSS);
        	i2s->audss_clkgate = __raw_readl(S5P_CLKGATE_AUDSS);
	}

	

	return 0;
}

static int s3c_i2s_resume(struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);

	if(value_saved!=true){
		__raw_writel(i2s->audss_clksrc,S5P_CLKSRC_AUDSS);
                __raw_writel(i2s->audss_clkdiv,S5P_CLKDIV_AUDSS);
                __raw_writel(i2s->audss_clkgate,S5P_CLKGATE_AUDSS);
                writel(i2s->iiscon, i2s->regs + S3C_IISCON);
                writel(i2s->iismod, i2s->regs + S3C_IISMOD);
                writel(i2s->iisfic, i2s->regs + S3C_IISFIC);
                writel(i2s->iispsr, i2s->regs + S3C_IISPSR);
	}

	return 0;
}
#else
#define s3c_i2s_suspend NULL
#define s3c_i2s_resume  NULL
#endif

void s3c_i2sv2_register_dai(struct snd_soc_dai *dai)
{
	struct snd_soc_dai_ops *ops = dai->ops;
	dai->ops->startup   = s3c_i2s_startup;
        ops->shutdown  = s3c_i2s_shutdown; 
	ops->prepare = s3c_i2s_prepare;
	ops->trigger = s3c_i2s_trigger;
	ops->hw_params = s3c_i2s_hw_params;
	ops->set_fmt = s3c_i2s_set_fmt;
	ops->set_clkdiv = s3c_i2s_set_clkdiv;
	ops->set_sysclk = s3c_i2s_set_sysclk;
	dai->suspend = s3c_i2s_suspend;
	dai->resume = s3c_i2s_resume;

}
EXPORT_SYMBOL_GPL(s3c_i2sv2_register_dai);

MODULE_LICENSE("GPL");
