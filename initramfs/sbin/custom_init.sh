#!/system/bin/sh
# Originally written by tanimn for Twilight Zone Kernel 1.0.3
# Modified by Rodderik for Genocide Kernel
# CWM compatibility and keytimer support by DRockstar

# Remount filesystems RW

/sbin/busybox mount -o remount,rw /dev/block/stl9 /system
/sbin/busybox mount -o remount,rw / /

# Install busybox
/sbin/busybox mkdir /bin
/sbin/busybox --install -s /bin
rm -rf /system/xbin/busybox
ln -s /sbin/busybox /system/xbin/busybox
rm -rf /res
sync

# Fix permissions in /sbin, just in case
chmod 755 /sbin/*

# Lock the CPU down til SetCPU or such can set it.
echo 1000000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
echo 1000000 > /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq

# Switch to Conservative CPU governor after bootup
echo conservative > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

# Fix screwy ownerships
for blip in conf default.prop fota.rc init init.goldfish.rc init.rc init.smdkc110.rc lib lpm.rc modules recovery.rc res sbin
do
	chown root.system /$blip
	chown root.system /$blip/*
done

chown root.system /lib/modules/*
chown root.system /res/images/*

# Enable init.d support
if [ -d /system/etc/init.d ]
then
	logwrapper busybox run-parts /system/etc/init.d
fi
sync

# keyboard patch sysfs call 5 for snappy keyboard performance (range: 1-16 default: 7)
if [ ! -f "/data/local/timer_delay" ]; then
  echo 5 > /data/local/timer_delay
fi
cat /data/local/timer_delay > /sys/devices/platform/s3c-keypad/timer_delay

# setup tmpfs for su
mkdir /sdx
mount -o size=30k -t tmpfs tmpfs /sdx
cat /sbin/su > /sdx/su
chmod 6755 /sdx/su
# establish root in common system directories for 3rd party applications
rm /system/bin/su
rm /system/xbin/su
rm /system/bin/jk-su
ln -s /sdx/su /system/bin/su
ln -s /sdx/su /system/xbin/su
# remove su in problematic locations
rm -rf /bin/su
rm -rf /sbin/su

#setup proper passwd and group files for 3rd party root access
# Thanks DevinXtreme
if [ ! -f "/system/etc/passwd" ]; then
	echo "root::0:0:root:/data/local:/system/bin/sh" > /system/etc/passwd
	chmod 0666 /system/etc/passwd
fi
if [ ! -f "/system/etc/group" ]; then
	echo "root::0:" > /system/etc/group
	chmod 0666 /system/etc/group
fi

#Check and setup Superuser if missing
 if [ ! -f "/system/app/Superuser.apk" ] && [ ! -f "/data/app/Superuser.apk" ] && [[ ! -f "/data/app/com.noshufou.android.su"* ]]; then
	if [ -f "/system/app/Asphalt5_DEMO_ANMP_Samsung_D700_Sprint_ML.apk" ]; then
		rm /system/app/Asphalt5_DEMO_ANMP_Samsung_D700_Sprint_ML.apk
	fi
	if [ -f "/system/app/FreeHDGameDemos.apk" ]; then
		rm /system/app/FreeHDGameDemos.apk
	fi
 	busybox cp /sbin/Superuser.apk /system/app/Superuser.apk
 fi

# fix busybox DNS while system is read-write
if [ ! -f "/system/etc/resolv.conf" ]; then
	echo "nameserver 8.8.8.8" > /system/etc/resolv.conf
	echo "nameserver 8.8.4.4" >> /system/etc/resolv.conf
fi 

# Patch to attempt removal and prevention of DroidDream malware
if [ -f "/system/bin/profile" ]; then
	rm /system/bin/profile
fi
touch /system/bin/profile
chmod 644 /system/bin/profile
sync

# remount read only and continue
mount -o remount,ro /dev/block/stl9 /system
mount -o remount,ro / /

