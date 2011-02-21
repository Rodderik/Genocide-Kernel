
#obj-$(CONFIG_VIDEO_FIMC)	+= fimc_dev.o fimc_v4l2.o fimc_overlay.o

ifeq ($(CONFIG_VIDEO_FIMC_VICTORY),y)
obj-$(CONFIG_VIDEO_FIMC)	+= victory/fimc_capture.o victory/fimc_output.o victory/fimc_regs.o victory/fimc_dev.o victory/fimc_v4l2.o victory/fimc_overlay.o
endif
ifeq ($(CONFIG_VIDEO_FIMC_ATLAS),y)
obj-$(CONFIG_VIDEO_FIMC)	+= atlas/fimc_capture.o atlas/fimc_output.o atlas/fimc_regs.o atlas/fimc_dev.o atlas/fimc_v4l2.o atlas/fimc_overlay.o
endif

obj-$(CONFIG_VIDEO_FIMC_MIPI)	+= csis.o
obj-$(CONFIG_CPU_S5PV210)	+= ipc.o

ifeq ($(CONFIG_CPU_S5PV210),y)
EXTRA_CFLAGS += -DCONFIG_MIPI_CSI_ADV_FEATURE
endif

EXTRA_CFLAGS += -Idrivers/media/video

ifeq ($(CONFIG_VIDEO_FIMC_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif
