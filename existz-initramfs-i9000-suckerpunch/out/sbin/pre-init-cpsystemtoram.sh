#!/sbin/busybox sh

	#copy needed /system stuff to ram
	echo "make a limited copy of /system in ram"
	/sbin/busybox mkdir -p /system_in_ram/bin
	cp	/system/bin/toolbox \
		/system/bin/sh \
		/system/bin/log \
		/system/bin/linker \
		/sbin/mke2fs \
		/sbin/fat.format /system_in_ram/bin/
	
	/sbin/busybox ln -s /system_in_ram/bin/mke2fs /system_in_ram/bin/mkfs.ext4

	/sbin/busybox mkdir -p /system_in_ram/lib/
	cp 	/system/lib/liblog.so \
		/system/lib/libc.so \
		/system/lib/libstdc++.so \
		/system/lib/libm.so \
		/system/lib/libcutils.so /system_in_ram/lib/
	
	/sbin/busybox mkdir -p /system_in_ram/etc/
	cp /res/misc/mke2fs.conf /system_in_ram/etc/
	
	umount /system
	rm -rf /system/bin
	ln -s /system_in_ram/* /system