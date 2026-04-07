
## 1) SD formatting and partitioning

Using the Linux utility **fdisk**, it is possible to create, modify, and delete partitions on any storage device. In this case, the SD card is located at **/dev/sda**. Once the correct device path is known, the SD card can be prepared. 

### Kernel Partition

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

### Debian partition

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