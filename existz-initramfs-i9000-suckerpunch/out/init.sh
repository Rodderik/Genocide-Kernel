#!/sbin/busybox sh

/sbin/pre-init.sh > /res/pre-init.log 2>&1

# if a fatal error occured display it, then restart if not in recovery mode
if /sbin/busybox [ -f "/res/fatal.error" ]; then
  /sbin/graphsh "/sbin/busybox cat /res/pre-init.log"
  if /sbin/busybox [ -z "`/sbin/busybox grep 'bootmode=2' /proc/cmdline`" ]; then
    /sbin/busybox sleep 30s
    /sbin/busybox ln -s /sbin/recovery /sbin/reboot
    /sbin/reboot -f
  fi
fi

exec /oldinit
