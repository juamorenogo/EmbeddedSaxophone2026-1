
---

It is necessary to get the necessary submodules that the based on repository asks, but it does not work if some changes are not made. 

First, the SD card must be cleaned and formatted in order to correctly deploy the embedded system image. The following image illustrates the required SD logic :


![F1](Images/F1.png)


In version 1.0, the initial step is to compile **U-Boot** and create the necessary SD card partitions. The steps for preparing the SD card and fixing U-boot are described below. 


---
## U-boot fix

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
36383 # Last sector
p # Check that the partition has been created and its size ies 512K
w # Write table to disk and exit
```

The exact location of the first and last sectors is determined by the memory space required to store the **U-Boot bootloader** on the SD card. The first sector is set to **35360** because the initial sectors of the SD card are reserved for the bootloader, boot configuration data, and other low-level system components required by the SoC during the boot process.

a
### 2) 



