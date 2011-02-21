#
# Makefile for the s3c framebuffer driver
#

#mipi_temp
obj-y				+= s3cfb.o
obj-y				+= s3cfb_ams397g201.o
obj-y				+= s5p_dsim_lowlevel.o
obj-y				+= s5p-dsim.o
obj-y	                      += s3cfb_fimd6x.o

ifeq ($(CONFIG_FB_S3C_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

