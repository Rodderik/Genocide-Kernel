#
# Makefile for kernel Onedram drivers.
#

ifeq ($(CONFIG_SAMSUNG_PHONE_TTY_VICTORY),m)
obj-$(CONFIG_SAMSUNG_PHONE_TTY) +=victory/dpram.o
#obj-$(CONFIG_SAMSUNG_PHONE_TTY) +=victory/multipdp.o
endif
ifeq ($(CONFIG_SAMSUNG_PHONE_TTY_ATLAS),m)
obj-$(CONFIG_SAMSUNG_PHONE_TTY) +=atlas/dpram.o
#obj-$(CONFIG_SAMSUNG_PHONE_TTY) +=atlas/multipdp.o
endif

obj-m	+= dpram_recovery/
