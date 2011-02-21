/* linux/drivers/video/samsung/s3cfb.c
 *
 * Core file for Samsung Display Controller (FIMD) driver
 *
 * Jonghun Han, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * InKi Dae, <inki.dae@samsung.com>
 * 	added Dual lcd features.
 * 	added progress bar.
 * 	changed the way of setting lcd panel data.
 * 	added mDNIe support.
 * 	added MIPI-DSI support.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#endif
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>

#include <plat/clock.h>
#include <mach/regs-clock.h>
#include <plat/regs-fb.h>
#include <plat/cpu-freq.h>
#include <plat/pm.h>
#include <plat/media.h>

#include <asm/mach-types.h>

#include <mach/map.h>
//#include <mach/mdnie.h> mipi_temp
#include <mach/mipi_ddi.h>

#include <linux/suspend.h>

#ifdef CONFIG_FB_S3C_LTE480WV
#include "logo_rgb24_wvga_landscape.h"
#endif

//#include "logo.h"

#ifdef CONFIG_FB_S3C_AMS320
#include "logo_rgb24_hvga_portrait.h"
#endif

#include "logo_rgb24_wvga_portrait.h"
#include "s3cfb.h"

#define GAMMA_MAX_LEVEL	6
#define LCD_ESD_INT   IRQ_EINT10

#define DBG printk

#ifdef CONFIG_HAS_WAKELOCK
        extern suspend_state_t get_suspend_state(void);
        //extern suspend_state_t requested_suspend_state;
#endif
extern void lcd_pannel_on(void);
extern void ams397g201_sleep_in(struct device *dev);
extern void ams397g201_sleep_out(struct device *dev);

static struct s3cfb_global *fbdev;
extern int machine_is_screensplit(void);

/* vsync interrupt flag for SGX Driver. */
int vsync_flag = 0;
EXPORT_SYMBOL(vsync_flag);

/* for vsync interrupt handler */
static LIST_HEAD(vsync_info_list);
static DEFINE_MUTEX(vsync_lock);

#ifdef CONFIG_FB_S3C_PROGRESS_BAR
extern int fbutil_init(void);
extern int fbutil_stop(void);
extern int fbutil_exit(void);
extern int fbutil_thread_run(unsigned int delay);
#endif

/*
 *  Mark for GetLog (tkhwang)
 */

struct struct_frame_buf_mark {
	u32 special_mark_1;
	u32 special_mark_2;
	u32 special_mark_3;
	u32 special_mark_4;
	void *p_fb;
	u32 resX;
	u32 resY;
	u32 bpp;    //color depth : 16 or 24
	u32 frames; // frame buffer count : 2
};

static struct struct_frame_buf_mark  frame_buf_mark = {
	.special_mark_1 = (('*' << 24) | ('^' << 16) | ('^' << 8) | ('*' << 0)),
	.special_mark_2 = (('I' << 24) | ('n' << 16) | ('f' << 8) | ('o' << 0)),
	.special_mark_3 = (('H' << 24) | ('e' << 16) | ('r' << 8) | ('e' << 0)),
	.special_mark_4 = (('f' << 24) | ('b' << 16) | ('u' << 8) | ('f' << 0)),
	.p_fb   = 0,
	.resX   = 480,
	.resY   = 992,
	.bpp    = 32,
	.frames = 2
};

struct s3c_platform_fb *to_fb_plat(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	return (struct s3c_platform_fb *)pdev->dev.platform_data;
}

int conv_rgb565_to_rgb888(unsigned short rgb565, unsigned int sw)
{
	char red, green, blue;
	unsigned int threshold = 150;

	red = (rgb565 & 0xF800) >> 11;
	green = (rgb565 & 0x7E0) >> 5;
	blue = (rgb565 & 0x1F);

	red = red << 3;
	green = green << 2;
	blue = blue << 3;

	/* correct error pixels of samsung logo. */
	if (sw) {
		if (red > threshold)
			red = 255;
		if (green > threshold)
			green = 255;
		if (blue > threshold)
			blue = 255;
	}

	return (red << 16 | green << 8 | blue);
}

static void draw_bitmap(void *lcdbase, int lcd_width, int x, int y, int w, int h, unsigned long *bmp)
{
	int i, j, k = 0;
	unsigned long *fb = (unsigned  long *)lcdbase;

	for (j = y; j < (y + h); j++) {
		for (i = x; i < (w + x); i++) {
			*(fb + (j * lcd_width) + i) =
			    *(unsigned long *)(bmp + k);
			k++;
		}
	}
}

void _draw_samsung_logo(void *lcdbase, int lcd_width, int x, int y, int w, int h, unsigned short *bmp)
{
	int i, j, error_range = 40;
	short k = 0;
	unsigned int pixel;
	unsigned long *fb = (unsigned  long*) lcdbase;

	for (j = y; j < (y + h); j++) {
		for (i = x; i < (x + w); i++) {
			pixel = (*(bmp + k++));

			/* 40 lines under samsung logo image are error range. */
			if (j > h + y - error_range)
				*(fb + (j * lcd_width) + i) =
					conv_rgb565_to_rgb888(pixel, 1);
			else
				*(fb + (j * lcd_width) + i) =
					conv_rgb565_to_rgb888(pixel, 0);
		}
	}
}
//mipi_temp
#if 0 
static void draw_samsung_logo(struct s3c_platform_fb *pdata)
{
	unsigned int x, y, win_id, width, height;
	struct fb_info *pfb = NULL;

	win_id = pdata->default_win;
	pfb = fbdev->fb[win_id];

	width = pfb->var.xres;
	height = pfb->var.yres;

	x = (width - 298) / 2;
	y = (height - 78) / 2 - 5;

	_draw_samsung_logo(pfb->screen_base, width, x, y, 298, 78,
		(unsigned short *) logo);
}
#ifndef CONFIG_FRAMEBUFFER_CONSOLE
static int s3cfb_draw_logo(struct fb_info *fb)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);

	draw_samsung_logo(pdata);

	return 0;
}
#else
int fb_is_primary_device(struct fb_info *fb)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	struct s3cfb_window *win = fb->par;

	dev_dbg(fbdev->dev, "[fb%d] checking for primary device\n", win->id);

	if (win->id == pdata->default_win)
		return 1;
	else
		return 0;
}
#endif
#endif
void s3cfb_enable_vsync_interrupt(void)
{
	s3cfb_set_global_interrupt(fbdev, 1);
	s3cfb_set_vsync_interrupt(fbdev, 1);

	fbdev->vsync_ref_count++;
}
EXPORT_SYMBOL(s3cfb_enable_vsync_interrupt);

void s3cfb_disable_vsync_interrupt(void)
{
	fbdev->vsync_ref_count--;

	if (fbdev->vsync_ref_count <= 0) {
		s3cfb_set_global_interrupt(fbdev, 0);
		s3cfb_set_vsync_interrupt(fbdev, 0);

		fbdev->vsync_ref_count = 0;
	}
}
EXPORT_SYMBOL(s3cfb_disable_vsync_interrupt);

int s3cfb_register_vsync_handler(int (*vsync_handler) (void *pdata),
	void *pdata, const char *name)
{
	struct s3cfb_vsync *vsync = NULL;

	vsync = kmalloc(sizeof(struct s3cfb_vsync), GFP_KERNEL);
	if (vsync == NULL)
		return -ENOMEM;

	if (vsync_handler) {
		vsync->vsync_handler = vsync_handler;
		if (pdata)
			vsync->pdata = pdata;
		if (name)
			strcpy(vsync->name, name);
	} else
		return -1;

	mutex_lock(&vsync_lock);
	list_add_tail(&vsync->list, &vsync_info_list);
	mutex_unlock(&vsync_lock);

	dev_dbg(fbdev->dev, "registered vsync_handler(%s)\n", name);

	return 0;
}
EXPORT_SYMBOL(s3cfb_register_vsync_handler);

int s3cfb_unregister_vsync_handler(const char *name)
{
	struct s3cfb_vsync *vsync, *temp;

	mutex_lock(&vsync_lock);

	list_for_each_entry_safe(vsync, temp, &vsync_info_list, list) {
		if (strcmp(vsync->name, name) == 0) {
			list_del(&vsync->list);
			kfree(vsync);

			goto found;
		}
	}

	dev_warn(fbdev->dev, "can't find vsync handler(%s).\n", name);

	mutex_unlock(&vsync_lock);

	return -1;

found:
	mutex_unlock(&vsync_lock);

	dev_dbg(fbdev->dev, "removed vsync handler(%s) from vsync list.\n",
		name);

	return 0;
}
EXPORT_SYMBOL(s3cfb_unregister_vsync_handler);

static irqreturn_t s3cfb_irq_frame(int irq, void *dev_id)
{
	struct s3cfb_vsync *vsync = NULL;

	s3cfb_clear_interrupt(fbdev);

	/* call vsync handlers registered. */
	list_for_each_entry(vsync, &vsync_info_list, list) {
		if (vsync)
			vsync->vsync_handler(vsync->pdata);
	}

	fbdev->wq_count++;

	//spin-lock init
	init_waitqueue_head(&fbdev->wq);

	wake_up(&fbdev->wq);

	return IRQ_HANDLED;
}

