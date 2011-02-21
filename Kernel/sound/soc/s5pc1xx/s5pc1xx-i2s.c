/* sound/soc/s3c24xx/s5pc1xx-i2s.c
 *
 * ALSA SoC Audio Layer - s5pc1xx I2S driver
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include <sound/soc.h>

#include <mach/dma.h>
#include <mach/map.h>
#include <mach/irqs.h> 
#include <mach/regs-clock.h> 
#include <mach/regs-audss.h> 
#include "s3c-dma.h"
#include "s5pc1xx-i2s.h"
/* The value should be set to maximum of the total number
 * of I2Sv3 controllers that any supported SoC has.
 */
#define MAX_I2SV3	2

extern struct snd_soc_dai i2s_sec_fifo_dai;
extern void s5p_i2s_sec_init(void *, dma_addr_t);

static struct s3c2410_dma_client s5pc1xx_dma_client_out = {
	.name		= "I2S PCM Stereo out"
};

static struct s3c2410_dma_client s5pc1xx_dma_client_in = {
	.name		= "I2S PCM Stereo in"
};

static struct s3c_dma_params s5pc1xx_i2s_pcm_stereo_out[MAX_I2SV3];
static struct s3c_dma_params s5pc1xx_i2s_pcm_stereo_in[MAX_I2SV3];
static struct s3c_i2sv2_info s5pc1xx_i2s[MAX_I2SV3];
static struct s3c_i2sv2_info *i2s;
struct snd_soc_dai s5pc1xx_i2s_dai[MAX_I2SV3];

EXPORT_SYMBOL_GPL(s5pc1xx_i2s_dai);

static inline struct s3c_i2sv2_info *to_info(struct snd_soc_dai *cpu_dai)
{
	return cpu_dai->private_data;
}

static irqreturn_t s3c_iis_irq(int irqno, void *dev_id)
{
        u32 iiscon, iisahb;
	//u32	val;
//TODO
        //dump_i2s();
#if 1
        if(readl(i2s->regs + S3C_IISCON) & (1<<26))
        {
        //printk("\n rx overflow int ..@=%d\n",__LINE__);
        writel(readl(i2s->regs + S3C_IISCON) | (1<<26),i2s->regs + S3C_IISCON); //clear rxfifo overflow interrupt
	//      writel(readl(i2s->regs + S3C_IISFIC) | (1<<7) , i2s->regs + S3C_IISFIC); //flush rx
	}
        //printk("++++IISCON = 0x%08x\n, IISFIC 0x%x", readl(i2s->regs + S3C_IISCON),readl(i2s->regs + S3C_IISFIC));
#endif
	iisahb  = readl(i2s->regs + S3C_IISAHB);
        iiscon  = readl(i2s->regs + S3C_IISCON);
        if(iiscon & S3C_IISCON_FTXSURSTAT) {
                iiscon |= S3C_IISCON_FTXURSTATUS;
                writel(iiscon, i2s->regs + S3C_IISCON);
                //printk("TX_S underrun interrupt IISCON = 0x%08x\n", readl(i2s->regs + S3C_IISCON));
        }
        if(iiscon & S3C_IISCON_FTXURSTATUS) {
                //iiscon &= ~S3C_IISCON_FTXURINTEN;
                iiscon |= S3C_IISCON_FTXURSTATUS;
                writel(iiscon, i2s->regs + S3C_IISCON);
                //printk("TX_P underrun interrupt IISCON = 0x%08x\n", readl(i2s->regs + S3C_IISCON));
        }
	return IRQ_HANDLED;
}

struct clk *s5pc1xx_i2s_get_clock(struct snd_soc_dai *dai)
{

	return i2s->i2sclk;
}
EXPORT_SYMBOL_GPL(s5pc1xx_i2s_get_clock);

#define s5pc1xx_I2S_RATES \
	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | \
	SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
	SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 | \
	SNDRV_PCM_RATE_KNOT)

#define s5pc1xx_I2S_FMTS \
	(SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE |\
	 SNDRV_PCM_FMTBIT_S24_LE)

