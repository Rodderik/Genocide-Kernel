#!/sbin/busybox sh

/sbin/busybox cp /sbin/fat.format /system/bin/fat.format

# use default recovery from rom if we have an update to process, so things like CSC updates work
# Use CWM if we simply entered recovery mode by hand, to be able to use it's features
if /sbin/busybox [ -f "/cache/recovery/command" ];
then
  # if we use the original recovery it might modify the filesystem, so we erase our config, in order to avoid regeneration
  # of the filesystems. The drawback is that the lagfix has to be re-applied by hand
  /sbin/busybox rm /system/etc/lagfix.conf
  /sbin/busybox rm /system/etc/lagfix.conf.old
  rm /sdcard
  mkdir /sdcard
  #/sbin/rec2e &
  /system/bin/recovery &
else
  # TODO: CWM is a bit more intelligent, but it might fail to
  /sbin/recovery &
fi;