#ifdef CONFIG_FB_S3C_TRACE_UNDERRUN
static irqreturn_t s3cfb_irq_fifo(int irq, void *dev_id)
{
	s3cfb_clear_interrupt(fbdev);

	return IRQ_HANDLED;
}
#endif
extern unsigned char bFrameDone;
static irqreturn_t s3cfb_irq_i80done(int irq, void *dev_id)
{
	s3cfb_clear_interrupt(fbdev);
	s3cfb_is_frame_done(fbdev);
	bFrameDone |= 0x2;
}

static int s3cfb_enable_window(int id)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;

	if (s3cfb_window_on(fbdev, id)) {
		win->enabled = 0;
		return -EFAULT;
	} else {
		win->enabled = 1;
		return 0;
	}
}



static int s3cfb_enable_localpath(int id)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;

	if (s3cfb_channel_localpath_on(fbdev, id)) {
		win->enabled = 0;
		return -EFAULT;
	} else {
		win->enabled = 1;
		return 0;
	}
}

static int s3cfb_disable_localpath(int id)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;

	if (s3cfb_channel_localpath_off(fbdev, id)) {
		win->enabled = 1;
		return -EFAULT;
	} else {
		win->enabled = 0;
		return 0;
	}
}

static int s3cfb_disable_window(int id)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;

	if (s3cfb_window_off(fbdev, id)) {
		win->enabled = 1;
		return -EFAULT;
	} else {
		win->enabled = 0;
		return 0;
	}
}

static int s3cfb_init_global(void)
{
	fbdev->output = OUTPUT_RGB;
	fbdev->rgb_mode = MODE_RGB_P;

	fbdev->wq_count = 0;
	fbdev->vsync_ref_count = 0;
	init_waitqueue_head(&fbdev->wq);
	mutex_init(&fbdev->lock);

	s3cfb_set_output(fbdev);
	s3cfb_set_display_mode(fbdev);
	s3cfb_set_polarity(fbdev);
	s3cfb_set_timing(fbdev);
	s3cfb_set_lcd_size(fbdev); //sehun

	return 0;
}

static int s3cfb_map_video_memory(struct fb_info *fb)
{
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s3cfb_window *win = fb->par;
	unsigned int reserved_size;

	if (win->owner == DMA_MEM_OTHER)
		return 0;

	fix->smem_start = s3c_get_media_memory_bank(S3C_MDEV_FIMD, 1);
	reserved_size = s3c_get_media_memsize_bank(S3C_MDEV_FIMD, 1);
	fb->screen_base = ioremap_wc(fix->smem_start, reserved_size);

	if (!fb->screen_base)
		return -ENOMEM;
	else
		dev_info(fbdev->dev, "[fb%d] dma: 0x%08x, cpu: 0x%08x, "
			 "size: 0x%08x\n", win->id,
			 (unsigned int)fix->smem_start,
			 (unsigned int)fb->screen_base, fix->smem_len);

	win->owner = DMA_MEM_FIMD;

#ifdef CONFIG_HIBERNATION
	/*
	 * FIXME : for generic
	 * LCD memory will be no saved except main LCD content
	 */
	do {
		unsigned long start = fix->smem_start + ((fix->smem_len) >> 1);
		unsigned long end = fix->smem_start + fix->smem_len;
		register_nosave_region(start >> PAGE_SHIFT, end >> PAGE_SHIFT);
	} while (0);
#endif

	/*
	 *  Mark for GetLog (tkhwang)
	 */

   	frame_buf_mark.p_fb = fix->smem_start;

	return 0;
}

static int s3cfb_unmap_video_memory(struct fb_info *fb)
{
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s3cfb_window *win = fb->par;

	if (fix->smem_start) {
		dma_free_writecombine(fbdev->dev, fix->smem_len,
				      fb->screen_base, fix->smem_start);
		fix->smem_start = 0;
		fix->smem_len = 0;
		dev_info(fbdev->dev, "[fb%d] video memory released\n", win->id);
	}

	return 0;
}

static int s3cfb_set_bitfield(struct fb_var_screeninfo *var)
{
	switch (var->bits_per_pixel) {
	case 16:
		if (var->transp.length == 1) {
			var->red.offset = 10;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 5;
			var->blue.offset = 0;
			var->blue.length = 5;
			var->transp.offset = 15;
		} else if (var->transp.length == 4) {
			var->red.offset = 8;
			var->red.length = 4;
			var->green.offset = 4;
			var->green.length = 4;
			var->blue.offset = 0;
			var->blue.length = 4;
			var->transp.offset = 12;
		} else {
			var->red.offset = 11;
			var->red.length = 5;
			var->green.offset = 5;
			var->green.length = 6;
			var->blue.offset = 0;
			var->blue.length = 5;
			var->transp.offset = 0;
		}
		break;

	case 24:
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;

	case 32:
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 24;
		break;
	}

	return 0;
}

static int s3cfb_set_alpha_info(struct fb_var_screeninfo *var,
				struct s3cfb_window *win)
{
	if (var->transp.length > 0)
		win->alpha.mode = PIXEL_BLENDING;
	else {
		win->alpha.mode = PLANE_BLENDING;
		win->alpha.channel = 0;
		win->alpha.value = S3CFB_AVALUE(0xf, 0xf, 0xf);
	}

	return 0;
}

static int s3cfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;
	struct s3cfb_lcd *lcd = fbdev->lcd;

	dev_dbg(fbdev->dev, "[fb%d] check_var\n", win->id);

	if (var->bits_per_pixel != 16 && var->bits_per_pixel != 24 &&
	    var->bits_per_pixel != 32) {
		dev_err(fbdev->dev, "invalid bits per pixel\n");
		return -EINVAL;
	}

	if (var->xres > lcd->width)
		var->xres = lcd->width;

	if (var->yres > lcd->height)
		var->yres = lcd->height;

	if (var->xres_virtual != var->xres)
		var->xres_virtual = var->xres;

	if (var->yres_virtual > var->yres * (fb->fix.ypanstep + 1))
		var->yres_virtual = var->yres * (fb->fix.ypanstep + 1);

	if (var->xoffset != 0)
		var->xoffset = 0;

	if (var->yoffset + var->yres > var->yres_virtual)
		var->yoffset = var->yres_virtual - var->yres;

	if (win->x + var->xres > lcd->width)
		win->x = lcd->width - var->xres;

	if (win->y + var->yres > lcd->height)
		win->y = lcd->height - var->yres;

	s3cfb_set_bitfield(var);
	s3cfb_set_alpha_info(var, win);

	return 0;
}

static void s3cfb_set_win_params(int id)
{
	s3cfb_set_window_control(fbdev, id);
	s3cfb_set_window_position(fbdev, id);
	s3cfb_set_window_size(fbdev, id);
	s3cfb_set_buffer_address(fbdev, id);
	s3cfb_set_buffer_size(fbdev, id);

	if (id > 0) {
		s3cfb_set_alpha_blending(fbdev, id);
		s3cfb_set_chroma_key(fbdev, id);
	}
}

static int s3cfb_set_par(struct fb_info *fb)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	struct s3cfb_window *win = fb->par;

	dev_dbg(fbdev->dev, "[fb%d] set_par\n", win->id);

	if ((win->id != pdata->default_win) && fb->fix.smem_start)
		s3cfb_unmap_video_memory(fb);

	fb->fix.line_length = fb->var.xres_virtual * fb->var.bits_per_pixel / 8;
	fb->fix.smem_len = fb->fix.line_length * fb->var.yres_virtual;

	if ((win->id != pdata->default_win))
		s3cfb_map_video_memory(fb);

	s3cfb_set_win_params(win->id);

	if (!pdata->mdnie_is_enabled) {
		if (pdata->sub_lcd_enabled)
			s3cfb_set_dualrgb(fbdev, 1);
		else
			s3cfb_set_dualrgb(fbdev, 0);
	}

	return 0;
}

static int s3cfb_blank(int blank_mode, struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;

	dev_dbg(fbdev->dev, "change blank mode\n");

	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		if (fb->fix.smem_start) {
			s3cfb_win_map_off(fbdev, win->id);
			s3cfb_enable_window(win->id);
		} else {
			dev_info(fbdev->dev,
				 "[fb%d] no allocated memory for unblank\n",
				 win->id);
		}

		break;

	case FB_BLANK_NORMAL:
		s3cfb_win_map_on(fbdev, win->id, 0x0);
		s3cfb_enable_window(win->id);

		break;
		
	case FB_BLANK_POWERDOWN:
		s3cfb_disable_window(win->id);
		s3cfb_win_map_off(fbdev, win->id);

		break;

	case FB_BLANK_VSYNC_SUSPEND:	/* fall through */
	case FB_BLANK_HSYNC_SUSPEND:	/* fall through */
	default:
		dev_dbg(fbdev->dev, "unsupported blank mode\n");
		return -EINVAL;
	}

	return 1;
}

static int s3cfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;

	if (var->yoffset + var->yres > var->yres_virtual) {
		dev_err(fbdev->dev, "invalid yoffset value\n");
		return -EINVAL;
	}

	fb->var.yoffset = var->yoffset;

	dev_dbg(fbdev->dev, "[fb%d] yoffset for pan display: %d\n", win->id,
		var->yoffset);

	s3cfb_set_buffer_address(fbdev, win->id);

	return 0;
}

