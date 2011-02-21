/* linux/arch/arm/plat-s3c/include/plat/fb.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C - FB platform data definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __PLAT_S3C_FB_H
#define __PLAT_S3C_FB_H __FILE__

#ifdef CONFIG_FB_S3C

#define FB_SWAP_WORD	(1 << 24)
#define FB_SWAP_HWORD	(1 << 16)
#define FB_SWAP_BYTE	(1 << 8)
#define FB_SWAP_BIT	(1 << 0)

struct platform_device;
struct clk;
#if defined CONFIG_S5PV210_GARNETT_DELTA
#if defined(CONFIG_FB_S3C_MIPI_LCD)
/* enumerates display mode. */
enum {
	SINGLE_LCD_MODE = 1,
	DUAL_LCD_MODE = 2,
};

/* enumerates interface mode. */
enum {
	FIMD_RGB_INTERFACE = 1,
	FIMD_CPU_INTERFACE = 2,
};
#endif
#endif
struct s3c_platform_fb {
	int		hw_ver;
	char		clk_name[16];
	int		nr_wins;
	int		nr_buffers[5];
	int		default_win;
	int		swap;
#if defined(CONFIG_FB_S3C_MIPI_LCD)
	void		*lcd_data;
	unsigned int	sub_lcd_enabled;
	unsigned int	machine_is_cypress;
	unsigned int	machine_is_p1p2;
	unsigned int	mdnie_is_enabled;
	unsigned int	mipi_is_enabled;
	unsigned int	interface_mode;

	void		*single_lcd;
	void		*dual_lcd;

	void		(*set_display_path)(unsigned int mode);
	void		(*cfg_gpio)(void);
	int		(*backlight_on)(int onoff);
	int		(*reset_lcd)(void);

	/* variables and interface for mDNIe */
	char		mdnie_clk_name[20];
	void		*mdnie_clk;
	unsigned int	mdnie_phy_base;
	unsigned int	ielcd_phy_base;
	void __iomem	*mdnie_mmio_base;
	void __iomem	*ielcd_mmio_base;
	unsigned char	mdnie_mode;

	void		(*set_mdnie_clock)(void *mdnie_clk, unsigned char enable);
	void		(*init_mdnie)(unsigned int mdnie_base,
				unsigned int hsize, unsigned int vsize);
	void		(*mdnie_set_mode)(unsigned int mdnie_base, unsigned char mode);

	void		(*start_ielcd_logic)(unsigned int ielcd_base);
	void		(*init_ielcd)(unsigned int ielcd_base, void *l, void *c);
#else	
	void		(*cfg_gpio)(struct platform_device *dev);
	int		(*backlight_on)(struct platform_device *dev);
	int		(*reset_lcd)(struct platform_device *dev);
#endif
	void            *lcd;
	int		(*backlight_onoff)(struct platform_device *dev, int onoff);
	int		(*clk_on)(struct platform_device *pdev, struct clk **s3cfb_clk);
	int		(*clk_off)(struct platform_device *pdev, struct clk **clk);
};
#if defined (CONFIG_S5PV210_GARNETT_DELTA)
#if defined(CONFIG_FB_S3C_MIPI_LCD)
/*
 * struct s3cfb_lcd_polarity
 * @rise_vclk:	if 1, video data is fetched at rising edge
 * @inv_hsync:	if HSYNC polarity is inversed
 * @inv_vsync:	if VSYNC polarity is inversed
 * @inv_vden:	if VDEN polarity is inversed
*/
struct s3cfb_lcd_polarity {
	int	rise_vclk;
	int	inv_hsync;
	int	inv_vsync;
	int	inv_vden;
};

/*
 * struct s3cfb_lcd_timing
 * @h_fp:	horizontal front porch
 * @h_bp:	horizontal back porch
 * @h_sw:	horizontal sync width
 * @v_fp:	vertical front porch
 * @v_fpe:	vertical front porch for even field
 * @v_bp:	vertical back porch
 * @v_bpe:	vertical back porch for even field
*/
struct s3cfb_lcd_timing {
	int	h_fp;
	int	h_bp;
	int	h_sw;
	int	v_fp;
	int	v_fpe;
	int	v_bp;
	int	v_bpe;
	int	v_sw;
	int	cmd_allow_len;
	void		(*cfg_gpio)(struct platform_device *dev);
	int		(*backlight_on)(struct platform_device *dev);
	int		(*reset_lcd)(struct platform_device *dev);
};

