#!/sbin/sh
# DRockstar Clean Kernel recovery.sh called from recovery.rc and fota.rc

/sbin/busybox mount -o remount,rw / /

# Install recovery busybox to /sbin
/sbin/busybox ls /res/sbin | while read line
do
  /sbin/busybox mv -f /res/sbin/$line /sbin/$line
done
rmdir /res/sbin
chmod 6755 /sbin/su

# Remove unneccessary files used in normal boot
for file in Superuser.apk keytimer
do
  rm -rf /sbin/$file
done

# Fix permissions in /sbin, just in case
/sbin/busybox chmod 755 /sbin/*

# Fix screwy ownerships
for blip in conf default.prop fota.rc init init.goldfish.rc init.rc init.smdkc110.rc lib lpm.rc modules recovery.rc res sbin
do
  chown root.system /$blip
  chown root.system /$blip/*
done

chown root.system /lib/modules/*
chown root.system /res/images/*

mkdir /etc
cp /res/etc/recovery.fstab /etc/recovery.fstab
sync
/sbin/recovery