static inline unsigned int __chan_to_field(unsigned int chan,
					   struct fb_bitfield bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf.length;

	return chan << bf.offset;
}

static int s3cfb_setcolreg(unsigned int regno, unsigned int red,
			   unsigned int green, unsigned int blue,
			   unsigned int transp, struct fb_info *fb)
{
	unsigned int *pal = (unsigned int *)fb->pseudo_palette;
	unsigned int val = 0;

	if (regno < 16) {
		/* fake palette of 16 colors */
		val |= __chan_to_field(red, fb->var.red);
		val |= __chan_to_field(green, fb->var.green);
		val |= __chan_to_field(blue, fb->var.blue);
		val |= __chan_to_field(transp, fb->var.transp);

		pal[regno] = val;
	}

	return 0;
}

static int s3cfb_open(struct fb_info *fb, int user)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	struct s3cfb_window *win = fb->par;
	int ret = 0;

	mutex_lock(&fbdev->lock);

#ifdef CONFIG_FB_S3C_PROGRESS_BAR
	fbutil_stop();
#endif

	if (atomic_read(&win->in_use)) {
		if (win->id == pdata->default_win) {
			dev_dbg(fbdev->dev,
				"multiple open for default window\n");
			ret = 0;
		} else {
			dev_dbg(fbdev->dev,
				"do not allow multiple open "
				"for non-default window\n");
			ret = -EBUSY;
		}
	} else
		atomic_inc(&win->in_use);

	mutex_unlock(&fbdev->lock);

	return ret;
}

static int s3cfb_release_window(struct fb_info *fb)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	struct s3cfb_window *win = fb->par;

	if (win->id != pdata->default_win) {
		s3cfb_disable_window(win->id);
		s3cfb_unmap_video_memory(fb);
		s3cfb_set_buffer_address(fbdev, win->id);
	}

	win->x = 0;
	win->y = 0;

	return 0;
}

static int s3cfb_release(struct fb_info *fb, int user)
{
	struct s3cfb_window *win = fb->par;

	s3cfb_release_window(fb);

	mutex_lock(&fbdev->lock);
	atomic_dec(&win->in_use);
	mutex_unlock(&fbdev->lock);

	return 0;
}

static int s3cfb_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
	/* nothing to do for removing cursor */
	return 0;
}

static int s3cfb_wait_for_vsync(void)
{
	dev_dbg(fbdev->dev, "waiting for VSYNC interrupt\n");

	//sleep_on_timeout(&fbdev->wq, HZ / 10);

	dev_dbg(fbdev->dev, "got a VSYNC interrupt\n");

	return 0;
}

int s3cfb_change_mode(unsigned int id, unsigned int lcd_mode);

static int s3cfb_ioctl(struct fb_info *fb, unsigned int cmd, unsigned long arg)
{
	struct fb_var_screeninfo *var = &fb->var;
	struct s3cfb_window *win = fb->par;
	struct s3cfb_lcd *lcd = fbdev->lcd;
	int ret = 0;
	struct fb_fix_screeninfo *fix = &fb->fix;
        s3cfb_next_info_t next_fb_info;
	union {
		struct s3cfb_user_window user_window;
		struct s3cfb_user_plane_alpha user_alpha;
		struct s3cfb_user_chroma user_chroma;
		int vsync;
		int lcd_mode;
	} p;

	switch (cmd) {
	case FBIO_WAITFORVSYNC:
		s3cfb_wait_for_vsync();
		break;

	case S3CFB_WIN_POSITION:
		if (copy_from_user(&p.user_window,
				   (struct s3cfb_user_window __user *)arg,
				   sizeof(p.user_window)))
			ret = -EFAULT;
		else {
			if (p.user_window.x < 0)
				p.user_window.x = 0;

			if (p.user_window.y < 0)
				p.user_window.y = 0;

			if (p.user_window.x + var->xres > lcd->width)
				win->x = lcd->width - var->xres;
			else
				win->x = p.user_window.x;

			if (p.user_window.y + var->yres > lcd->height)
				win->y = lcd->height - var->yres;
			else
				win->y = p.user_window.y;

			s3cfb_set_window_position(fbdev, win->id);
		}
		break;

	case S3CFB_WIN_SET_PLANE_ALPHA:
		if (copy_from_user(&p.user_alpha,
				   (struct s3cfb_user_plane_alpha __user *)arg,
				   sizeof(p.user_alpha)))
			ret = -EFAULT;
		else {
			win->alpha.mode = PLANE_BLENDING;
			win->alpha.channel = p.user_alpha.channel;
			win->alpha.value =
			    S3CFB_AVALUE(p.user_alpha.red,
					 p.user_alpha.green, p.user_alpha.blue);

			s3cfb_set_alpha_blending(fbdev, win->id);
		}
		break;

	case S3CFB_WIN_SET_CHROMA:
		if (copy_from_user(&p.user_chroma,
				   (struct s3cfb_user_chroma __user *)arg,
				   sizeof(p.user_chroma)))
			ret = -EFAULT;
		else {
			win->chroma.enabled = p.user_chroma.enabled;
			win->chroma.key = S3CFB_CHROMA(p.user_chroma.red,
						       p.user_chroma.green,
						       p.user_chroma.blue);

			s3cfb_set_chroma_key(fbdev, win->id);
		}
		break;

	case S3CFB_SET_VSYNC_INT:
		if (get_user(p.vsync, (int __user *)arg))
			ret = -EFAULT;
		else {
			if (p.vsync)
				s3cfb_set_global_interrupt(fbdev, 1);

			s3cfb_set_vsync_interrupt(fbdev, p.vsync);
		}
		break;
        case S3CFB_GET_CURR_FB_INFO:
                next_fb_info.phy_start_addr = fix->smem_start;
                next_fb_info.xres = var->xres;
                next_fb_info.yres = var->yres;
                next_fb_info.xres_virtual = var->xres_virtual;
                next_fb_info.yres_virtual = var->yres_virtual;
                next_fb_info.xoffset = var->xoffset;
                next_fb_info.yoffset = var->yoffset;
                //next_fb_info.lcd_offset_x = fbi->lcd_offset_x;
                //next_fb_info.lcd_offset_y = fbi->lcd_offset_y;
                next_fb_info.lcd_offset_x = 0;
                next_fb_info.lcd_offset_y = 0;

        if (copy_to_user((void *)arg, (s3cfb_next_info_t *) &next_fb_info, sizeof(s3cfb_next_info_t)))
            return -EFAULT;
        break;

		
	case S3CFB_CHANGE_MODE:
		if (get_user(p.lcd_mode, (int __user *)arg))
			ret = -EFAULT;

		s3cfb_change_mode(win->id, (unsigned int) p.lcd_mode);
		break;
	}

	return ret;
}

struct fb_ops s3cfb_ops = {
	.owner = THIS_MODULE,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_check_var = s3cfb_check_var,
	.fb_set_par = s3cfb_set_par,
	.fb_blank = s3cfb_blank,
	.fb_pan_display = s3cfb_pan_display,
	.fb_setcolreg = s3cfb_setcolreg,
	.fb_cursor = s3cfb_cursor,
	.fb_ioctl = s3cfb_ioctl,
	.fb_open = s3cfb_open,
	.fb_release = s3cfb_release,
};

/* new function to open fifo */
int s3cfb_open_fifo(int id, int ch, int (*do_priv) (void *), void *param)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;

	dev_dbg(fbdev->dev, "[fb%d] open fifo\n", win->id);

	if (win->path == DATA_PATH_DMA) {
		dev_err(fbdev->dev, "WIN%d is busy.\n", id);
		return -EFAULT;
	}

	win->local_channel = ch;

	if (do_priv) {
		if (do_priv(param)) {
			dev_err(fbdev->dev, "failed to run for "
				"private fifo open\n");
			s3cfb_enable_window(id);
			return -EFAULT;
		}
	}

	s3cfb_set_window_control(fbdev, id);
	s3cfb_enable_window(id);
	s3cfb_enable_localpath(id);

	return 0;
}

int s3cfb_close_fifo(int id, int (*do_priv) (void *), void *param)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;
	win->path = DATA_PATH_DMA;

	dev_dbg(fbdev->dev, "[fb%d] close fifo\n", win->id);

	if (do_priv) {
		s3cfb_display_off(fbdev);

		if (do_priv(param)) {
			dev_err(fbdev->dev, "failed to run for"
				"private fifo close\n");
			s3cfb_enable_window(id);
			s3cfb_display_on(fbdev);
			return -EFAULT;
		}

		s3cfb_disable_window(id);
		s3cfb_disable_localpath(id);
		s3cfb_display_on(fbdev);
	} else {
		s3cfb_disable_window(id);
		s3cfb_disable_localpath(id);	/* Only for FIMD 6.2 */
	}

	return 0;
}

