
---
## Kernel

The Linux kernel is responsible for low-level communication with the hardware. In many case (especially in embedded systems) much of this process is already automated. The process of adapting the Linux kernel and the filesystem to a specific platform can generally be performed using two approaches:

- **Buildroot**
- **A system image assembled from individual components**

In many situations, a Buildroot configuration is already provided for a specific platform or development board. This is the case for the **Allwinner T113s**, where existing configurations can be used as a starting point for building a functional embedded Linux system. 

After building and installing the bootloader, the next step consists of compiling the Linux kernel adapted to the target hardware platform. The base repository relies on the official Linux kernel source tree. However, several modifications are required in order to support the custom SAXO development board based on the Allwinner T113-S3 processor. It uses the script *build_kernel.sh* to apply several specific modifications before compiling the kernel:

```

#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

cd $SCRIPT_DIR

cp linux-patch-6.16.9/sun8i-t113s-saxo-gateway.dts linux/arch/arm/boot/dts/allwinner
cp linux-patch-6.16.9/sunxi-d1s-t113s-saxo.dtsi     linux/arch/arm/boot/dts/allwinner
cp linux-patch-6.16.9/config  linux/.config

cd linux

git checkout -f

patch -d . -p1 < ../linux-patch-6.16.9/0001-saxo-dtb-reference.patch

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- menuconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- zImage dtbs modules -j4

cd $SCRIPT_DIR

LOAD_ADDR=0x41800000
ENTRY_ADDR=0x41800000

mkimage -A arm -O linux -T kernel -C none \
  -a $LOAD_ADDR -e $ENTRY_ADDR \
  -n "SAXO Linux Kernel (T113-S3)" \
  -d ./linux/arch/arm/boot/zImage uImage
echo "SAXO Linux Kernel (T113-S3)" : uImage

```


To achieve this, specific Device Tree files describing the hardware configuration of the board are copied into the Linux source tree. In addition, a predefined kernel configuration file is provided to enable the required drivers and subsystems.

A patch is also applied to the kernel source code in order to register the new Device Tree within the build system. Once these modifications are in place, the kernel, device tree blobs, and kernel modules are compiled using a cross-compiler targeting the ARM architecture.

Finally, the resulting kernel image is converted into a U-Boot compatible format using the `mkimage` tool. This generates a `uImage` file that can be loaded by the bootloader during the system startup process.