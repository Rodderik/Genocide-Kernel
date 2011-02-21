/* linux/drivers/video/samsung/s3cfb2.h
 *
 * Header file for Samsung Display Driver (FIMD) driver
 *
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S3CFB_H
#define _S3CFB_H

#ifdef __KERNEL__
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/fb.h>
#include <plat/fb.h>
#endif
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#endif

/*
 * C O M M O N   D E F I N I T I O N S
 *
*/
#define S3CFB_NAME		"s3cfb"

#define S3CFB_AVALUE(r, g, b)	(((r & 0xf) << 8) | ((g & 0xf) << 4) | ((b & 0xf) << 0))
#define S3CFB_CHROMA(r, g, b)	(((r & 0xff) << 16) | ((g & 0xff) << 8) | ((b & 0xff) << 0))

/* Mask Color Keys for win1-4 for 16bpp*/
#define S3C_WIN1_COLOR_MASK_KEY                 0x070337
#define S3C_WIN2_COLOR_MASK_KEY                 0x071b07
#define S3C_WIN3_COLOR_MASK_KEY                 0xc70307
#define S3C_WIN4_COLOR_MASK_KEY                 0x070337

/*
 * E N U M E R A T I O N S
 *
*/
enum s3cfb_data_path_t {
	DATA_PATH_FIFO = 0,
	DATA_PATH_DMA = 1,
	DATA_PATH_IPC = 2,
};

enum s3cfb_alpha_t {
	PLANE_BLENDING,
	PIXEL_BLENDING,
};

enum s3cfb_chroma_dir_t {
	CHROMA_FG,
	CHROMA_BG,
};

enum s3cfb_output_t {
	OUTPUT_RGB,
	OUTPUT_ITU,
	OUTPUT_I80LDI0,
	OUTPUT_I80LDI1,
	OUTPUT_WB_RGB,
	OUTPUT_WB_I80LDI0,
	OUTPUT_WB_I80LDI1,
};

enum s3cfb_rgb_mode_t {
	MODE_RGB_P = 0,
	MODE_BGR_P = 1,
	MODE_RGB_S = 2,
	MODE_BGR_S = 3,
};

enum s3cfb_mem_owner_t {
	DMA_MEM_NONE	= 0,
	DMA_MEM_FIMD	= 1,
	DMA_MEM_OTHER	= 2,
};

enum s3cfb_dualrgb_mode {
	MODE_BYPASS_SINGLE = 0,
	MODE_BYPASS_DUAL = 1,
	MODE_MIE_DUAL = 2,
	MODE_MIE_MDNIE = 3,
};

enum s3cfb_vidout_mode {
	VIDOUT_RGB = 0,
	VIDOUT_ITU601 = 1,
	VIDOUT_I80_LDI0 = 2,
	VIDOUT_I80_LDI1 = 3,
	VIDOUT_WB_RGB = 4,
	VIDOUT_WB_I80_LDI0 = 6,
	VIDOUT_WB_I80_LDI1 = 7,
};


enum s3cfb_cpu_auto_cmd_rate {
	DISABLE_AUTO_FRM,
	PER_TWO_FRM,
	PER_FOUR_FRM,
	PER_SIX_FRM,
	PER_EIGHT_FRM,
	PER_TEN_FRM,
	PER_TWELVE_FRM,
	PER_FOURTEEN_FRM,
	PER_SIXTEEN_FRM,
	PER_EIGHTEEN_FRM,
	PER_TWENTY_FRM,
	PER_TWENTY_TWO_FRM,
	PER_TWENTY_FOUR_FRM,
	PER_TWENTY_SIX_FRM,
	PER_TWENTY_EIGHT_FRM,
	PER_THIRTY_FRM,
};
/*
 * F I M D   S T R U C T U R E S
 *
*/

/*
 * struct s3cfb_alpha
 * @mode:		blending method (plane/pixel)
 * @channel:		alpha channel (0/1)
 * @value:		alpha value (for plane blending)
*/
struct s3cfb_alpha {
	enum 		s3cfb_alpha_t mode;
	int		channel;
	unsigned int	value;
};

/*
 * struct s3cfb_chroma
 * @enabled:		if chroma key function enabled
 * @blended:		if chroma key alpha blending enabled (unused)
 * @key:		chroma value to be applied
 * @comp_key:		compare key (unused)
 * @alpha:		alpha value for chroma (unused)
 * @dir:		chroma key direction (fg/bg, fixed to fg)
 *
*/
struct s3cfb_chroma {
	int 		enabled;
	int 		blended;
	unsigned int	key;
	unsigned int	comp_key;
	unsigned int	alpha;
	enum 		s3cfb_chroma_dir_t dir;
};

