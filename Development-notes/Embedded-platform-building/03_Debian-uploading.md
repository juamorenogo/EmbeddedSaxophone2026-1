
# Debian-preparations

## Arch does not work for this part.

It was necessary to install a Debian Virtual Machine to be able to make the next steps.

## Debian Filesystem building and partition mounting

Once the partitioning step is completed, it is necessary to create a filesystem for the Debian root partition. In this case, an **ext4 filesystem** is used, which is the standard filesystem for Linux systems:

```
sudo mkfs.ext4 /dev/sda2
```

After creating the filesystem, the partition must be mounted to make it accessible from the host system:

```
sudo mkdir -p /mnt/rootfs
sudo mount /dev/sda2 /mnt/rootfs
```

This mounts the physical partition `/dev/sda2` into the directory `/mnt/rootfs`, allowing files to be written directly into the SD card. This directory will temporarily act as the root (`/`) of the future Debian system.

## Debian debootstrap

The `debootstrap` tool is used to create a minimal Debian root filesystem from scratch, without requiring a pre-existing installation. The following command is used for this purpose:

```
sudo debootstrap --arch=armhf bookworm /mnt/rootfs
```

This command **downloads and installs a basic Debian system** into the mounted partition. It creates the essential directory structure and installs the core packages required for a functional Linux environment. After execution, the following structure is generated inside `/mnt/rootfs`:

	/bin  
	/etc  
	/lib  
	/usr  
	/sbin


Most importantly, it installs`/sbin/init`, the **first user-space process executed by the kernel**. Additionally, it installs core libraries required for system operation  and basic system utilities .

During the boot process, the Linux kernel will **mount this partition as the root filesystem** and execute `/sbin/init`, which starts the Debian user space. This step effectively transforms the SD card from containing only a kernel into a complete, bootable Linux system.

## Debootstrap warning: chroot execution failure

During the execution of `debootstrap`, the following warning may appear:

```
W: Failure trying to run: chroot "/mnt/rootfs" /bin/true
```


This message can be misleading. It is **expected in cross-architecture environments**, but it also indicates that the installation is **incomplete** and requires an additional step. The root cause of this warning is a **cross-architecture mismatch**:

- Host system (development machine): `x86_64`  
- Target system (embedded board): `armhf`  

During the second stage of `debootstrap`, the tool attempts to execute a binary (`/bin/true`) inside the newly created root filesystem using `chroot`. However, this binary is compiled for the ARM architecture, and therefore cannot be executed on an x86_64 host system.

As a result, the second stage of `debootstrap` is not completed automatically.

After this step:

- The Debian root filesystem is **partially created**  
- Base packages are downloaded and extracted  
- The directory structure (`/bin`, `/etc`, `/lib`, `/usr`, `/sbin`) exists  

However:

- System packages are **not fully configured**  
- `systemd` may not be installed correctly  
- `/sbin/init` may be missing  
- The system is **not yet bootable**

