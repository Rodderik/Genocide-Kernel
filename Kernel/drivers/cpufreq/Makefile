# CPUfreq core
obj-$(CONFIG_CPU_FREQ)			+= cpufreq.o
# CPUfreq stats
obj-$(CONFIG_CPU_FREQ_STAT)             += cpufreq_stats.o

# CPUfreq governors 
obj-$(CONFIG_CPU_FREQ_GOV_PERFORMANCE)	+= cpufreq_performance.o
obj-$(CONFIG_CPU_FREQ_GOV_POWERSAVE)	+= cpufreq_powersave.o
obj-$(CONFIG_CPU_FREQ_GOV_USERSPACE)	+= cpufreq_userspace.o
obj-$(CONFIG_CPU_FREQ_GOV_ONDEMAND)	+= cpufreq_ondemand.o

ifeq ($(CONFIG_CPU_FREQ_GOV_CONSERVATIVE_VICTORY),y)
obj-$(CONFIG_CPU_FREQ_GOV_CONSERVATIVE)	+= victory/cpufreq_conservative.o
endif 
ifeq ($(CONFIG_CPU_FREQ_GOV_CONSERVATIVE_ATLAS),y)
obj-$(CONFIG_CPU_FREQ_GOV_CONSERVATIVE)	+= atlas/cpufreq_conservative.o
endif 

# CPUfreq cross-arch helpers
obj-$(CONFIG_CPU_FREQ_TABLE)		+= freq_table.o

