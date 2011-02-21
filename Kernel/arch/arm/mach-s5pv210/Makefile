# arch/arm/mach-s5pv210/Makefile
#
# Copyright (c) 2010 Samsung Electronics Co., Ltd.
# 		http://www.samsung.com/
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:=
obj-				:=

# Core support for S5PV210 system

obj-$(CONFIG_CPU_S5PV210)	+= cpu.o init.o clock.o gpio.o dma.o pd.o irq-eint-group.o irq-eint.o

# machine support

obj-$(CONFIG_MACH_SMDKV210)	+= mach-smdkv210.o

ifeq ($(CONFIG_S5PV210_VICTORY),y)
obj-$(CONFIG_MACH_SMDKC110)	+= victory/mach-smdkc110.o
endif
ifeq ($(CONFIG_S5PV210_ATLAS),y)
obj-$(CONFIG_MACH_SMDKC110)	+= atlas/mach-smdkc110.o
endif

obj-$(CONFIG_MACH_S5PC110_P1)	+= mach-p1.o p1-rfkill.o

ifeq ($(CONFIG_S5PV210_VICTORY),y)
obj-$(CONFIG_MACH_S5PC110_ARIES)	+= victory/mach-aries.o victory/aries-rfkill.o
endif
ifeq ($(CONFIG_S5PV210_ATLAS),y)
obj-$(CONFIG_MACH_S5PC110_ARIES)	+= atlas/mach-aries.o atlas/aries-rfkill.o
endif

obj-$(CONFIG_MACH_S5PC110_ARIES)	+= mach-aries-gps.o

obj-$(CONFIG_S5PV210_PM)	+= pm.o
obj-$(CONFIG_PM_PWR_GATING)	+= powergating.o

ifeq ($(CONFIG_S5PV210_VICTORY),y)
obj-$(CONFIG_S5PV210_SETUP_SDHCI)       += victory/setup-sdhci.o
endif
ifeq ($(CONFIG_S5PV210_ATLAS),y)
obj-$(CONFIG_S5PV210_SETUP_SDHCI)       += atlas/setup-sdhci.o
endif

ifeq ($(CONFIG_S5PV210_VICTORY),y)
obj-$(CONFIG_CPU_FREQ)	+= victory/cpu-freq.o
endif
ifeq ($(CONFIG_S5PV210_ATLAS),y)
obj-$(CONFIG_CPU_FREQ)	+= atlas/cpu-freq.o
endif

ifeq ($(CONFIG_S5PV210_VICTORY),y)
obj-$(CONFIG_REGULATOR_MAX8998) += victory/max8998_consumer.o
obj-$(CONFIG_REGULATOR_MAX8893) += victory/max8893_consumer.o
endif
ifeq ($(CONFIG_S5PV210_ATLAS),y)
obj-$(CONFIG_REGULATOR_MAX8998) += atlas/max8998_consumer.o
endif

# Device setup - ADCTS
obj-$(CONFIG_S5PV210_ADCTS)		+= s5pc11x-adcts.o
obj-$(CONFIG_S5P_ADC)			+= adc.o

obj-$(CONFIG_S5PV210_SETUP_FB)		+= setup-fb.o
obj-$(CONFIG_S5PV210_SETUP_FIMC0)	+= setup-fimc0.o
obj-$(CONFIG_S5PV210_SETUP_FIMC1)	+= setup-fimc1.o
obj-$(CONFIG_S5PV210_SETUP_FIMC2)	+= setup-fimc2.o
obj-$(CONFIG_S5PV210_SETUP_CSIS)	+= setup-csis.o

ifeq ($(CONFIG_S5PV210_VICTORY),y)
obj-y	+= victory/sec_jack.o
endif
ifeq ($(CONFIG_S5PV210_ATLAS),y)
obj-y	+= atlas/sec_jack.o
endif

obj-$(CONFIG_CPU_IDLE)		+= cpuidle.o idle2.o
