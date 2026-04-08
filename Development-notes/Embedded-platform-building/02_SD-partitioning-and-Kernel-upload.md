## Kernel image selection

Two common kernel image formats can be used with U-Boot: `zImage` and `uImage`.

- **zImage**
This is the native compressed Linux kernel format for ARM. It does not include a U-Boot header and is loaded using the `bootz` command. It is simpler, more direct, and commonly used in modern embedded Linux systems.

- **uImage**
This format wraps the `zImage` with an additional U-Boot header using the `mkimage` tool. The header includes metadata such as load address, entry point, and checksum. It is loaded using the `bootm` command and is typically used in legacy or more controlled boot environments.

In this project, **zImage is selected** due to its simplicity, alignment with the mainline Linux workflow, and better suitability for learning and debugging in an embedded environment.

# 1) SD formatting and partitioning

Using the Linux utility **fdisk**, it is possible to create, modify, and delete partitions on any storage device. In this case, the SD card is located at **/dev/sda**. Once the correct device path is known, the SD card can be prepared. 

## Kernel Partition

Using the following commands the Kernel partition is created:

```
sudo fdisk /dev/sda

d # Repeat until every partition has been deleted.
n # Add new partition
p # Type of partition (Primary)
1 # Partation number
35360 # First sector
59935 # 12MB
p # Check that the partition has been created and its size is 12M

# FAT format 
t # change a partition type
c # 0c W95 FAT32 (LBA) 

w # Write on partition table and exit
```

The exact location of the first and last sectors is determined by the memory space required to store the **Linux kernel** on the SD card. The first sector is set to **35360** because the initial sectors of the SD card are reserved for the bootloader, boot configuration data, and other low-level system components required by the SoC during the boot process.

## Debian partition

Using the following commands the Kernel partition is created:

```
sudo fdisk /dev/sda

n # Add new partition
p # Type of partition (Primary)
2 # Partation number
59936 # First sector
<enter>  # let fdisk automatically align and use the remaining available space
p # Check that the partition has been created

# ext4 format 
t # change a partition type
2 # Partiton number
83 # Linux

w # Write on partition table and exit
```

Since no additional partitions are required, the remaining storage space can be fully allocated to the Debian root filesystem. In this particular case, the resulting **partition size is approximately 7.3 GB**. 


# 2) Kernel uploading to SD partition

## Creation of filesystem for the kernel partition

Using the following command, a **filesystem is created for the kernel partition**. In previous steps, the partition was defined and its type was assigned using `fdisk`. However, this only updates the partition table metadata and does not create an actual usable filesystem.

Therefore, it is necessary to explicitly generate the filesystem structure within the partition:

```
sudo mkfs.vfat -F 32 /dev/sda1
```

This step initializes the FAT32 filesystem, enabling the partition to store and manage files such as the kernel image (`zImage`) and the device tree blobs (DTBs).

## Mounting the boot partition

The next command attaches (mounts) the filesystem of the partition `/dev/sda1` to the directory `/mnt/boot`:

```
sudo mkdir -p /mnt/boot
sudo mount /dev/sda1 /mnt/boot
```

After executing the mount command, the system attaches the partition `/dev/sda1` to the directory `/mnt/boot`, making its contents accessible through the standard filesystem hierarchy.

In practical terms, this means that `/mnt/boot` becomes a **direct representation of the SD card’s boot partition**. Any file operations performed within this directory (such as copying, deleting, or modifying files) are **applied directly to the SD card**.

Before mounting, `/mnt/boot` is simply an empty directory (or a regular folder on the host system). Once the mount operation is completed, its contents are temporarily replaced by the filesystem stored in `/dev/sda1`. This abstraction **allows Linux to treat physical storage devices as part of a unified directory tree**.

- `/dev/sda1`: the first partition of the SD card, which contains the FAT filesystem used for boot files  
- `/mnt/boot`: the mount point, the directory through which the partition is accessed  
- `mount`: the command that links a block storage device to a directory in the filesystem  

It is important to note that while the partition is mounted, `/mnt/boot` no longer refers to the local filesystem of the host machine, but instead to the SD card. **Once the partition is unmounted, the directory returns to its original state.**

This mechanism is fundamental in Linux systems, as it enables seamless interaction with different storage devices using a consistent file-based interface.

	Important: Always unmount the partition using `sudo umount /mnt/boot` before removing the SD card to prevent data corruption.

## Copying files to Kernel partition

### zImage copying

Now that the partition has been mounted and is accessible through the host system’s filesystem, the generated kernel image can be copied using the following command:

```
sudo cp linux/arch/arm/boot/zImage /mnt/boot/
```

This step transfers the compiled kernel (`zImage`) into the boot partition of the SD card. Since `/mnt/boot` is currently mapped to `/dev/sda1`, **any file copied to this directory is physically written to the SD card.**

The purpose of this step is to **make the kernel available to the bootloader (U-Boot)**. During the boot process, U-Boot will load this `zImage` from the FAT partition into RAM and execute it using the `bootz` command, initiating the Linux kernel startup sequence.

### DTB copying.

Next, the corresponding Device Tree Blob (DTB) file for the target board is copied into the mounted boot partition:

```
sudo cp linux/arch/arm/boot/dts/allwinner/sun8i-t113s-saxo-gateway.dtb /mnt/boot/
```

The purpose of the DTB is to **describe the hardware configuration of the target board** to the Linux kernel. It contains information about the CPU, memory, peripherals, and buses. During the boot process, U-Boot loads both the kernel (`zImage`) and the corresponding DTB into memory. 

### Synchronizing and unmounting the boot partition

After copying the required files into the mounted partition, the following commands are executed:

```
sync
sudo umount /mnt/boot
```


The `sync` command ensures that **all pending write operations are flushed from system buffers to the physical storage device**. This guarantees that all copied files (such as `zImage` and the DTB) are fully written to the SD card before it is accessed or removed.

The `umount` command **safely detaches the partition from the filesystem.** Once unmounted, the directory `/mnt/boot` is no longer linked to the SD card, and the system ensures that no further operations are performed on the device.

	This step is critical to prevent data corruption, as removing the SD card without properly unmounting it may result in incomplete writes or filesystem damage.