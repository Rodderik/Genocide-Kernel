#
# Makefile for the s3c framebuffer driver
#

ifeq ($(CONFIG_FB_S3C),y)

ifeq ($(CONFIG_FB_S3C_VICTORY),y)
obj-y				+= victory/s3cfb.o
endif
ifeq ($(CONFIG_FB_S3C_ATLAS),y)
obj-y				+= atlas/s3cfb.o
endif

obj-$(CONFIG_ARCH_S5PV210)	+= s3cfb_fimd6x.o

obj-$(CONFIG_FB_S3C_LTE480WV)	+= s3cfb_lte480wv.o
obj-$(CONFIG_FB_S3C_LVDS)       += s3cfb_lvds.o
obj-$(CONFIG_FB_S3C_HT101HD1)   += s3cfb_ht101hd1.o

ifeq ($(CONFIG_FB_S3C_TL2796_VICTORY),y)
obj-$(CONFIG_FB_S3C_TL2796)     += victory/s3cfb_tl2796.o victory/s6e63m0.o
endif
ifeq ($(CONFIG_FB_S3C_TL2796_ATLAS),y)
obj-$(CONFIG_FB_S3C_TL2796)     += atlas/s3cfb_tl2796.o atlas/s6e63m0.o
endif

ifeq ($(CONFIG_S5PV210_CRESPO_DELTA),y)
ifdef CONFIG_FB_S3C_NT35580
obj-$(CONFIG_FB_S3C_NT35580)	+= victory/s3cfb_nt35580.o
else
obj-$(CONFIG_FB_S3C_TL2796)     += victory/s3cfb_tl2796.o victory/s6e63m0.o
endif
endif

obj-$(CONFIG_FB_S3C_AMS701KA)	+= s3cfb_ams701ka.o
obj-$(CONFIG_FB_S3C_MDNIE)      += s3cfb_mdnie.o s3cfb_ielcd.o
obj-$(CONFIG_FB_S3C_CMC623)     += tune_cmc623.o
endif

ifeq ($(CONFIG_FB_S3C_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

