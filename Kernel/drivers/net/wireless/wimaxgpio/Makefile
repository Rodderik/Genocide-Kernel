#
#   Makefile S3C WiMAX GPIO driver
#
#   Copyright(c) 2010, Samsung Electronics, Co., Ltd.
#


obj-m			:= wimax_gpio.o

wimax_gpio-y	:= wibrogpio.o wimax_download.o wimax_i2c.o wimaxproc.o 

all:
	@echo "$(MAKE) --no-print-directory -C $(KDIR) SUBDIRS=$(CURDIR) modules"
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules

clean:
	rm -f *.o *.ko *.mod.* *~ .*.cmd Module.symvers modules.order

install:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules_install
