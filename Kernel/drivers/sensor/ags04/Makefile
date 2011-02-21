#
# Makefile for the grip sensor ICs with grip sensor driver
#

ifneq ($(KERNELRELEASE), )

MOD_NAME = ags04_driver
obj-$(CONFIG_GRIP_SENSOR_AGS04) := $(MOD_NAME).o

EXTRA_CFLAGS := -I$(PRJROOT)/modules/include

$(MOD_NAME)-y := ags04.o \
				ags04_dev.o \
				ags04_i2c.o 

else

ifndef KDIR
	KDIR :=$(PRJROOT)/linux-2.6.29
endif 

all:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules

clean:
	rm -f *.o *.ko *.mod.c *~ .*.cmd

install:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules_install

endif
