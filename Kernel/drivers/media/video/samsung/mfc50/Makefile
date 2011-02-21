
ifeq ($(CONFIG_VIDEO_MFC50_VICTORY),y)
obj-$(CONFIG_VIDEO_MFC50) += victory/mfc_fw.o victory/mfc.o  victory/mfc_intr.o victory/mfc_memory.o victory/mfc_opr.o 
endif
ifeq ($(CONFIG_VIDEO_MFC50_ATLAS),y)
obj-$(CONFIG_VIDEO_MFC50) += atlas/mfc_fw.o atlas/mfc.o  atlas/mfc_intr.o atlas/mfc_memory.o atlas/mfc_opr.o 
endif

obj-$(CONFIG_VIDEO_MFC50) += mfc_buffer_manager.o  mfc_shared_mem.o

ifeq ($(CONFIG_VIDEO_MFC50_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

#EXTRA_CFLAGS += -DMFC_REQUEST_TIME
