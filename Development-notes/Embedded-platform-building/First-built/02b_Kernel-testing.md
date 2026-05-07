
# Kernel bring-up testing over UART

To test the kernel, it is required that U-Boot has already been correctly written to the SD card and that the kernel image (`zImage`) along with the corresponding DTB have been properly copied into the boot partition.

If all previous steps were successful, communication through **UART0** should be functional, allowing interaction with the bootloader and observation of the kernel boot process.

# Serial communication with UART0 (minicom)

The serial interface can be accessed using `minicom` with the following configuration:

```
sudo minicom -D /dev/ttyUSB0 -b 115200
```

## Expected results

After powering the board and connecting through `minicom`, the boot sequence can be observed through the serial console.

Initially, **U-Boot starts automatically** and prints its initialization messages. Once in the U-Boot console, the **kernel and Device Tree Blob (DTB) must be loaded manually** from the SD card into memory using the following commands:

```
fatload mmc 0:1 ${kernel_addr_r} zImage  
fatload mmc 0:1 ${fdt_addr_r} sun8i-t113s-saxo-gateway.dtb
```

After both the kernel and DTB are loaded into RAM, the kernel can be executed with:

```
bootz ${kernel_addr_r} - ${fdt_addr_r}
```

At this point, the Linux kernel begins its execution. The serial console should display:

- Kernel decompression messages  
- CPU and memory initialization logs  
- Hardware detection based on the DTB  

Finally, since no root filesystem is present yet, the system will stop with a **kernel panic related to the root filesystem**. This behavior is expected and confirms that the kernel has been successfully loaded and executed.

#### Kernel starting.

![](First-built/Images/Fkernelstart.png)

#### Kernel panic.

![](First-built/Images/FKernelpanic.png)