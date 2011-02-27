#!/sbin/busybox sh
# filesystem patcher
# Copyright SztupY, Licence: GPLv3
# Modified by HardCORE

export PATH=/sbin:/system/bin:/system/xbin
mv /data/user.log /data/user.log.old
exec >>/data/user.log
exec 2>&1

# init read-only'd the rootfs
/sbin/busybox mount -o remount,rw /

# Put the rfs creation tools back, they are needed if you want to install apps to sdcard
/sbin/busybox cp /sbin/fat.format /system/bin/fat.format
# /sbin/busybox mv /system/bin/newfs_msdos.bak /system/bin/newfs_msdos

# Android Logger enable tweak
if /sbin/busybox [ "`grep ANDROIDLOGGER /system/etc/tweaks.conf`" ]; then
  insmod /lib/modules/logger.ko
else
  rm /lib/modules/logger.ko
fi

# IPv6 privacy tweak
if /sbin/busybox [ "`grep IPV6PRIVACY /system/etc/tweaks.conf`" ]; then
  echo "2" > /proc/sys/net/ipv6/conf/all/use_tempaddr
fi

# run fs tweaks
if /sbin/busybox [ "`grep IOSCHED /system/etc/tweaks.conf`" ]; then
  # Tweak cfq io scheduler
  for i in $(ls -1 /sys/block/stl*) $(ls -1 /sys/block/mmc*) $(ls -1 /sys/block/bml*) $(ls -1 /sys/block/tfsr*)
  do echo "0" > $i/queue/rotational
    echo "1" > $i/queue/iosched/low_latency
	echo "2" > $i/queue/iosched/slice_idle
	#echo "8" > $i/queue/iosched/quantum
    echo "1" > $i/queue/iosched/back_seek_penalty
    echo "1000000000" > $i/queue/iosched/back_seek_max
  done
  # Remount all partitions with noatime
  for k in $(/sbin/busybox mount | grep relatime | cut -d " " -f3)
  do
        sync
        /sbin/busybox mount -o remount,noatime $k
  done
fi

# Tweak kernel VM management
if /sbin/busybox [ "`grep KERNELVM /system/etc/tweaks.conf`" ]; then
  echo "60" > /proc/sys/vm/swappiness
  #echo "10" > /proc/sys/vm/dirty_ratio
  #echo "1000" > /proc/sys/vm/vfs_cache_pressure
  #echo "4096" > /proc/sys/vm/min_free_kbytes
fi

# Tweak kernel scheduler
#if /sbin/busybox [ "`grep KERNELSCHED /system/etc/tweaks.conf`" ]; then
#  echo "18000000" > /proc/sys/kernel/sched_latency_ns;
#  echo "1500000" > /proc/sys/kernel/sched_min_granularity_ns;
#  echo "3000000" > /proc/sys/kernel/sched_wakeup_granularity_ns;
#fi

# Miscellaneous tweaks
if /sbin/busybox [ "`grep MISC /system/etc/tweaks.conf`" ]; then
  echo "2000" > /proc/sys/vm/dirty_writeback_centisecs
  echo "1000" > /proc/sys/vm/dirty_expire_centisecs
fi

# lowmemkiller tweak
if /sbin/busybox [ "`grep OLDLOWMEMKILLER /system/etc/tweaks.conf`" ]; then
  echo "2560,4096,6144,10240,11264,12288" > /sys/module/lowmemorykiller/parameters/minfree
fi

# Enable CIFS tweak
if /sbin/busybox [ "`grep CIFS /system/etc/tweaks.conf`" ]; then
  /sbin/busybox insmod /lib/modules/cifs.ko
else
  /sbin/busybox rm /lib/modules/cifs.ko
fi

# Enable TUN tweak
if /sbin/busybox [ "`grep TUN /system/etc/tweaks.conf`" ]; then
  /sbin/busybox insmod /lib/modules/tun.ko
else
  /sbin/busybox rm /lib/modules/tun.ko
fi

# BacklightNotification
if /sbin/busybox [ -f /system/etc/bln.conf ]; then
	if /sbin/busybox [ $(cat /system/etc/bln.conf) -eq 1 ]; then
		echo 1 > /sys/class/misc/backlightnotification/enabled
	fi
else
	echo 0 > /sys/class/misc/backlightnotification/enabled
fi

# install BacklightNotfication liblights
/sbin/busybox sh /sbin/install_bln_liblights.sh

