#!/sbin/busybox_disabled sh
# play android bootanimation.zip if it exists, if not play playlogos1
if [ -f /data/local/bootanimation.zip ] || [ -f /system/media/bootanimation.zip ];
then
exec /system/bin/bootanimation
fi