int s3cfb_direct_ioctl(int id, unsigned int cmd, unsigned long arg)
{
	struct fb_info *fb = fbdev->fb[id];
	struct fb_var_screeninfo *var = &fb->var;
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s3cfb_window *win = fb->par;
	struct s3cfb_lcd *lcd = fbdev->lcd;
	struct s3cfb_user_window user_win;
	void *argp = (void *)arg;
	int ret = 0;

	switch (cmd) {
	case FBIOGET_FSCREENINFO:
		ret = memcpy(argp, &fb->fix, sizeof(fb->fix)) ? 0 : -EFAULT;
		break;

	case FBIOGET_VSCREENINFO:
		ret = memcpy(argp, &fb->var, sizeof(fb->var)) ? 0 : -EFAULT;
		break;

	case FBIOPUT_VSCREENINFO:
		ret = s3cfb_check_var((struct fb_var_screeninfo *)argp, fb);
		if (ret) {
			dev_err(fbdev->dev, "invalid vscreeninfo\n");
			break;
		}

		ret = memcpy(&fb->var, (struct fb_var_screeninfo *)argp,
			     sizeof(fb->var)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(fbdev->dev, "failed to put new vscreeninfo\n");
			break;
		}

		s3cfb_set_win_params(id);
		break;

	case S3CFB_WIN_POSITION:
		ret = memcpy(&user_win, (struct s3cfb_user_window __user *)arg,
			     sizeof(user_win)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(fbdev->dev, "failed to S3CFB_WIN_POSITION\n");
			break;
		}

		if (user_win.x < 0)
			user_win.x = 0;

		if (user_win.y < 0)
			user_win.y = 0;

		if (user_win.x + var->xres > lcd->width)
			win->x = lcd->width - var->xres;
		else
			win->x = user_win.x;

		if (user_win.y + var->yres > lcd->height)
			win->y = lcd->height - var->yres;
		else
			win->y = user_win.y;

		s3cfb_set_window_position(fbdev, win->id);
		break;

	case S3CFB_GET_LCD_WIDTH:
		ret = memcpy(argp, &lcd->width, sizeof(int)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(fbdev->dev, "failed to S3CFB_GET_LCD_WIDTH\n");
			break;
		}

		break;

	case S3CFB_GET_LCD_HEIGHT:
		ret = memcpy(argp, &lcd->height, sizeof(int)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(fbdev->dev, "failed to S3CFB_GET_LCD_HEIGHT\n");
			break;
		}

		break;

	case S3CFB_SET_WRITEBACK:
		if ((u32)argp == 1)
			fbdev->output = OUTPUT_WB_RGB;
		else
			fbdev->output = OUTPUT_RGB;

		s3cfb_set_output(fbdev);

		break;

	case S3CFB_SET_WIN_ON:
		s3cfb_enable_window(id);
		break;

	case S3CFB_SET_WIN_OFF:
		s3cfb_disable_window(id);
		break;

	case S3CFB_SET_WIN_PATH :
		win->path = (enum s3cfb_data_path_t)argp;
		break;

	case S3CFB_SET_WIN_ADDR:
		fix->smem_start = (unsigned long)argp;
		s3cfb_set_buffer_address(fbdev, id);
		break;

	case S3CFB_SET_WIN_MEM :
		win->owner = (enum s3cfb_mem_owner_t)argp;
		break;

	default:
		ret = s3cfb_ioctl(fb, cmd, arg);
		break;
	}

	return ret;
}

EXPORT_SYMBOL(s3cfb_direct_ioctl);

static int s3cfb_init_fbinfo(int id)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	struct fb_info *fb = fbdev->fb[id];
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct fb_var_screeninfo *var = &fb->var;
	struct s3cfb_window *win = fb->par;
	struct s3cfb_alpha *alpha = &win->alpha;
	struct s3cfb_lcd *lcd = fbdev->lcd;
	struct s3cfb_lcd_timing *timing = &lcd->timing;

	memset(win, 0, sizeof(struct s3cfb_window));
	platform_set_drvdata(to_platform_device(fbdev->dev), fb);
	strcpy(fix->id, S3CFB_NAME);

	/* fimd specific */
	win->id = id;
	win->path = DATA_PATH_DMA;
	win->dma_burst = 16;
	alpha->mode = PLANE_BLENDING;

	/* fbinfo */
	fb->fbops = &s3cfb_ops;
	fb->flags = FBINFO_FLAG_DEFAULT;
	fb->pseudo_palette = &win->pseudo_pal;
	fix->xpanstep = 0;
	fix->ypanstep = pdata->nr_buffers[id] - 1;
	fix->type = FB_TYPE_PACKED_PIXELS;
	fix->accel = FB_ACCEL_NONE;
	fix->visual = FB_VISUAL_TRUECOLOR;
	var->xres = lcd->width;
	var->yres = lcd->height;
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres + (var->yres * fix->ypanstep);
	var->bits_per_pixel = 32;
	var->xoffset = 0;
	var->yoffset = 0;
	var->width = lcd->width_mm; //sehun
	var->height = lcd->height_mm;
	var->transp.length = 0;

	fix->line_length = (var->xres_virtual * var->bits_per_pixel / 8);
	fix->smem_len = fix->line_length * var->yres_virtual;

	var->nonstd = 0;
	var->activate = FB_ACTIVATE_NOW;
	var->vmode = FB_VMODE_NONINTERLACED;
	var->hsync_len = timing->h_sw;
	var->vsync_len = timing->v_sw;
	var->left_margin = timing->h_fp;
	var->right_margin = timing->h_bp;
	var->upper_margin = timing->v_fp;
	var->lower_margin = timing->v_bp;

	if (pdata->interface_mode == FIMD_CPU_INTERFACE) {
		struct s3cfb_cpu_timing *cpu_timing = NULL;

		cpu_timing = &lcd->cpu_timing;
		if (cpu_timing == NULL)
			dev_warn(fbdev->dev, "cpu_timing is NULL.\n");

		/* 
		 * calculate vclk for lcd panel
		 * based on mipi dsi(command mode using cpu interface).
		 */
		var->pixclock = lcd->width * lcd->height * lcd->freq *
			(cpu_timing->cs_setup + cpu_timing->wr_setup +
			cpu_timing->wr_act + cpu_timing->wr_hold + 1);

		dev_dbg(fbdev->dev, "width = %d, height = %d, freq = %d\n",
			lcd->width, lcd->height, lcd->freq);
		dev_dbg(fbdev->dev, "cs_setup = %d, wr_setup = %d ",
			cpu_timing->cs_setup, cpu_timing->wr_setup);
		dev_dbg(fbdev->dev, "wr_act = %d, wr_hold = %d\n",
			cpu_timing->wr_act, cpu_timing->wr_hold);
	} else {
		if (pdata->sub_lcd_enabled) {
			/* 
			 * pixel clock for DUAL LCD should be setting
			 * as ONE lcd panel.
			 * */
			var->pixclock = lcd->freq * (var->left_margin +
				var->right_margin +
				var->hsync_len + (var->xres / 2)) *
				(var->upper_margin + var->lower_margin +
				var->vsync_len + (var->yres));
		} else {
			var->pixclock = lcd->freq * (var->left_margin +
				var->right_margin +
				var->hsync_len + var->xres) *
				(var->upper_margin + var->lower_margin +
				var->vsync_len + var->yres);
		}
	}

	dev_dbg(fbdev->dev, "pixclock: %d\n", var->pixclock);

	s3cfb_set_bitfield(var);
	s3cfb_set_alpha_info(var, win);

	return 0;
}

int s3cfb_get_default_win(void)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);

	if (pdata == NULL) {
	    dev_err(fbdev->dev, "failed to get defualt window number.\n");
	    return -EFAULT;
	}

	return pdata->default_win;
}
EXPORT_SYMBOL(s3cfb_get_default_win);

void s3cfb_trigger(void)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);

	if (pdata == NULL) {
	    dev_err(fbdev->dev, "failed to get defualt window number.\n");
	    return;
	}
	s3cfb_set_trigger(fbdev);
}
EXPORT_SYMBOL(s3cfb_set_trigger);

u8 s3cfb_frame_done(void)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);

	if (pdata == NULL) {
	    dev_err(fbdev->dev, "failed to get defualt window number.\n");
	    return 0;
	}

	return s3cfb_is_frame_done(fbdev);
}

static int s3cfb_alloc_framebuffer(void)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	int ret, i;

	/* alloc for framebuffers */
	fbdev->fb = kmalloc(pdata->nr_wins *
			    sizeof(struct fb_info *), GFP_KERNEL);
	if (!fbdev->fb) {
		dev_err(fbdev->dev, "not enough memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	/* alloc for each framebuffer */
	for (i = 0; i < pdata->nr_wins; i++) {
		fbdev->fb[i] = framebuffer_alloc(sizeof(struct s3cfb_window),
						 fbdev->dev);
		if (!fbdev->fb[i]) {
			dev_err(fbdev->dev, "not enough memory\n");
			ret = -ENOMEM;
			goto err_alloc_fb;
		}

		ret = s3cfb_init_fbinfo(i);
		if (ret) {
			dev_err(fbdev->dev,
				"failed to allocate memory for fb%d\n", i);
			ret = -ENOMEM;
			goto err_alloc_fb;
		}

		if (i == pdata->default_win) {
			if (s3cfb_map_video_memory(fbdev->fb[i])) {
				dev_err(fbdev->dev,
					"failed to map video memory "
					"for default window (%d)\n", i);
				ret = -ENOMEM;
				goto err_alloc_fb;
			}
		}
	}

	return 0;

err_alloc_fb:
	for (i = 0; i < pdata->nr_wins; i++) {
		if (fbdev->fb[i])
			framebuffer_release(fbdev->fb[i]);
	}

	kfree(fbdev->fb);

err_alloc:
	return ret;
}

static int s3cfb_draw_logo(struct fb_info *fb)
{
        struct fb_fix_screeninfo *fix = &fb->fix;
        struct fb_var_screeninfo *var = &fb->var;
        struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);

        memcpy(fbdev->fb[pdata->default_win]->screen_base,
               LOGO_RGB24, fix->line_length * var->yres);

        return 0;
}

