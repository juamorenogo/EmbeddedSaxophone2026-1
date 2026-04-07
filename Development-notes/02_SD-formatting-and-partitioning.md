
##  1) Kernel details for partitioning

After running:

```
./build_kernel.sh
```

the kernel build process generates all the required binaries to boot the system on the target platform. This process is not only compiling the Linux kernel, but also preparing it to be compatible with the **U-Boot bootloader**, which is responsible for loading the system at startup.

In this workflow, the kernel is first compiled in its native format and then transformed into a **U-Boot compatible image (uImage)**. Additionally, the system generates **Device Tree Blobs (DTB)** that describe the hardware configuration.

---
### Generated files

The build process produces multiple outputs, but not all of them are directly used during boot. It is important to distinguish between **intermediate artifacts** and **final bootable components**.

##### a) Compressed kernel (base)

- **Path:** linux/arch/arm/boot/zImage  
- **Type:** compressed Linux kernel image  

This file is the direct output of the Linux kernel compilation. It contains the compressed kernel binary and is considered the **standard kernel artifact** in Linux systems.

However, in this project it is not used directly, because U-Boot requires a specific image format. Therefore, this file serves as an **intermediate step** before generating the final bootable image.

##### b) U-Boot kernel image (main)

- **Path:** uImage  
- **Type:** U-Boot wrapped kernel image  
- **Generated via:** mkimage  

This is the **main kernel file used during the boot process**. It is created by wrapping the `zImage` with a header that contains metadata required by U-Boot.

This header includes critical information such as:
- load address  
- entry point  
- image type  

Without this format, U-Boot cannot correctly interpret or execute the kernel.

- **Build parameters:**
  - **Size:** ~5.64 MB  
  - **Load address:** 0x41800000  
  - **Entry point:**  0x41800000  

These parameters must remain consistent with the **U-Boot configuration**, otherwise the system may fail at boot time.
##### c) Device Tree Blobs (DTB)

- **Path:** linux/arch/arm/boot/dts/allwinner/*.dtb  
- **Type:** hardware description binary  

The DTB files define the **complete hardware configuration of the system**, including:
- peripherals  
- memory layout  
- clocks  
- pin multiplexing (pinmux)  

The Linux kernel uses this file during early boot to understand how to interact with the hardware.

A mismatch between the DTB and the actual hardware (or U-Boot configuration) can lead to:
- kernel panic  
- missing peripherals  
- system freeze during boot  

For this reason, the DTB must be **aligned with both the custom SAXO board design and the modifications applied in U-Boot and kernel sources**.
##### d) Kernel modules

- **Path:** generated during build (modules/)  
- **Type:** loadable kernel objects (.ko)  

Kernel modules are optional components that extend the kernel functionality. They are loaded after the system boots and are typically used for:
- device drivers  
- optional subsystems  

They are not required for the initial boot process, but they are necessary for a fully functional system.

---
### Files required for boot

To successfully boot the system, only a minimal set of files is required. These files are loaded by U-Boot from the SD card.

- **uImage**  
- **Corresponding .dtb file**

Both must be present and correctly referenced by the bootloader. Missing or incorrect files will prevent the system from booting.

## 1a) Final Implications of the Kernel Build
### Partitioning implications

The size and structure of the generated kernel directly influence how the **boot partition** should be defined on the SD card.

- **Kernel size:** ~5.64 MB  

The boot partition must include:
- **uImage**  
- **DTB files**  
- optional boot scripts (e.g., boot.scr)

Although the kernel itself is relatively small, additional space is required for flexibility, updates, and debugging.

- **Recommended sizes (IA advice):**
  - **Minimum:** 32 MB  
  - **Recommended:** 64 MB  

Allocating sufficient space avoids future repartitioning when adding features or debugging tools.

---
### Validation checks

Before deploying the kernel to the SD card, it is necessary to verify that all required artifacts were correctly generated.

- **Check kernel image:** ls uImage  
- **Check DTB files:** ls linux/arch/arm/boot/dts/allwinner/*.dtb  
- **Inspect image metadata:** file uImage  
- **Inspect U-Boot header:** mkimage -l uImage  

These checks ensure that the kernel image is valid and properly formatted for U-Boot.

## 2) SD formatting and partitioning

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
