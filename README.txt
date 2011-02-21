################################################################################

1. How to Build
	- get Toolchain
		From Code Sourcery site ( www.codesourcery.com )
		Ex) Download :http://www.codesourcery.com/public/gnu_toolchain/arm-none-linux-gnueabi/arm-2009q3-67-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2
			     or  http://code.google.com/p/smp-on-qemu/downloads/detail?name=arm-2009q3-67-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2&can=2&q= 

		recommand : later than 2009q3 version.
					Feature : ARM
					Target OS : "EABI"
					package : "IA32 GNU/Linux TAR"

	- edit build_kernel.sh
		edit "TOOLCHAIN" and "TOOLCHAIN_PREFIX" to right toolchain path(You downloaded).
		Ex)  30 TOOLCHAIN=/usr/local/toolchain/arm-2009q3/bin/		// check the location of toolchain
		     31 TOOLCHAIN_PREFIX=arm-none-gnueabi-			// You have to check.

	- run build_kernel.sh
		$ ./build_kernel.sh

2. Output files
	- Kernel : Kernel/arch/arm/boot/zImage
	- module : Kernel/drivers/*/*.ko

3. How to Clean
	- run build_kernel.sh Clean
		$ ./build_kernel.sh Clean

4. How to make .tar binary for downloading into target.

	- change current directory to Kernel/arch/arm/boot
	- type following command
		$ tar cvf sph-D700-kernel.tar zImage

################################################################################