int s3cfb_register_framebuffer(void)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	int ret, i;

	for (i = 0; i < pdata->nr_wins; i++) {
		ret = register_framebuffer(fbdev->fb[i]);
		if (ret) {
			dev_err(fbdev->dev, "failed to register "
				"framebuffer device\n");
			return -EINVAL;
		}
//			s3cfb_draw_logo(fbdev->fb[i]);
#ifndef CONFIG_FRAMEBUFFER_CONSOLE
		if (i == pdata->default_win) {
			s3cfb_check_var(&fbdev->fb[i]->var, fbdev->fb[i]);
			s3cfb_set_par(fbdev->fb[i]);
			s3cfb_draw_logo(fbdev->fb[i]);
		}
#endif
	}

	return 0;
}

static int s3cfb_sysfs_show_win_power(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct s3c_platform_fb *pdata = to_fb_plat(dev);
	struct s3cfb_window *win;
	char temp[16];
	int i;

	for (i = 0; i < pdata->nr_wins; i++) {
		win = fbdev->fb[i]->par;
		sprintf(temp, "[fb%d] %s\n", i, win->enabled ? "on" : "off");
		strcat(buf, temp);
	}

	return strlen(buf);
}

static int s3cfb_sysfs_store_win_power(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t len)
{
	struct s3c_platform_fb *pdata = to_fb_plat(dev);
	char temp[4] = { 0, };
	const char *p = buf;
	int id, to;

	while (*p != '\0') {
		if (!isspace(*p))
			strncat(temp, p, 1);
		p++;
	}

	if (strlen(temp) != 2)
		return -EINVAL;

		id = simple_strtoul(temp, NULL, 10) / 10;
		to = simple_strtoul(temp, NULL, 10) % 10;

	if (id < 0 || id > pdata->nr_wins)
		return -EINVAL;

	if (to != 0 && to != 1)
		return -EINVAL;

	if (to == 0)
		s3cfb_disable_window(id);
	else
		s3cfb_enable_window(id);

	return len;
}

static DEVICE_ATTR(win_power, 0644,
		s3cfb_sysfs_show_win_power, s3cfb_sysfs_store_win_power);

/* sysfs export of baclight control */
static int s3cfb_sysfs_show_lcd_power(struct device *dev, struct device_attribute *attr, char *buf)
{
	return ;
}

static int s3cfb_sysfs_store_lcd_power(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	if (len < 1)
		return -EINVAL;

	if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
		ams397g201_sleep_out(dev);
	else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
	{
		disable_irq( LCD_ESD_INT);
		ams397g201_sleep_in(dev);
	}
	else
		return -EINVAL;

	return len;
}

static DEVICE_ATTR(lcd_power, 0777,
			s3cfb_sysfs_show_lcd_power,
			s3cfb_sysfs_store_lcd_power);

char lcd_wakeup_source_reg = 0;
//lcd_wakeup_source_reg = 0 ->  no-key wake up source 
//lcd_wakeup_source_reg = 1 ->  key wake up source 
static int s3cfb_sysfs_show_key_wakeup(struct device *dev, struct device_attribute *attr, char *buf)
{
       pr_err("%s =%d \n",__func__);
	   
	return sprintf(buf,"%u\n",lcd_wakeup_source_reg);
}

static int s3cfb_sysfs_store_key_wakeup(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	int value;
	
	sscanf(buf, "%d", &value);

	if(value==0 )
	{
		lcd_wakeup_source_reg = 0;
	}
	else if(value==1)
	{
		lcd_wakeup_source_reg = 1;
	}

       pr_err("%s =%d \n",__func__);

	return len;
}


static DEVICE_ATTR(key_wakeup, 0777,
			s3cfb_sysfs_show_key_wakeup,
			s3cfb_sysfs_store_key_wakeup);
//mipi_temp
#if 0 
static int mdnie_sysfs_show_mdnie_mode(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	char temp[16];

	struct s3c_platform_fb *pdata = to_fb_plat(dev);

	if (pdata->mdnie_mode == MDNIE_UI)
		sprintf(temp, "MODE-UI");
	else if (pdata->mdnie_mode == MDNIE_CAMERA)
		sprintf(temp, "MODE-CAMERA");
	else if (pdata->mdnie_mode == MDNIE_MOVIE)
		sprintf(temp, "MODE-MOVIE");

	dev_info(dev, "%s\n", temp);

	return strlen(buf);
}

static int mdnie_sysfs_store_mdnie_mode(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t len)
{
	struct s3c_platform_fb *pdata = to_fb_plat(dev);

	pdata->mdnie_mode = simple_strtoul(buf, NULL, 10);

	pdata->mdnie_set_mode((unsigned int) pdata->mdnie_mmio_base, (enum e_mdnie_mode) pdata->mdnie_mode);

	return len;
}

static DEVICE_ATTR(mdnie_mode, 0644,
		mdnie_sysfs_show_mdnie_mode, mdnie_sysfs_store_mdnie_mode);

#endif

static int s3cfb_sysfs_show_refresh(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct s3cfb_lcd *lcd = fbdev->lcd;

	dev_info(dev, "%d\n", lcd->freq);

	return strlen(buf);
}

static int s3cfb_sysfs_store_refresh(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t len)
{
	u32 refresh;
	struct s3cfb_lcd *lcd = fbdev->lcd;
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	struct fb_info *fb = fbdev->fb[pdata->default_win];
	struct fb_var_screeninfo *var = &fb->var;

	refresh = simple_strtoul(buf, NULL, 10);

	lcd->freq = refresh;

	if (pdata->interface_mode == FIMD_CPU_INTERFACE) {
		struct s3cfb_cpu_timing *cpu_timing = NULL;

		cpu_timing = &lcd->cpu_timing;
		if (cpu_timing == NULL)
			dev_warn(fbdev->dev, "cpu_timing is NULL.\n");

		var->pixclock = lcd->width * lcd->height * lcd->freq *
			(cpu_timing->cs_setup + cpu_timing->wr_setup +
			cpu_timing->wr_act + cpu_timing->wr_hold + 1);

		dev_dbg(fbdev->dev, "width = %d, height = %d, freq = %d\n",
			lcd->width, lcd->height, lcd->freq);
		dev_dbg(fbdev->dev, "cs_setup = %d, wr_setup = %d ",
			cpu_timing->cs_setup, cpu_timing->wr_setup);
		dev_dbg(fbdev->dev, "wr_act = %d, wr_hold = %d\n",
			cpu_timing->wr_act, cpu_timing->wr_hold);
	} else {
		var->pixclock = lcd->freq * (var->left_margin + var->right_margin +
			var->hsync_len + var->xres) *
			(var->upper_margin + var->lower_margin +
			var->vsync_len + var->yres);
	}

	lcd->freq = refresh;

	dev_info(fbdev->dev, "refresh = %d, pixel clock = %d\n",
		refresh, var->pixclock);

	s3cfb_set_clock(fbdev, var->pixclock);

	return len;
}

static DEVICE_ATTR(refresh, 0644,
		s3cfb_sysfs_show_refresh, s3cfb_sysfs_store_refresh);

#ifdef CONFIG_CPU_FREQ
/*
 * CPU clock speed change handler.  We need to adjust the LCD timing
 * parameters when the CPU clock is adjusted by the power management
 * subsystem.
 */
static int
s3cfb_freq_transition(struct notifier_block *nb, unsigned long val,
			 void *data)
{
	switch (val) {
	case CPUFREQ_PRECHANGE:

		break;

	case CPUFREQ_POSTCHANGE:

		break;
	default:
		break;
	}

	return 0;
}

static int
s3cfb_freq_policy(struct notifier_block *nb, unsigned long val,
		     void *data)
{
	switch (val) {
	case CPUFREQ_ADJUST:
	case CPUFREQ_INCOMPATIBLE:
		break;
	case CPUFREQ_NOTIFY:
		break;
	}
	return 0;
}
#endif

static void s3cfb_init_mdnie_path(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);

	/* display path is mDNIE. */
	if (pdata->set_display_path)
		pdata->set_display_path(1);

	/* set dual rgb to mDNIe*/
	s3cfb_set_dualrgb_mode(fbdev, MODE_MIE_MDNIE);
}