/*
 * struct s3cfb_window
 * @id:			window id
 * @enabled:		if enabled
 * @x:			left x of start offset
 * @y:			top y of start offset
 * @path:		data path (dma/fifo)
 * @local_channel:	local channel for fifo path (0/1)
 * @dma_burst:		dma burst length (4/8/16)
 * @unpacked:		if unpacked format is
 * @pseudo_pal:		pseudo palette for fb layer
 * @alpha:		alpha blending structure
 * @chroma:		chroma key structure
*/
struct s3cfb_window {
	int			id;
	int			enabled;
	atomic_t		in_use;
	int			x;
	int			y;
	enum 			s3cfb_data_path_t path;
	enum 			s3cfb_mem_owner_t owner;
	int			local_channel;
	int			dma_burst;
	unsigned int		pseudo_pal[16];
	struct			s3cfb_alpha alpha;
	struct			s3cfb_chroma chroma;
};

/*
 * struct s3cfb_global
 *
 * @fb:			pointer to fb_info
 * @enabled:		if signal output enabled
 * @dsi:		if mipi-dsim enabled
 * @interlace:		if interlace format is used
 * @output:		output path (RGB/I80/Etc)
 * @rgb_mode:		RGB mode
 * @lcd:		pointer to lcd structure
*/
struct s3cfb_global {
	/* general */
	void __iomem		*regs;
	struct mutex		lock;
	struct device		*dev;
	struct clk		*clock;
	int			irq;
	wait_queue_head_t	wq;
	unsigned int		wq_count;
	unsigned int		vsync_ref_count;
	struct fb_info		**fb;

	/* fimd */
	int			enabled;
	int			dsi;
	int			interlace;
	enum s3cfb_output_t	output;
	enum s3cfb_rgb_mode_t	rgb_mode;
	struct s3cfb_lcd	*lcd;

#ifdef CONFIG_CPU_FREQ
	struct notifier_block	freq_transition;
	struct notifier_block	freq_policy;
#endif
	struct s3c_platform_fb *platform_data;
#ifdef CONFIG_HAS_WAKELOCK
        struct early_suspend    early_suspend;
        struct wake_lock        idle_lock;
#endif

};

typedef struct {
	unsigned int phy_start_addr;
	unsigned int xres;		/* visible resolution*/
	unsigned int yres;
	unsigned int xres_virtual;	/* virtual resolution*/
	unsigned int yres_virtual;
	unsigned int xoffset;		/* offset from virtual to visible */
	unsigned int yoffset;		/* resolution */
	unsigned int lcd_offset_x;
	unsigned int lcd_offset_y;
} s3cfb_next_info_t;

/*
 * S T R U C T U R E S  F O R  C U S T O M  I O C T L S
 *
*/
struct s3cfb_user_window {
	int x;
	int y;
};

struct s3cfb_user_plane_alpha {
	int 		channel;
	unsigned char	red;
	unsigned char	green;
	unsigned char	blue;
};

struct s3cfb_user_chroma {
	int 		enabled;
	unsigned char	red;
	unsigned char	green;
	unsigned char	blue;
};

struct s3cfb_vsync {
	struct	list_head list;
	char	name[64];
	int	(*vsync_handler) (void *pdata);
	void	*pdata;
};

/*
 * C U S T O M  I O C T L S
 *
*/
#define FBIO_WAITFORVSYNC		_IO  ('F', 32)
#define S3CFB_WIN_POSITION		_IOW ('F', 203, struct s3cfb_user_window)
#define S3CFB_WIN_SET_PLANE_ALPHA	_IOW ('F', 204, struct s3cfb_user_plane_alpha)
#define S3CFB_WIN_SET_CHROMA		_IOW ('F', 205, struct s3cfb_user_chroma)
#define S3CFB_SET_VSYNC_INT		_IOW ('F', 206, u32)
#define S3CFB_CHANGE_MODE		_IOW ('F', 207, u32)
#define S3CFB_GET_LCD_WIDTH		_IOR ('F', 302, int)
#define S3CFB_GET_LCD_HEIGHT		_IOR ('F', 303, int)
#define S3CFB_SET_WRITEBACK		_IOW ('F', 304, u32)
#define S3CFB_SET_WIN_ON		_IOW ('F', 305, u32)
#define S3CFB_SET_WIN_OFF		_IOW ('F', 306, u32)
#define S3CFB_SET_WIN_PATH		_IOW ('F', 307, enum s3cfb_data_path_t) 
#define S3CFB_SET_WIN_ADDR		_IOW ('F', 308, unsigned long)
#define S3CFB_SET_WIN_MEM		_IOW ('F', 309, enum s3cfb_mem_owner_t) 
#define S3CFB_GET_CURR_FB_INFO		_IOR ('F', 305, s3cfb_next_info_t)