static int s5pc1xx_iis_dev_probe(struct platform_device *pdev,struct snd_soc_dai *dai)
{
	struct s3c_audio_pdata *i2s_pdata;
	//struct resource *res;//getting resources was failing
	int ret = 0; 
        struct clk *cm , *cn; 
	if (pdev->id >= MAX_I2SV3) {
		dev_err(&pdev->dev, "id %d out of range\n", pdev->id);
		return -EINVAL;
	}

	i2s = &s5pc1xx_i2s[pdev->id];
	i2s->dma_capture = &s5pc1xx_i2s_pcm_stereo_in[pdev->id];
	i2s->dma_playback = &s5pc1xx_i2s_pcm_stereo_out[pdev->id];
#if 0
	res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get I2S-TX dma resource\n");
		return -ENXIO;
	}
	//i2s->dma_playback->channel = res->start;
#endif //sayanta commmented.. it is unable to get resource
	i2s->dma_playback->channel = S3C_DMACH_I2S_OUT; 
#if 0
	res = platform_get_resource(pdev, IORESOURCE_DMA, 1);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get I2S-RX dma resource\n");
		return -ENXIO;
	}
	//i2s->dma_capture->channel = res->start;
#endif //sayanta commmented.. it is unable to get resource
	i2s->dma_capture->channel = S3C_DMACH_I2S_IN; 

#if 0
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get I2S SFR address\n");
		return -ENXIO;
	}
	if (!request_mem_region(res->start, resource_size(res),
							"s5pc1xx-i2s")) {
		dev_err(&pdev->dev, "Unable to request SFR region\n");
		return -EBUSY;
	}
#endif ////sayanta commmented..it is unable to get resource


	i2s->dma_capture->client = &s5pc1xx_dma_client_in;
	i2s->dma_capture->dma_size = 4;
	i2s->dma_playback->client = &s5pc1xx_dma_client_out;
	i2s->dma_playback->dma_size = 4;

	i2s_pdata = pdev->dev.platform_data;

        i2s->regs = ioremap(S3C_IIS_PABASE, 0x100); 
        if (i2s->regs == NULL)
                return -ENXIO;

	ret = request_irq(S3C_IISIRQ, s3c_iis_irq, 0, "s3c-i2s", pdev);
        if (ret < 0) {
                printk("fail to claim i2s irq , ret = %d\n", ret);
                iounmap(i2s->regs);
                return -ENODEV;
        }
	
	i2s->dma_capture->dma_addr = S3C_IIS_PABASE + S3C_IISRXD;
        i2s->dma_playback->dma_addr = S3C_IIS_PABASE + S3C_IISTXD;
	/* Configure the I2S pins if MUX'ed */
/*	if (i2s_pdata && i2s_pdata->cfg_gpio && i2s_pdata->cfg_gpio(pdev)) {  
		dev_err(&pdev->dev, "Unable to configure gpio\n");
		return -EINVAL;
	}*/ //sayanta commented..to avoid compilation error ..need to check usage

        i2s->iis0_ip_clk = clk_get(&pdev->dev, CLK_I2S0);
        if (IS_ERR(i2s->iis0_ip_clk)) {
                printk("failed to get clk(%s)\n", CLK_I2S0);
                goto err;
        }
        printk("Got Clock -> %s\n", CLK_I2S0);
        /* To avoid switching between sources(LP vs NM mode),
         * we use EXTPRNT as parent clock of i2sclkd2.
        */
	
	i2s->i2sclk = clk_get(&pdev->dev, EXTCLK);
        if (IS_ERR(i2s->i2sclk)) {
                printk("failed to get clk(%s)\n", EXTCLK);
                goto lb3;
        }
        printk("Got Audio I2s-Clock -> %s\n", EXTCLK);
       
	i2s->i2sbusclk = clk_get(NULL, "dout_audio_bus_clk_i2s");//getting AUDIO BUS CLK
         if (IS_ERR(i2s->i2sbusclk)) {
                        printk("failed to get audss_hclk\n");
                        goto lb2;
         }
         printk("Got audss_hclk..Audio-Bus-clk\n");
        
	cm = clk_get(NULL, EPLLCLK);
        if (IS_ERR(cm)) {
                printk("failed to get fout_epll\n");
                goto lb2_busclk;
        }

        cn = clk_get(NULL,EXTPRNT);
        if (IS_ERR(cn)) {
                printk("failed to get i2smain_clk\n");
                goto lb1;
        }

         if(clk_set_parent(i2s->i2sclk, cn)){
                printk("failed to set i2s-main clock as parent of audss_hclk\n");
                clk_put(cn);
                goto lb1;
        }

        if(clk_set_parent(cn, cm)){
                printk("failed to set fout-epll as parent of i2s-main clock \n");
                clk_put(cn);
                goto lb1;
        }

	clk_put(cn);
	clk_put(cm);
        clk_put(i2s->i2sbusclk);
 	
	ret = s3c_i2sv2_probe(pdev, dai, i2s,
			S3C_IIS_PABASE);
	if (ret)
		goto err;
