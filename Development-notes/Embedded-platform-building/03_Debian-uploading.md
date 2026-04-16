
# Debian-preparations

## Arch does not work for this part.

It was necessary to install a Debian Virtual Machine to be able to make the next steps.  Para esto, ademas de configurar la VM, es necesario instalar las siguientes dependencias:

```
sudo apt install -y debootstrap fakeroot qemu-user-static binfmt-support
```

## Debian Filesystem building and partition mounting

Once the partitioning step is completed, it is necessary to create a filesystem for the Debian root partition. In this case, an **ext4 filesystem** is used, which is the standard filesystem for Linux systems:

```
sudo mkfs.ext4 /dev/sdb2
```

After creating the filesystem, the partition must be mounted to make it accessible from the host system:

```
sudo mkdir -p /mnt/sd
sudo mount /dev/sdb2 /mnt/sd
```

This mounts the physical partition `/dev/sdb2` into the directory `/mnt/sd`, allowing files to be written directly into the SD card. This directory will temporarily act as the root (`/`) of the future Debian system.

## Debian debootstrap

The `debootstrap` tool is used to create a minimal Debian root filesystem from scratch, without requiring a pre-existing installation.

In cross-architecture scenarios (e.g., creating an ARM root filesystem from an x86 host), the process requires two stages. The first stage downloads and extracts the base system, while the second stage completes the installation by executing target-architecture binaries using emulation.

The following command performs the first stage:

```
sudo debootstrap --arch=armhf bookworm /mnt/sd https://deb.debian.org/debian
```


This command **downloads and installs a basic Debian system** into the mounted partition. It creates the essential directory structure and installs the core packages required for a functional Linux environment. After execution, the following structure is generated inside `/mnt/rootfs`:

	/bin  
	/etc  
	/lib  
	/usr  
	/sbin


Most importantly, it installs`/sbin/init`, the **first user-space process executed by the kernel**. Additionally, it installs core libraries required for system operation  and basic system utilities .

During the boot process, the Linux kernel will **mount this partition as the root filesystem** and execute `/sbin/init`, which starts the Debian user space. This step effectively transforms the SD card from containing only a kernel into a complete, bootable Linux system.

To verify that the Debian root filesystem has been correctly installed, the following command can be used:

```
sudo chroot /mnt/sd
```

If the command successfully provides access to a **root shell within the new filesystem**, it confirms that the base system was properly installed and is functional. This means that the essential directory structure, system binaries, and shared libraries were correctly created by debootstrap.

The command works by changing the root directory of the current process to the specified path. In Linux, the root directory (/) is the starting point for resolving all absolute paths. By changing it to /mnt/sd, the system makes that directory behave as if it were the real root filesystem.

As a result, any command executed after entering the environment will operate inside the new filesystem. For example, accessing /etc/fstab will actually refer to /mnt/sd/etc/fstab on the host system.

This is necessary because the target system (the embedded board) is not yet running. Using this method allows configuring the system in advance, ensuring that critical files such as fstab, hostname, and network settings are correctly defined before the first boot.

## ## Initial configuration on chroot


Once inside the chroot environment, several essential configurations must be performed to ensure that the system can boot and operate correctly on the target hardware. These steps **prepare the minimal filesystem** created by debootstrap to behave as a complete and functional Linux system.

---

The filesystem table defines how storage devices are mounted during the boot process. The root filesystem must be explicitly declared so that the kernel knows where to mount it.

To modify this file, use the following command:

```
nano /etc/fstab
```

Then, **add or edit the root filesystem entry** as follows:

```
/dev/mmcblk0p2  /  ext4  defaults  0  1
```

This step is necessary to ensure that the root filesystem is mounted automatically during boot. Without this configuration, the system may fail to start properly.

---

The hostname identifies the system within a network and is required by various system services.

To configure the hostname, use the following command:

```
echo "t113" > /etc/hostname
```

Additionally, the ``/etc/hosts`` file must be configured to correctly resolve the hostname locally.

To edit this file, use the following command:

```
nano /etc/hosts
```

Then, ensure it contains the following entries:

```
127.0.0.1   localhost
127.0.1.1   t113
```

This configuration ensures proper local name resolution and avoids issues with services that depend on hostname lookup.

---

To enable DNS resolution, at least one nameserver must be defined.

To configure DNS, use the following command:

```
echo "nameserver 8.8.8.8" > /etc/resolv.conf
```

Without this configuration, the system will not be able to resolve domain names, which affects networking and package management.

---

Since embedded systems typically rely on a serial interface instead of a graphical environment, a login prompt must be enabled over UART.

To enable a serial login console, use the following command:

```
ln -s /lib/systemd/system/serial-getty@.service /etc/systemd/system/getty.target.wants/serial-getty@ttyS0.service
```

This creates a symbolic link that enables a getty service on the ttyS0 interface. As a result, once the system boots, a login prompt will be available through the serial console, allowing interaction using tools such as minicom.

---

Finally, a password must be set for the root user to allow system access.

To set the root password, use the following command:

```
passwd
0000
0000
```

This step is required to ensure secure and controlled access to the system after boot.

---

These configurations complete the initialization of the Debian root filesystem, ensuring that the system is properly prepared for its first boot on the target embedded platform.