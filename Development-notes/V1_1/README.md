
---
## General context of the Kernel Built

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

A patch is also applied to the kernel source code in order to register the new Device Tree within the build system. Once these modifications are in place, the kernel, device tree blobs, and kernel modules are compiled using a cross-compiler targeting the ARM architecture. Finally, the resulting kernel image is converted into a U-Boot compatible format using the `mkimage` tool. This generates a `uImage` file that can be loaded by the bootloader during the system startup process.

However, since some changes were previously made to U-Boot, it is necessary to verify that the patch still applies correctly. In other words, each file modified by the patch must be reviewed to ensure that the changes are still valid. Additionally, the build script should be checked to confirm that its execution flow remains logical and consistent with the current modifications.

---

## Checking Linux DTS

### 1) sunxi-d1s-t113s-saxo.dtsi

The same modifications performed in the U-Boot configuration must be applied here as well. In general, this consists of enabling **UART0** and disabling **UART3** in order to match the hardware configuration of the board:

```
&uart3 {        
        pinctrl-names = "default";
        pinctrl-0 = <&uart3_pb_pins>;
        status = "disabled";
};

&uart0 {        
        pinctrl-names = "default";
        pinctrl-0 = <&uart0_pe2_pins>;
        status = "okay";
};
```

Additionally, the following code section must be modified to specify which serial port will be used as the CPU's RX–TX interface:

```
	chosen {
		stdout-path = "serial3:115200n8";
	};
	
	# to
	
		chosen {
		stdout-path = "serial0:115200n8";
	};
```

### 2) Kernel configuration file (.config)

The Linux kernel build system uses a configuration file named `.config` located at the root of the kernel source tree. This file defines all the options used during the compilation process, including enabled drivers, supported subsystems, architecture settings, and hardware-specific features.

Each configuration option follows the format:

```
CONFIG_OPTION=value
```

These options determine which components are compiled directly into the kernel, compiled as loadable modules, or excluded from the build. This ensures that the kernel is compiled with the correct configuration required for the target hardware platform (Allwinner T113-S3). Using a predefined configuration avoids the need to manually enable the required drivers and features through the `menuconfig` interface.

Once the `.config` file is present, the Linux build system automatically uses it to determine which components must be compiled when executing the `make` command.


### 3) Update `build_kernel.sh`

The `build_kernel.sh` script also contains the command `git checkout -f`. As in the U-Boot build script, this command is moved to the beginning of the script in order to avoid unnecessary overwrites of the modified files.

Executing `git checkout -f` at the start ensures that the Linux source tree is restored to a clean state before applying the required patches and copying the modified configuration files.

The final version of the script is shown below:

```
#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

cd linux
git checkout -f

cd $SCRIPT_DIR

cp linux-patch-6.16.9/sun8i-t113s-saxo-gateway.dts linux/arch/arm/boot/dts/allwinner
cp linux-patch-6.16.9/sunxi-d1s-t113s-saxo.dtsi     linux/arch/arm/boot/dts/allwinner
cp linux-patch-6.16.9/config  linux/.config

cd linux

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

### 4) Kernel build

The kernel build script is then executed to verify that the compilation process completes successfully. This step allows checking that the applied modifications, patches, and configuration files are consistent and do not introduce compilation errors.

The build process generates the kernel image, the corresponding Device Tree binaries, and the required kernel modules for the target platform.



---
## SD Preparation

### 1) SD formatting and partitioning

Using the Linux utility **fdisk**, it is possible to create, modify, and delete partitions on any storage device. In this case, the SD card is located at **/dev/sda**. Once the correct device path is known, the SD card can be prepared using the following commands:


```
sudo fdisk /dev/sda
d # Repeat until every partition has been deleted.
n # Add new partition
p # Type of partition (Primary)
1 # Partation number
35360 # First sector
+21M # Last sector oder aprox size of partition.
p # Check that the partition has been created and its size ies 512K
w # Write table to disk and exit
```

The exact location of the first and last sectors is determined by the memory space required to store the **Linux kernel** on the SD card. The first sector is set to **35360** because the initial sectors of the SD card are reserved for the bootloader, boot configuration data, and other low-level system components required by the SoC during the boot process.