#if 0 //sayanta commented secondary DAI initialization 
	/*** Secondary Stream DAI ***/
	if (pdev->id == 0) { /* I2S_v5 */
		i2s_sec_fifo_dai.dev = &pdev->dev;
		i2s_sec_fifo_dai.playback.rates = s5pc1xx_I2S_RATES;
		i2s_sec_fifo_dai.playback.formats = s5pc1xx_I2S_FMTS;
		i2s_sec_fifo_dai.playback.channels_min = 2;
		i2s_sec_fifo_dai.playback.channels_max = 2;
		s5p_i2s_sec_init(i2s->regs,
			i2s->dma_playback->dma_addr - S3C2412_IISTXD);
		ret = snd_soc_register_dai(&i2s_sec_fifo_dai);
		if (ret) {
			snd_soc_unregister_dai(dai);
			goto err_i2sv2;
		}
	}
	/******************************/
#endif
	return 0;
#ifdef USE_CLKAUDIO
lb1:
        clk_put(cm);

lb2_busclk:
        clk_put(i2s->i2sbusclk);
lb2:
        clk_put(i2s->i2sclk);
#endif
lb3:
        clk_put(i2s->iis0_ip_clk);
	
//err_i2sv2: //used in secondary DAI initialization
	/* Not implemented for I2Sv2 core yet */
err:
	return ret;
}

static void s5pc1xx_iis_dev_remove(struct platform_device *pdev,struct snd_soc_dai *dai)
{
	dev_err(&pdev->dev, "Device removal not yet supported\n");
}
static struct snd_soc_dai_ops s5pc1xx_i2s_dai_ops ;
struct snd_soc_dai i2s_dai = {
                .name = "s3c-i2s",
                .id = 0,
                .probe = s5pc1xx_iis_dev_probe,
                .remove = s5pc1xx_iis_dev_remove,
                .playback = {
                        .channels_min = 2,
                        .channels_max = PLBK_CHAN,
                        .rates = SNDRV_PCM_RATE_8000_96000,
                        .formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
                        },
                .capture = {
                        .channels_min = 1,
                        .channels_max = 2,
                        .rates = SNDRV_PCM_RATE_8000_96000,
                        .formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
                },
		.ops = &s5pc1xx_i2s_dai_ops,
};
EXPORT_SYMBOL_GPL(i2s_dai);
/*static struct platform_driver s5pc1xx_iis_driver = {
	.probe  = s5pc1xx_iis_dev_probe,
	.remove = s5pc1xx_iis_dev_remove,
	.driver = {
		.name = "s5pc1xx-iis",
		.owner = THIS_MODULE,
	},
};*/

static int __init s5pc1xx_i2s_init(void)
{
	s3c_i2sv2_register_dai(&i2s_dai);
	return snd_soc_register_dai(&i2s_dai);
}
module_init(s5pc1xx_i2s_init);

static void __exit s5pc1xx_i2s_exit(void)
{
	snd_soc_unregister_dai(&i2s_dai);
}
module_exit(s5pc1xx_i2s_exit);

/* Module information */
MODULE_AUTHOR("Ben Dooks, <ben@simtec.co.uk>");
MODULE_DESCRIPTION("s5pc1xx I2S SoC Interface");
MODULE_LICENSE("GPL");