# We will delete these links including the symlink to CWM's busybox, so they won't interfere with the optionally installed busybox on the device
/sbin/busybox rm /sbin/[
/sbin/busybox rm /sbin/[[
/sbin/busybox rm /sbin/amend
/sbin/busybox rm /sbin/ash
/sbin/busybox rm /sbin/awk
/sbin/busybox rm /sbin/basename
/sbin/busybox rm /sbin/bbconfig
/sbin/busybox rm /sbin/bunzip2
/sbin/busybox rm /sbin/bzcat
/sbin/busybox rm /sbin/bzip2
/sbin/busybox rm /sbin/cal
/sbin/busybox rm /sbin/cat
/sbin/busybox rm /sbin/catv
/sbin/busybox rm /sbin/chgrp
/sbin/busybox rm /sbin/chmod
/sbin/busybox rm /sbin/chown
/sbin/busybox rm /sbin/chroot
/sbin/busybox rm /sbin/cksum
/sbin/busybox rm /sbin/clear
/sbin/busybox rm /sbin/cmp
/sbin/busybox rm /sbin/cp
/sbin/busybox rm /sbin/cpio
/sbin/busybox rm /sbin/cut
/sbin/busybox rm /sbin/date
/sbin/busybox rm /sbin/dc
/sbin/busybox rm /sbin/dd
/sbin/busybox rm /sbin/depmod
/sbin/busybox rm /sbin/devmem
/sbin/busybox rm /sbin/df
/sbin/busybox rm /sbin/diff
/sbin/busybox rm /sbin/dirname
/sbin/busybox rm /sbin/dmesg
/sbin/busybox rm /sbin/dos2unix
/sbin/busybox rm /sbin/du
/sbin/busybox rm /sbin/dump_image
/sbin/busybox rm /sbin/echo
/sbin/busybox rm /sbin/egrep
/sbin/busybox rm /sbin/env
/sbin/busybox rm /sbin/erase_image
/sbin/busybox rm /sbin/expr
/sbin/busybox rm /sbin/false
/sbin/busybox rm /sbin/fdisk
/sbin/busybox rm /sbin/fgrep
/sbin/busybox rm /sbin/find
/sbin/busybox rm /sbin/flash_image
/sbin/busybox rm /sbin/fold
/sbin/busybox rm /sbin/free
/sbin/busybox rm /sbin/freeramdisk
/sbin/busybox rm /sbin/fuser
/sbin/busybox rm /sbin/getopt
/sbin/busybox rm /sbin/grep
/sbin/busybox rm /sbin/gunzip
/sbin/busybox rm /sbin/gzip
/sbin/busybox rm /sbin/head
/sbin/busybox rm /sbin/hexdump
/sbin/busybox rm /sbin/id
/sbin/busybox rm /sbin/insmod
/sbin/busybox rm /sbin/install
/sbin/busybox rm /sbin/kill
/sbin/busybox rm /sbin/killall
/sbin/busybox rm /sbin/killall5
/sbin/busybox rm /sbin/length
/sbin/busybox rm /sbin/less
/sbin/busybox rm /sbin/ln
/sbin/busybox rm /sbin/losetup
/sbin/busybox rm /sbin/ls
/sbin/busybox rm /sbin/lsmod
/sbin/busybox rm /sbin/lspci
/sbin/busybox rm /sbin/lsusb
/sbin/busybox rm /sbin/lzop
/sbin/busybox rm /sbin/lzopcat
/sbin/busybox rm /sbin/md5sum
/sbin/busybox rm /sbin/mkdir
/sbin/busybox rm /sbin/mkfifo
/sbin/busybox rm /sbin/mknod
/sbin/busybox rm /sbin/mkswap
/sbin/busybox rm /sbin/mktemp
/sbin/busybox rm /sbin/mkyaffs2image
/sbin/busybox rm /sbin/modprobe
/sbin/busybox rm /sbin/more
/sbin/busybox rm /sbin/mount
/sbin/busybox rm /sbin/mountpoint
/sbin/busybox rm /sbin/mv
/sbin/busybox rm /sbin/nandroid
/sbin/busybox rm /sbin/nice
/sbin/busybox rm /sbin/nohup
/sbin/busybox rm /sbin/od
/sbin/busybox rm /sbin/patch
/sbin/busybox rm /sbin/pgrep
/sbin/busybox rm /sbin/pidof
/sbin/busybox rm /sbin/pkill
/sbin/busybox rm /sbin/printenv
/sbin/busybox rm /sbin/printf
/sbin/busybox rm /sbin/ps
/sbin/busybox rm /sbin/pwd
/sbin/busybox rm /sbin/rdev
/sbin/busybox rm /sbin/readlink
/sbin/busybox rm /sbin/realpath
/sbin/busybox rm /sbin/reboot
/sbin/busybox rm /sbin/renice
/sbin/busybox rm /sbin/reset
/sbin/busybox rm /sbin/rm
/sbin/busybox rm /sbin/rmdir
/sbin/busybox rm /sbin/rmmod
/sbin/busybox rm /sbin/run-parts
/sbin/busybox rm /sbin/sed
/sbin/busybox rm /sbin/seq
/sbin/busybox rm /sbin/setsid
/sbin/busybox rm /sbin/sh
/sbin/busybox rm /sbin/sha1sum
/sbin/busybox rm /sbin/sha256sum
/sbin/busybox rm /sbin/sha512sum
/sbin/busybox rm /sbin/sleep
/sbin/busybox rm /sbin/sort
/sbin/busybox rm /sbin/split
/sbin/busybox rm /sbin/stat
/sbin/busybox rm /sbin/strings
/sbin/busybox rm /sbin/stty
/sbin/busybox rm /sbin/swapoff
/sbin/busybox rm /sbin/swapon
/sbin/busybox rm /sbin/sync
/sbin/busybox rm /sbin/sysctl
/sbin/busybox rm /sbin/tac
/sbin/busybox rm /sbin/tail
/sbin/busybox rm /sbin/tar
/sbin/busybox rm /sbin/tee
/sbin/busybox rm /sbin/test
/sbin/busybox rm /sbin/time
/sbin/busybox rm /sbin/top
/sbin/busybox rm /sbin/touch
/sbin/busybox rm /sbin/tr
/sbin/busybox rm /sbin/true
/sbin/busybox rm /sbin/truncate
/sbin/busybox rm /sbin/tty
/sbin/busybox rm /sbin/umount
/sbin/busybox rm /sbin/uname
/sbin/busybox rm /sbin/uniq
/sbin/busybox rm /sbin/unix2dos
/sbin/busybox rm /sbin/unlzop
/sbin/busybox rm /sbin/unyaffs
/sbin/busybox rm /sbin/unzip
/sbin/busybox rm /sbin/uptime
/sbin/busybox rm /sbin/usleep
/sbin/busybox rm /sbin/uudecode
/sbin/busybox rm /sbin/uuencode
/sbin/busybox rm /sbin/watch
/sbin/busybox rm /sbin/wc
/sbin/busybox rm /sbin/which
/sbin/busybox rm /sbin/whoami
/sbin/busybox rm /sbin/xargs
/sbin/busybox rm /sbin/yes
/sbin/busybox rm /sbin/zcat

# Remove the included shared modules, they can be also found on /system anyway
# /sbin/busybox rm /lib/*.so

# we'll need mount to remount the rootfs ro
/sbin/busybox ln -s /sbin/recovery /res/mount

# self destruct :)
/sbin/busybox rm /sbin/busybox

echo $(date) Unloading and deleting unused modules...
# unload un-needed filesystem modules
if [ -z "`/res/mount | grep jfs`" ]; then
	rmmod jfs
	#rm /lib/modules/jfs.ko
fi
if [ -z "`/res/mount | grep ext3`" ]; then
	rmmod ext3
	rmmod jbd
	rm /lib/modules/ext3.ko
	rm /lib/modules/jbd.ko
fi
if [ -z "`/res/mount | grep ext4`" ]; then
	rmmod ext4
	rmmod jbd2
	rm /lib/modules/ext4.ko
	rm /lib/modules/jbd2.ko
fi
#if [ -z "`/res/mount | grep ext2`" ]; then
#	rmmod ext2
#	rm /lib/modules/ext2.ko
#fi

echo $(date) Free up RAM by deleting files from initramfs...
rm /sbin/fsck.jfs /sbin/mkfs.jfs /sbin/mke2fs /sbin/tune2fs /sbin/e2fsck /sbin/realsu /sbin/fat.format
rm /sbin/fsck* /sbin/mkfs*

# rootfs and system should be closed for now
#/res/mount -o remount,ro /
#/res/mount -o remount,ro /system

# continue with playing the logo
# play android bootanimation.zip if it exists, if not play playlogos1
if [ ! -f /data/local/bootanimation.zip ] && [ ! -f /system/media/bootanimation.zip ];
then
setprop audioflinger.bootsnd 1
exec /system/bin/playlogos1
fi