/*
 * E X T E R N S
 *
*/
extern int soft_cursor(struct fb_info *info, struct fb_cursor *cursor);
extern void s3cfb_set_lcd_info(struct s3cfb_global *ctrl);
extern struct s3c_platform_fb *to_fb_plat(struct device *dev);
extern void s3cfb_check_line_count(struct s3cfb_global *ctrl);
extern int s3cfb_set_output(struct s3cfb_global *ctrl);
extern int s3cfb_set_free_run(struct s3cfb_global *ctrl, int onoff);
extern int s3cfb_set_vidout(struct s3cfb_global *ctrl,
	enum s3cfb_vidout_mode mode);
extern int s3cfb_enable_mipi_dsi_mode(struct s3cfb_global *ctrl,
	unsigned int enable);
extern int s3cfb_set_cpu_interface_timing(struct s3cfb_global *ctrl,
	unsigned char ldi);
extern void s3cfb_set_trigger(struct s3cfb_global *ctrl);
extern u8 s3cfb_is_frame_done(struct s3cfb_global *ctrl);
extern int s3cfb_set_auto_cmd_rate(struct s3cfb_global *ctrl,
	unsigned char cmd_rate, unsigned char ldi);
extern int s3cfb_set_display_mode(struct s3cfb_global *ctrl);
extern int s3cfb_display_on(struct s3cfb_global *ctrl);
extern int s3cfb_display_off(struct s3cfb_global *ctrl);
extern int s3cfb_frame_off(struct s3cfb_global *ctrl);
extern int s3cfb_set_clock(struct s3cfb_global *ctrl, unsigned long rate);
extern unsigned int s3cfb_get_vidcon0_cfg(struct s3cfb_global *ctrl,
	unsigned int rate, unsigned int *vidcon0_reg);
extern int s3cfb_set_polarity(struct s3cfb_global *ctrl);
extern int s3cfb_set_timing(struct s3cfb_global *ctrl);
extern int s3cfb_set_lcd_size(struct s3cfb_global *ctrl);
extern int s3cfb_set_global_interrupt(struct s3cfb_global *ctrl, int enable);
extern int s3cfb_set_vsync_interrupt(struct s3cfb_global *ctrl, int enable);
extern int s3cfb_set_fifo_interrupt(struct s3cfb_global *ctrl, int enable);
extern int s3cfb_clear_interrupt(struct s3cfb_global *ctrl);
extern int s3cfb_channel_localpath_on(struct s3cfb_global *ctrl, int id);
extern int s3cfb_channel_localpath_off(struct s3cfb_global *ctrl, int id);
extern int s3cfb_window_on(struct s3cfb_global *ctrl, int id);
extern int s3cfb_window_off(struct s3cfb_global *ctrl, int id);
extern int s3cfb_win_map_on(struct s3cfb_global *ctrl, int id, int color);
extern int s3cfb_win_map_off(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_window_control(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_alpha_blending(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_window_position(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_window_size(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_buffer_address(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_buffer_size(struct s3cfb_global *ctrl, int id);
extern int s3cfb_set_chroma_key(struct s3cfb_global *ctrl, int id);

extern int s3cfb_set_dualrgb(struct s3cfb_global *ctrl, unsigned int enabled);
extern int s3cfb_set_dualrgb_mode(struct s3cfb_global *ctrl,
	enum s3cfb_dualrgb_mode mode);

#ifdef CONFIG_HAS_WAKELOCK
#ifdef CONFIG_HAS_EARLYSUSPEND
extern int s3cfb_early_re_suspend(void);
extern int s3cfb_early_suspend(struct early_suspend *h);
extern int s3cfb_late_resume(struct early_suspend *h);
extern int s3cfb_late_re_resume(void);
#endif
#endif

#define dev_dbg(x...)

#endif /* _S3CFB_H */