static void s3cfb_set_mdnie_fimd_lite(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);

	pdata->set_mdnie_clock(pdata->mdnie_clk, 1);

	/* set free run mode. */
	s3cfb_set_free_run(fbdev, 1);

	/* set vidout mode to WB and RGB. */
	s3cfb_set_vidout(fbdev, VIDOUT_WB_RGB);

	if (pdata->init_mdnie)
		pdata->init_mdnie((unsigned int) pdata->mdnie_mmio_base, fbdev->lcd->width,
			fbdev->lcd->height);

	if (pdata->start_ielcd_logic)
		pdata->start_ielcd_logic((unsigned int) pdata->ielcd_mmio_base);

	if (pdata->init_ielcd)
		pdata->init_ielcd((unsigned int) pdata->ielcd_mmio_base, (void *) fbdev->lcd,
			(void *) fbdev->clock);

	/* it should be delayed before FIMD display on. */
	msleep(50);
}


static void s3cfb_set_mipi_re_dsi(struct device *dev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&dev);

	if (pdata->interface_mode == FIMD_CPU_INTERFACE) {
		fbdev->output = OUTPUT_I80LDI0;
		fbdev->rgb_mode = MODE_RGB_P;

		/* set cpu interface timing. */
		s3cfb_set_cpu_interface_timing(fbdev, DDI_MAIN_LCD);
		s3cfb_set_auto_cmd_rate(fbdev, DISABLE_AUTO_FRM, DDI_MAIN_LCD);
		s3cfb_set_auto_cmd_rate(fbdev, DISABLE_AUTO_FRM, DDI_SUB_LCD);
	} else {
		fbdev->output = OUTPUT_RGB;
		fbdev->rgb_mode = MODE_RGB_P;

		/* set video mode timing and polarity. */
		s3cfb_set_polarity(fbdev);
		s3cfb_set_timing(fbdev);
		s3cfb_set_lcd_size(fbdev);
	}

	/* set display mode. */
	s3cfb_set_display_mode(fbdev);

	/* set output mode. */
	s3cfb_set_output(fbdev);

	s3cfb_enable_mipi_dsi_mode(fbdev, 1);

	dev_dbg(fbdev->dev, "vidcon0 = %x, vidcon1 = %x, vidcon2 = %x\n",
		readl(fbdev->regs + S3C_VIDCON0), readl(fbdev->regs + S3C_VIDCON1),
		readl(fbdev->regs + S3C_VIDCON2));
}

static void s3cfb_set_mipi_dsi(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);

	if (pdata->interface_mode == FIMD_CPU_INTERFACE) {
		fbdev->output = OUTPUT_I80LDI0;
		fbdev->rgb_mode = MODE_RGB_P;

		/* set cpu interface timing. */
		s3cfb_set_cpu_interface_timing(fbdev, DDI_MAIN_LCD);
		s3cfb_set_auto_cmd_rate(fbdev, DISABLE_AUTO_FRM, DDI_MAIN_LCD);
		s3cfb_set_auto_cmd_rate(fbdev, DISABLE_AUTO_FRM, DDI_SUB_LCD);
	} else {
		fbdev->output = OUTPUT_RGB;
		fbdev->rgb_mode = MODE_RGB_P;

		/* set video mode timing and polarity. */
		s3cfb_set_polarity(fbdev);
		s3cfb_set_timing(fbdev);
		s3cfb_set_lcd_size(fbdev);
	}

	/* set display mode. */
	s3cfb_set_display_mode(fbdev);

	/* set output mode. */
	s3cfb_set_output(fbdev);

	s3cfb_enable_mipi_dsi_mode(fbdev, 1);

	dev_dbg(fbdev->dev, "vidcon0 = %x, vidcon1 = %x, vidcon2 = %x\n",
		readl(fbdev->regs + S3C_VIDCON0), readl(fbdev->regs + S3C_VIDCON1),
		readl(fbdev->regs + S3C_VIDCON2));
}
extern int s3cfb_set_i80_framedone_interrupt(struct s3cfb_global *ctrl, unsigned char chipselect, int enable);
static int s3cfb_probe(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata;
	struct resource *res;
	int ret = 0;

	/* initialzing global structure */
	DBG("S3CFB_PROBE_1 \n");
	fbdev = kzalloc(sizeof(struct s3cfb_global), GFP_KERNEL);
	if (!fbdev) {
		dev_err(fbdev->dev, "failed to allocate for "
			"global fb structure\n");
		goto err_global;
	}

	fbdev->dev = &pdev->dev;

	pdata = to_fb_plat(&pdev->dev);
	fbdev->platform_data = (struct s3c_platform_fb *) pdata;

	fbdev->lcd = (struct s3cfb_lcd *) pdata->lcd_data;
	if (!fbdev->lcd) {
		dev_err(fbdev->dev, "failed to get lcd panel data.\n");
		ret = -EFAULT;
		goto err_global;
	}

	/* clock */
	DBG("S3CFB_PROBE_2 \n");
	fbdev->clock = clk_get(&pdev->dev, pdata->clk_name);
	if (IS_ERR(fbdev->clock)) {
		dev_err(fbdev->dev, "failed to get fimd clock source\n");
		ret = -EINVAL;
		goto err_clk;
	}

	clk_enable(fbdev->clock);

	/* io memory */
	DBG("S3CFB_PROBE_3 \n");
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(fbdev->dev, "failed to get io memory region\n");
		ret = -EINVAL;
		goto err_io;
	}

	/* request mem region */
	DBG("S3CFB_PROBE_4 \n");	
	res = request_mem_region(res->start,
				 res->end - res->start + 1, pdev->name);
	if (!res) {
		dev_err(fbdev->dev, "failed to request io memory region\n");
		ret = -EINVAL;
		goto err_io;
	}

	/* ioremap for register block */
	DBG("S3CFB_PROBE_5 \n");	
	fbdev->regs = ioremap(res->start, res->end - res->start + 1);
	if (!fbdev->regs) {
		dev_err(fbdev->dev, "failed to remap io region\n");
		ret = -EINVAL;
		goto err_io;
	}

	/* irq */
	DBG("S3CFB_PROBE_6 \n");	
	fbdev->irq = platform_get_irq(pdev, 0);
	if (request_irq(fbdev->irq, s3cfb_irq_frame, IRQF_SHARED,
			pdev->name, fbdev)) {
		dev_err(fbdev->dev, "request_irq failed\n");
		ret = -EINVAL;
		goto err_irq;
	}

//	s3cfb_display_off(fbdev); //####	
#if 0
	DBG("S3CFB_PROBE_7 \n");
	s3cfb_set_vsync_interrupt(fbdev, 1);
	DBG("S3CFB_PROBE_8 \n");	
	s3cfb_set_global_interrupt(fbdev, 1);
#endif
#if 1 //####

	/* in case of no mDNIe mode. */
	if (!pdata->mdnie_is_enabled) {
		/* display path is FIMD. */
		if (pdata->set_display_path)
			pdata->set_display_path(2);
	/* in case of mDNIe mode. */
	} else {
		pdata->mdnie_mmio_base = ioremap(pdata->mdnie_phy_base, SZ_1M);
		if (!pdata->mdnie_mmio_base) {
			dev_err(fbdev->dev, "failed to remap io region\n");
			goto err_mdnie;
		}

		pdata->ielcd_mmio_base = ioremap(pdata->ielcd_phy_base, SZ_1M);
		if (!pdata->ielcd_mmio_base) {
			dev_err(fbdev->dev, "failed to remap io region\n");
			goto err_ielcd;
		}

		pdata->mdnie_clk = (void *) clk_get(&pdev->dev, pdata->mdnie_clk_name);
		if (IS_ERR(pdata->mdnie_clk)) {
			dev_err(fbdev->dev, "failed to get mDNIe clock source\n");
			goto err_mdnie_clk;
		}

		if (pdata->set_mdnie_clock)
			pdata->set_mdnie_clock(pdata->mdnie_clk, 1);

		/* set mDNIe path. */
		s3cfb_init_mdnie_path(pdev);
	}
#endif

err_mdnie_clk:
	iounmap(pdata->ielcd_mmio_base);

err_ielcd:
	iounmap(pdata->mdnie_mmio_base);

err_mdnie:
	/* init global */
	DBG("S3CFB_PROBE_8 \n");	
#if 1 //@@
	s3cfb_init_global();
#endif

	/* prepare memory */
	if (s3cfb_alloc_framebuffer())
		goto err_alloc;

	if (s3cfb_register_framebuffer())
		goto err_alloc;
#if 1 //@@
	DBG("S3CFB_PROBE_9 \n");
	s3cfb_set_clock(fbdev, 0);
	DBG("S3CFB_PROBE_10 \n");
	s3cfb_enable_window(pdata->default_win);
#endif

#if 0 //mipi_temp
	/* set mDNIe and FIMD-lite. */
	if (pdata->mdnie_is_enabled) {
		s3cfb_set_mdnie_fimd_lite(pdev);

		ret = device_create_file(&(pdev->dev), &dev_attr_mdnie_mode);
		if (ret < 0)
			dev_err(fbdev->dev, "failed to add sysfs entries\n");
	}
#endif

	/* enable MIPI DSI */
	DBG("S3CFB_PROBE_11 \n");
	if (pdata->mipi_is_enabled)
		s3cfb_set_mipi_dsi(pdev);

	DBG("S3CFB_PROBE_12 \n");
