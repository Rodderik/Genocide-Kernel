#!/sbin/sh
# Custom script to intercept init binary to determine which init.rc to boot
# By Rodderik for Epic 4G Genocide kernel

#environment stuff
set -x
PATH=/sbin:/res/sbin/

# create used devices nodes
mkdir /dev
mkdir /dev/block
mkdir /dev/snd

# standard
mknod /dev/null c 1 3
mknod /dev/zero c 1 5

# internal & external SD
mknod /dev/block/mmcblk0 b 179 0
mknod /dev/block/mmcblk0p1 b 179 1
mknod /dev/block/mmcblk0p2 b 179 2
mknod /dev/block/mmcblk0p3 b 179 3
mknod /dev/block/mmcblk0p4 b 179 4
mknod /dev/block/mmcblk1 b 179 8
mknod /dev/block/stl1 b 138 1
mknod /dev/block/stl2 b 138 2
mknod /dev/block/stl3 b 138 3
mknod /dev/block/stl4 b 138 4
mknod /dev/block/stl5 b 138 5
mknod /dev/block/stl6 b 138 6
mknod /dev/block/stl7 b 138 7
mknod /dev/block/stl8 b 138 8
mknod /dev/block/stl9 b 138 9
mknod /dev/block/stl10 b 138 10
mknod /dev/block/stl11 b 138 11
mknod /dev/block/stl12 b 138 12

# soundcard
mknod /dev/snd/controlC0 c 116 0
mknod /dev/snd/controlC1 c 116 32
mknod /dev/snd/pcmC0D0c c 116 24
mknod /dev/snd/pcmC0D0p c 116 16
mknod /dev/snd/pcmC1D0c c 116 56
mknod /dev/snd/pcmC1D0p c 116 48
mknod /dev/snd/timer c 116 33

# insmod required modules
insmod /lib/modules/fsr.ko
insmod /lib/modules/fsr_stl.ko
insmod /lib/modules/rfs_glue.ko
insmod /lib/modules/rfs_fat.ko
insmod /lib/modules/j4fs.ko
insmod /lib/modules/dpram.ko

#create temporary mount point
mkdir /rodd

#mount sdcard
mount -t vfat -o rw,utf8 /dev/block/mmcblk0p1 /rodd

#file check
if [ -f "/rodd/bootsdcard" ]; then

    #replace .rc file(s)
    rm -f /init.rc
    ln -s /init.rc.sdcard /init.rc

fi

#copy log before we umount and lose it on a bad boot
cat /init.log >> /rodd/init.log
sync

#unmount temp sdcard directory
umount /rodd

#rmdir instead of rm -R so we dont accidently nuke sdcard
rmdir /rodd

#call init binary and continue boot
exec /sbin/init