/* for CPU Interface */
struct s3cfb_cpu_timing {
	unsigned int	cs_setup;
	unsigned int	wr_setup;
	unsigned int	wr_act;
	unsigned int	wr_hold;
};

/*
 * struct s3cfb_lcd
 * @name:		lcd panel name
 * @width:		horizontal resolution
 * @height:		vertical resolution
 * @width_mm:		width of picture in mm
 * @height_mm:		height of picture in mm
 * @bpp:		bits per pixel
 * @freq:		vframe frequency
 * @timing:		rgb timing values
 * @cpu_timing:		cpu timing values
 * @polarity:		polarity settings
 * @init_ldi:		pointer to LDI init function
 *
*/
struct s3cfb_lcd {
	char	*name;
	int	width;
	int	height;
	int	width_mm;
	int	height_mm;
	int	bpp;
	int	freq;
	struct	s3cfb_lcd_timing timing;
	struct	s3cfb_lcd_polarity polarity;
	struct	s3cfb_cpu_timing cpu_timing;

	void	(*init_ldi)(void);
	void	(*deinit_ldi)(void);
};
#endif
#endif
extern void s3cfb_set_platdata(struct s3c_platform_fb *fimd);

/* defined by architecture to configure gpio */
extern void s3cfb_cfg_gpio(struct platform_device *pdev);
extern int s3cfb_backlight_on(struct platform_device *pdev);
extern int s3cfb_backlight_onoff(struct platform_device *pdev, int onoff);
extern int s3cfb_reset_lcd(struct platform_device *pdev);
extern int s3cfb_clk_on(struct platform_device *pdev, struct clk **s3cfb_clk);
extern int s3cfb_clk_off(struct platform_device *pdev, struct clk **clk);
extern void s3cfb_get_clk_name(char *clk_name);

#else

#ifdef CONFIG_S3C_FB

/**
 * struct s3c_fb_pd_win - per window setup data
 * @win_mode: The display parameters to initialise (not for window 0)
 * @virtual_x: The virtual X size.
 * @virtual_y: The virtual Y size.
 */
struct s3c_fb_pd_win {
	struct fb_videomode	win_mode;

	unsigned short		default_bpp;
	unsigned short		max_bpp;
	unsigned short		virtual_x;
	unsigned short		virtual_y;
};

/**
 * struct s3c_fb_platdata -  S3C driver platform specific information
 * @setup_gpio: Setup the external GPIO pins to the right state to transfer
 *		the data from the display system to the connected display
 *		device.
 * @vidcon0: The base vidcon0 values to control the panel data format.
 * @vidcon1: The base vidcon1 values to control the panel data output.
 * @win: The setup data for each hardware window, or NULL for unused.
 * @display_mode: The LCD output display mode.
 *
 * The platform data supplies the video driver with all the information
 * it requires to work with the display(s) attached to the machine. It
 * controls the initial mode, the number of display windows (0 is always
 * the base framebuffer) that are initialised etc.
 *
 */
struct s3c_fb_platdata {
	void	(*setup_gpio)(void);

	struct s3c_fb_pd_win	*win[S3C_FB_MAX_WIN];

	u32			 vidcon0;
	u32			 vidcon1;
};

/**
 * s3c_fb_set_platdata() - Setup the FB device with platform data.
 * @pd: The platform data to set. The data is copied from the passed structure
 *      so the machine data can mark the data __initdata so that any unused
 *      machines will end up dumping their data at runtime.
 */
extern void s3c_fb_set_platdata(struct s3c_fb_platdata *pd);

/**
 * s3c64xx_fb_gpio_setup_24bpp() - S3C64XX setup function for 24bpp LCD
 *
 * Initialise the GPIO for an 24bpp LCD display on the RGB interface.
 */
extern void s3c64xx_fb_gpio_setup_24bpp(void);

/**
 * s5pc100_fb_gpio_setup_24bpp() - S5PC100 setup function for 24bpp LCD
 *
 * Initialise the GPIO for an 24bpp LCD display on the RGB interface.
 */
extern void s5pc100_fb_gpio_setup_24bpp(void);

#endif	/* CONFIG_FB */

#endif

#endif /* __PLAT_S3C_FB_H */