//	s3cfb_display_on(fbdev);
	s3cfb_display_off(fbdev); //@@

	ret = device_create_file(&(pdev->dev), &dev_attr_key_wakeup);
	if (ret < 0)
		printk(KERN_WARNING "s3cfb: failed to add entries\n");

	ret = device_create_file(&(pdev->dev), &dev_attr_lcd_power);
	if (ret < 0)
		printk(KERN_WARNING "s3cfb: failed to add entries\n");
	
	ret = device_create_file(&(pdev->dev), &dev_attr_win_power);
	if (ret < 0)
		dev_err(fbdev->dev, "failed to add sysfs entries\n");

	ret = device_create_file(&(pdev->dev), &dev_attr_refresh);
	if (ret < 0)
		dev_err(fbdev->dev, "failed to add sysfs entries\n");

	dev_info(fbdev->dev, "registered successfully\n");

#ifdef CONFIG_CPU_FREQ
	fbdev->freq_transition.notifier_call = s3cfb_freq_transition;
	fbdev->freq_policy.notifier_call = s3cfb_freq_policy;
	cpufreq_register_notifier(&fbdev->freq_transition, CPUFREQ_TRANSITION_NOTIFIER);
	cpufreq_register_notifier(&fbdev->freq_policy, CPUFREQ_POLICY_NOTIFIER);
#endif

#ifdef CONFIG_HAS_WAKELOCK
#ifdef CONFIG_HAS_EARLYSUSPEND
	fbdev->early_suspend.suspend = s3cfb_early_suspend;
	fbdev->early_suspend.resume = s3cfb_late_resume;
	fbdev->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	//if, is in USER_SLEEP status and no active auto expiring wake lock
	//if (has_wake_lock(WAKE_LOCK_SUSPEND) == 0 && get_suspend_state() == PM_SUSPEND_ON)
	register_early_suspend(&fbdev->early_suspend);
#endif
#endif

#ifdef CONFIG_FB_S3C_PROGRESS_BAR
	ret = fbutil_init();
	if (ret < 0)
		dev_err(fbdev->dev, "failed to initialize fbutil.\n");
	else
		/* delay time is 100ms. */
		fbutil_thread_run(100);
#endif
	return 0;

err_alloc:
	free_irq(fbdev->irq, fbdev);

err_irq:
	iounmap(fbdev->regs);

err_io:
	clk_disable(fbdev->clock);

err_clk:
	clk_put(fbdev->clock);

err_global:
	return ret;
}

int s3cfb_change_mode(unsigned int id, unsigned int lcd_mode)
{
	struct s3c_platform_fb *pdata = NULL;
	struct fb_info *fb;
	struct s3cfb_window *win;
	int ret = 0;

	pdata = to_fb_plat(fbdev->dev);

	s3cfb_disable_window(id);
	clk_disable(fbdev->clock);

	if (lcd_mode == DUAL_LCD_MODE) {
		pdata->sub_lcd_enabled = 1;
		fbdev->lcd = (struct s3cfb_lcd *) pdata->dual_lcd;
	} else {
		pdata->sub_lcd_enabled = 0;
		fbdev->lcd = (struct s3cfb_lcd *) pdata->single_lcd;
	}

	s3cfb_init_fbinfo(id);
	clk_enable(fbdev->clock);

	s3cfb_init_global();
	s3cfb_set_clock(fbdev, 0);
	s3cfb_display_on(fbdev);

	fb = fbdev->fb[id];
	win = fb->par;

	s3cfb_set_par(fb);

	/* for test. */
	memset(fbdev->fb[id]->screen_base, 0,
		fbdev->lcd->width * fbdev->lcd->height *
		fbdev->fb[id]->var.bits_per_pixel / 8);

//mipi_temp	draw_samsung_logo(pdata);

	s3cfb_enable_window(id);

	return ret;
}

static int s3cfb_remove(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	struct fb_info *fb;
	int i;

#ifdef CONFIG_FB_S3C_PROGRESS_BAR
	fbutil_exit();
#endif
#ifdef CONFIG_HAS_WAKELOCK
#ifdef CONFIG_HAS_EARLYSUSPEND
        unregister_early_suspend(&fbdev->early_suspend);
#endif
#endif

	free_irq(fbdev->irq, fbdev);
	iounmap(fbdev->regs);
	clk_disable(fbdev->clock);
	clk_put(fbdev->clock);

	if (pdata->mdnie_is_enabled) {
		pdata->set_mdnie_clock(pdata->mdnie_clk, 0);
		clk_put(pdata->mdnie_clk);

		if (pdata->mdnie_mmio_base)
			iounmap(pdata->mdnie_mmio_base);

		if (pdata->ielcd_mmio_base)
			iounmap(pdata->ielcd_mmio_base);
	}

	for (i = 0; i < pdata->nr_wins; i++) {
		fb = fbdev->fb[i];

		/* free if exists */
		if (fb) {
			s3cfb_unmap_video_memory(fb);
			s3cfb_set_buffer_address(fbdev, i);
			framebuffer_release(fb);
		}
	}

	kfree(fbdev->fb);
	kfree(fbdev);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct sleep_save lcd_fb_save[] = {
 SAVE_ITEM(S3C_VIDW00ADD0B0)
};

static struct sleep_save lcd_save[] = {
 SAVE_ITEM(S3C_WINCON0),
 SAVE_ITEM(S3C_WINSHMAP), 
 SAVE_ITEM(S3C_VIDOSD0B),
 SAVE_ITEM(S3C_VIDW00ADD0B0),
 SAVE_ITEM(S3C_VIDW00ADD1B0),
 SAVE_ITEM(S3C_VIDW00ADD2),
 SAVE_ITEM(S3C_VIDINTCON0),
};

/*
static struct sleep_save lcd_save[] = {
 SAVE_ITEM(S3C_VIDCON0),
 SAVE_ITEM(S3C_VIDCON1),
 SAVE_ITEM(S3C_VIDCON2), 
 SAVE_ITEM(S3C_PRTCON), 
 
 SAVE_ITEM(S3C_VIDTCON0),
 SAVE_ITEM(S3C_VIDTCON1),
 SAVE_ITEM(S3C_VIDTCON2),

 SAVE_ITEM(S3C_WINCON0),
 SAVE_ITEM(S3C_WINCON1),
 SAVE_ITEM(S3C_WINCON2),
 SAVE_ITEM(S3C_WINCON3),
 SAVE_ITEM(S3C_WINCON4),
 SAVE_ITEM(S3C_WINSHMAP), 
 SAVE_ITEM(S3C_VIDOSD0A),
 SAVE_ITEM(S3C_VIDOSD0B),
 SAVE_ITEM(S3C_VIDOSD0C),

 SAVE_ITEM(S3C_VIDOSD1A),
 SAVE_ITEM(S3C_VIDOSD1B),
 SAVE_ITEM(S3C_VIDOSD1C),
 SAVE_ITEM(S3C_VIDOSD1D),

 SAVE_ITEM(S3C_VIDOSD2A),
 SAVE_ITEM(S3C_VIDOSD2B),
 SAVE_ITEM(S3C_VIDOSD2C),
 SAVE_ITEM(S3C_VIDOSD2D),

 SAVE_ITEM(S3C_VIDOSD3A),
 SAVE_ITEM(S3C_VIDOSD3B),
 SAVE_ITEM(S3C_VIDOSD3C),

 SAVE_ITEM(S3C_VIDOSD4A),
 SAVE_ITEM(S3C_VIDOSD4B),
 SAVE_ITEM(S3C_VIDOSD4C),

 SAVE_ITEM(S3C_VIDW00ADD0B0),
 SAVE_ITEM(S3C_VIDW00ADD0B1),
 SAVE_ITEM(S3C_VIDW01ADD0B0),
 SAVE_ITEM(S3C_VIDW01ADD0B1),
 SAVE_ITEM(S3C_VIDW02ADD0),
 SAVE_ITEM(S3C_VIDW03ADD0),
 SAVE_ITEM(S3C_VIDW04ADD0),
 SAVE_ITEM(S3C_VIDW00ADD1B0),
 SAVE_ITEM(S3C_VIDW00ADD1B1),
 SAVE_ITEM(S3C_VIDW01ADD1B0),
 SAVE_ITEM(S3C_VIDW01ADD1B1),
 SAVE_ITEM(S3C_VIDW02ADD1),
 SAVE_ITEM(S3C_VIDW03ADD1),
 SAVE_ITEM(S3C_VIDW04ADD1),
 SAVE_ITEM(S3C_VIDW00ADD2),
 SAVE_ITEM(S3C_VIDW01ADD2),
 SAVE_ITEM(S3C_VIDW02ADD2),
 SAVE_ITEM(S3C_VIDW03ADD2),
 SAVE_ITEM(S3C_VIDW04ADD2),

 SAVE_ITEM(S3C_VP1TCON0),
 SAVE_ITEM(S3C_VP1TCON1),

 SAVE_ITEM(S3C_VIDINTCON0),
 SAVE_ITEM(S3C_VIDINTCON1),
 
 SAVE_ITEM(S3C_W1KEYCON0),
 SAVE_ITEM(S3C_W1KEYCON1),
 SAVE_ITEM(S3C_W2KEYCON0),
 SAVE_ITEM(S3C_W2KEYCON1),
 SAVE_ITEM(S3C_W3KEYCON0),
 SAVE_ITEM(S3C_W3KEYCON1),
 SAVE_ITEM(S3C_W4KEYCON0),
 SAVE_ITEM(S3C_W4KEYCON1),

 SAVE_ITEM(S3C_W1KEYALPHA),
 SAVE_ITEM(S3C_W2KEYALPHA),
 SAVE_ITEM(S3C_W3KEYALPHA),
 SAVE_ITEM(S3C_W4KEYALPHA),

 SAVE_ITEM(S3C_DITHMODE),
 
 SAVE_ITEM(S3C_WIN0MAP),
 SAVE_ITEM(S3C_WIN1MAP),
 SAVE_ITEM(S3C_WIN2MAP),
 SAVE_ITEM(S3C_WIN3MAP),
 SAVE_ITEM(S3C_WIN4MAP),
 
 SAVE_ITEM(S3C_WPALCON_H),
 SAVE_ITEM(S3C_WPALCON_L), 
 SAVE_ITEM(S3C_TRIGCON),
 SAVE_ITEM(S3C_I80IFCONA0),
 SAVE_ITEM(S3C_I80IFCONA1),
 SAVE_ITEM(S3C_I80IFCONB0),
 SAVE_ITEM(S3C_I80IFCONB1),
 SAVE_ITEM(S3C_LDI_CMDCON0),
 SAVE_ITEM(S3C_LDI_CMDCON1),
 SAVE_ITEM(S3C_SIFCCON0),
 SAVE_ITEM(S3C_SIFCCON1),
 SAVE_ITEM(S3C_SIFCCON2),

 SAVE_ITEM(S3C_VIDW0ALPHA0),
 SAVE_ITEM(S3C_VIDW0ALPHA1),
 SAVE_ITEM(S3C_VIDW1ALPHA0),
 SAVE_ITEM(S3C_VIDW1ALPHA1),
 SAVE_ITEM(S3C_VIDW2ALPHA0),
 SAVE_ITEM(S3C_VIDW2ALPHA1),
 SAVE_ITEM(S3C_VIDW3ALPHA0),
 SAVE_ITEM(S3C_VIDW3ALPHA1),
 SAVE_ITEM(S3C_VIDW4ALPHA0),
 SAVE_ITEM(S3C_VIDW4ALPHA1),
  
 SAVE_ITEM(S3C_BLENDEQ1),
 SAVE_ITEM(S3C_BLENDEQ2), 
 SAVE_ITEM(S3C_BLENDEQ3),  
 SAVE_ITEM(S3C_BLENDEQ4),
 SAVE_ITEM(S3C_BLENDCON),
 SAVE_ITEM(S3C_DUALRGB), 
  
 //SAVE_ITEM(S3C_HOSTIFB_MIFPCON),


 SAVE_ITEM(S3C64XX_GPICON),
 SAVE_ITEM(S3C64XX_GPIDAT),
 SAVE_ITEM(S3C64XX_GPIPUD),
 SAVE_ITEM(S3C64XX_GPJCON),
 SAVE_ITEM(S3C64XX_GPJDAT),
 SAVE_ITEM(S3C64XX_GPJPUD),

};
*/
static u32 fb_date = 0;
static u32 fimd_date[7] ={
0x00008035,
0x00000001,
0x000EFBDF,
0x4fe00000,
0x00FD1000,
0x00000780,
0x00001001,
};

void s5pc11x_lcd_fb_do_save(struct sleep_save *ptr, int count)
{
	fb_date = __raw_readl((u32 )ptr->reg + fbdev->regs);
}

void s5pc11x_lcd_fb_do_restore(struct sleep_save *ptr, int count)
{
	__raw_writel(fb_date, (u32 )ptr->reg + fbdev->regs);
}

void s5pc11x_lcd_do_restore(struct sleep_save *ptr, int count)
{
	char i=0;

	for (; count > 0; count--, ptr++) {
		__raw_writel(fimd_date[i], (u32 )ptr->reg + fbdev->regs);
		i++;
	}
}
int s3cfb_early_suspend(struct early_suspend *h)
{
	struct s3cfb_global *info = container_of(h,struct s3cfb_global,early_suspend);
	struct s3c_platform_fb *pdata = to_fb_plat(info->dev);
	struct s3cfb_window *win;
	int i;

	pr_err("[FIMD][%s] \n",__func__);
	disable_irq(S3C_GPIOINT(F0,1));
	msleep(20);

      	s5pc11x_lcd_fb_do_save(lcd_fb_save, ARRAY_SIZE(lcd_fb_save));
	s3cfb_display_off(info);
	clk_disable(info->clock);

	disable_irq( LCD_ESD_INT);
	return 0;
}

extern void ams397g201_set_tear_on(void);

int s3cfb_late_resume(struct early_suspend *h)
{
	struct s3cfb_global *info = container_of(h,struct s3cfb_global,early_suspend);

	struct s3c_platform_fb *pdata = to_fb_plat(info->dev);
	struct fb_info *fb;
	struct s3cfb_window *win;
	int i;
	struct platform_device *pdev = to_platform_device(info->dev);
	struct resource *res;
	int ret = 0;

	static int x;

	pr_err("[FIMD][%s] \n",__func__);

	/* clock */
//	DBG("S3CFB_RESUME_1 \n");
	fbdev->clock = clk_get(&pdev->dev, pdata->clk_name);
	if (IS_ERR(fbdev->clock)) {
		dev_err(fbdev->dev, "failed to get fimd clock source\n");
		ret = -EINVAL;
		goto err_clk;
	}

	clk_enable(fbdev->clock);
	
//	DBG("S3CFB_RESUME_2 \n");
	s3cfb_set_vsync_interrupt(fbdev, 1);
	s3cfb_set_global_interrupt(fbdev, 1);


	/* init global */
//	DBG("S3CFB_RESUME_3 \n");	
	s3cfb_init_global();

	s3cfb_set_clock(fbdev, 0);
	s3cfb_enable_window(pdata->default_win);


	/* enable MIPI DSI */
//	DBG("S3CFB_RESUME_4 \n");
	if (pdata->mipi_is_enabled)
		s3cfb_set_mipi_dsi(pdev);

 	s3cfb_display_on(fbdev);

 	s5pc11x_lcd_do_restore(lcd_save, ARRAY_SIZE(lcd_save));
	s5pc11x_lcd_fb_do_restore(lcd_fb_save, ARRAY_SIZE(lcd_fb_save));
	

	ams397g201_set_tear_on();

	s3cfb_trigger();	

	enable_irq(S3C_GPIOINT(F0,1));
	enable_irq( LCD_ESD_INT);	

//sehun_temp 	lcd_pannel_on();


	return 0;

err_alloc:
	free_irq(fbdev->irq, fbdev);

err_irq:
	iounmap(fbdev->regs);

err_io:
	clk_disable(fbdev->clock);

err_clk:
	clk_put(fbdev->clock);

err_global:
	return ret;

}
#else  // else CONFIG_HAS_EARLYSUSPEND
#ifdef CONFIG_PM
int s3cfb_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);

	s3cfb_display_off(fbdev);
	clk_disable(fbdev->clock);

	if (pdata->mdnie_is_enabled)
		pdata->set_mdnie_clock(pdata->mdnie_clk, 0);

	return 0;
}

int s3cfb_resume(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	struct fb_info *fb;
	struct s3cfb_window *win;
	int i;

	dev_dbg(fbdev->dev, "wake up from suspend\n");

	clk_enable(fbdev->clock);

	if (pdata->mdnie_is_enabled) {
		pdata->set_mdnie_clock(pdata->mdnie_clk, 1);

		s3cfb_init_mdnie_path(pdev);
	}

	s3cfb_init_global();
	s3cfb_set_clock(fbdev, 0);

	for (i = 0; i < pdata->nr_wins; i++) {
		fb = fbdev->fb[i];
		win = fb->par;
		if ((win->owner == DMA_MEM_FIMD) && (win->enabled)) {
			s3cfb_set_win_params(win->id);
			s3cfb_enable_window(win->id);
		}
	}

	if (pdata->mdnie_is_enabled)
		s3cfb_set_mdnie_fimd_lite(pdev);

	/* enable MIPI DSI */
	if (pdata->mipi_is_enabled)
		s3cfb_set_mipi_dsi(pdev);

	s3cfb_display_on(fbdev);

	return 0;
}
#else
#define s3cfb_suspend NULL
#define s3cfb_resume NULL
#endif  //  CONFIG_PM
#endif  // else CONFIG_HAS_EARLYSUSPEND

static struct platform_driver s3cfb_driver = {
	.probe = s3cfb_probe,
	.remove = s3cfb_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND 
	.suspend		= s3cfb_suspend,
	.resume		= s3cfb_resume,
#endif

	.driver = {
		   .name = S3CFB_NAME,
		   .owner = THIS_MODULE,
		   },
};

static int s3cfb_register(void)
{
	platform_driver_register(&s3cfb_driver);

	return 0;
}

static void s3cfb_unregister(void)
{
	platform_driver_unregister(&s3cfb_driver);
}

module_init(s3cfb_register);
module_exit(s3cfb_unregister);

MODULE_AUTHOR("Jonghun, Han <jonghun.han@samsung.com>");
MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("Samsung Display Controller (FIMD) driver");
MODULE_LICENSE("GPL");
