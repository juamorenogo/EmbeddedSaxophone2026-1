
---
## General context of the Kernel Built

The Linux kernel is responsible for low-level communication with hardware. In embedded systems, much of this process is typically automated. Adapting the kernel and filesystem to a specific platform can generally be done in two ways:

- **Buildroot**
- **A system image assembled from individual components**

After installing the bootloader, the next step is to compile the Linux kernel for the target hardware. The base repository uses the official kernel source but includes a script (`build_kernel.sh`) that automates required modifications before compilation. This script is intended to automate several key steps:

- Copying custom Device Tree files (DTS) that describe the SAXO board hardware.
- Providing a kernel configuration file with the required drivers and subsystems.
- Applying a patch to register the new Device Tree within the kernel build system.

Additionally, the original repository of this project includes not only this script, but also a Makefile and DTS files that are meant to support the build process. However, in practice, these components are not fully consistent and **require modification** in order to correctly generate a valid kernel image.

In particular, the original script is incomplete and, in some parts, incorrect. **It is necessary to extend it by adding missing steps, as well as to review and adjust several existing ones to ensure a coherent and functional build flow.** The same applies to the associated Makefile and Device Tree sources, which may not align with the current state of the kernel or the target hardware configuration.

Once these issues are addressed, the kernel, device tree blobs, and modules can be compiled using an ARM cross-compiler. Finally, the kernel image is converted into a U-Boot compatible format using `mkimage`, producing a `uImage` that can be loaded by the bootloader during system startup.

## Checking Linux Patch directory

### 1) Kernel configuration file (.config)

The Linux kernel build system uses a configuration file named `.config` located at the root of the kernel source tree. This file defines all the **options used during the compilation process**, including enabled drivers, supported subsystems, architecture settings, and hardware-specific features.

Each configuration option follows the format:

```
CONFIG_OPTION=value
```

These options determine which components are compiled directly into the kernel, compiled as loadable modules, or excluded from the build. This ensures that the kernel is compiled with the correct configuration required for the target hardware platform (Allwinner T113-S3). Using a **predefined configuration avoids the need to manually** enable the required drivers and features through the `menuconfig` interface.

Once the `.config` file is present, the Linux build system automatically uses it to determine which components must be compiled when executing the `make` command.

During the kernel build process, the compilation may fail in the `drivers/base/firmware_loader` stage.   This happens because the configuration file includes a reference to an **external firmware file that is not present in the build environment**.  
  
Originally, the configuration contained the following line:

```
CONFIG_EXTRA_FIRMWARE="rtlwifi/rtl8723bu_nic.bin"
CONFIG_EXTRA_FIRMWARE_DIR="\lib/firmware"
```


Originally, the configuration expected the firmware to be present under `/lib/firmware`, which caused the build process to fail since this directory does not exist in the build environment.

To resolve this issue, instead of removing the firmware reference, the required directory is explicitly created within the `build_kernel.sh` script, ensuring the firmware path is valid during compilation. This allows the kernel build system to correctly locate and embed the specified firmware.

Additionally, a small but important correction was made to the firmware directory path:

```
CONFIG_EXTRA_FIRMWARE="rtlwifi/rtl8723bu_nic.bin "
CONFIG_EXTRA_FIRMWARE_DIR="lib/firmware"
```

The leading backslash (`\`) was removed because it incorrectly defines an absolute path (`/lib/firmware`). In the context of the kernel build system, this causes it to look outside the build tree, leading to a missing path error.

By changing it to a relative path (`lib/firmware`), the firmware directory is correctly resolved within the build environment.

---
### 2) sunxi-d1s-t113s-saxo.dtsi

The same modifications performed in the U-Boot configuration must be applied here as well. In general, this consists of enabling **UART0** and disabling **UART3** in order to match the hardware configuration of the board.

First, the following line in the original script is modified in order to change the serial port that will be used as the CPU's RX–TX interface:

```
chosen {
		stdout-path = "serial3:115200n8";
	};

/* --> To --> */

chosen {
		stdout-path = "serial0:115200n8";
	};
```

An alternative way to locate the pins assigned to `UART0` is to define them within the `&pio` block. This block represents the **Pin Controller (PIO)** of the SoC and is responsible for configuring the multiplexing and electrical behavior of the physical pins.

Specifically, within `&pio` you can:
- Assign pins to specific peripheral functions (e.g., UART, SPI, I2C)
- Configure pin modes (input/output/alternate function)
- Set electrical properties such as pull-up, pull-down, and drive strength

In this context, adding the UART0 pins to the `&pio` block ensures that the selected pins are correctly configured to operate as the UART0 interface.

```
&pio {
	vcc-pb-supply = <&reg_3v3>;
	vcc-pd-supply = <&reg_3v3>;
	vcc-pe-supply = <&reg_avdd2v8>;
	vcc-pf-supply = <&reg_3v3>;
	vcc-pg-supply = <&reg_3v3>;
};

/* --> To --> */

&pio {
	vcc-pb-supply = <&reg_3v3>;
	vcc-pd-supply = <&reg_3v3>;
	vcc-pe-supply = <&reg_avdd2v8>;
	vcc-pf-supply = <&reg_3v3>;
	vcc-pg-supply = <&reg_3v3>;

	uart0_pe_pins: uart0-pe-pins {
		pins = "PE2", "PE3";
		function = "uart0";
	};
};
```

Additionally, it is necessary to explicitly define the `uart0` node in order to  **enable and configure this peripheral** within the system. Declaring this block ensures that the kernel properly initializes the UART0 interface, applies the corresponding pin configuration, and makes it available as a functional serial device:

```
&uart0 {
    pinctrl-names = "default";
    pinctrl-0 = <&uart0_pe_pins>;
    status = "okay";
};
```

Furthermore, any UART interfaces that are not currently in use **should be disabled**. This helps prevent resource conflicts, unintended pin usage, or driver initialization issues. To minimize potential errors, the unused UART nodes should also be simplified by **removing unnecessary properties** and leaving only the `status = "disabled";` field. This keeps the Device Tree clean and avoids misconfigurations during the boot process:

```
&uart1 {        
        pinctrl-names = "default";
        pinctrl-0 = <&uart1_pg6_pins>;
        status = "okay";
};

&uart3 {        
        pinctrl-names = "default";
        pinctrl-0 = <&uart3_pb_pins>;
        status = "okay";
};

&uart4 {        
        pinctrl-names = "default";
        pinctrl-0 = <&uart4_pg_pins>;
        status = "okay";
};

&uart5 {        
        pinctrl-names = "default";
        pinctrl-0 = <&uart5_pg_pins>;
        status = "okay";
};
```

Replaced by:

```
&uart1 { status = "disabled"; };
&uart3 { status = "disabled"; };
&uart4 { status = "disabled"; };
&uart5 { status = "disabled"; };
```

## Update Makefile

The Device Tree build system was updated to include the custom board definition in the kernel compilation process. Specifically, the file `linux/arch/arm/boot/dts/allwinner/Makefile` was modified by adding the following line:

```
dtb-$(CONFIG_ARCH_SUNXI) += sun8i-t113s-saxo-gateway.dtb
```

This change **ensures that the custom Device Tree** source file (`sun8i-t113s-saxo-gateway.dts`) is compiled into a corresponding `.dtb` file during the kernel build. Without this modification, the `.dts` file would not be processed, even if it exists in the directory, and no `.dtb` would be generated for the target board.


## Firmware directory and WIFI firmware module

### Why to do it

To properly enable the WiFi module based on the **RTL8723BU** chipset, it is necessary to include the corresponding firmware in the kernel tree. This firmware is not part of the mainline kernel source, so it must be added manually.

The firmware file (`rtl8723bu_nic.bin`) was previously downloaded and stored inside the project’s patch directory ``linux-patch-6.16.9/``.

This approach keeps all external resources (patches, firmware, etc.) centralized and version-controlled within the project.

During the kernel build process, the firmware must be copied into the standard directory where the system expects to find it on ``linux/lib/firmware/rtlwifi/``.

### Integration in the build script

Inside the `build_kernel.sh` script, the following commands are used to guarantee that the firmware is always present during the build:

```bash
# WiFi firmware for RTL8723BU
mkdir -p linux/lib/firmware/rtlwifi
cp linux-patch-6.16.9/rtl8723bu_nic.bin linux/lib/firmware/rtlwifi/
```

## Update build_kernel.sh

### Original script check

The original script is:

```bash

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

Before detailing the specific changes introduced in the final version of the script, it is important to highlight several issues present in the original implementation that could negatively affect the build process:

- **Lack of error handling:** 
The script did not enforce termination on failure. As a result, if **any command failed**, the execution would continue, potentially producing incomplete or invalid build artifacts.

- **Improper repository state management:**  
There was no guarantee that the Linux source tree started from a clean state. Residual files from previous builds or manual modifications could interfere with the current compilation.

- **Incorrect ordering of operations:**  
The script **copied modified files** into the source tree **before resetting the repository state**. This caused those **changes to be unintentionally discarded**, leading to inconsistent or misleading build results.

- **Redundant or conflicting patch application:**  
The script attempted to **apply a patch** regardless of whether its changes were already present. This could lead to **patch conflicts or errors** such as reversed/previously applied patch detection.

- **Use of interactive configuration in an automated script:**  
The inclusion of `menuconfig` made the process non-reproducible and unsuitable for automated builds.

- **Hardcoded build parameters:**  
Fixed values such as the number of parallel jobs (`-j4`) reduced portability and efficiency across different systems.

These issues collectively made the original script fragile, non-deterministic, and difficult to debug. The subsequent modifications aim to systematically address each of these weaknesses.

### Updated Script

The updated script introduces several corrections that address the main weaknesses of the original implementation, improving robustness, correctness, and reproducibility:

```bash
#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

# --- Verificar toolchain ---
if ! command -v arm-none-linux-gnueabihf-gcc >/dev/null 2>&1; then
    echo "ERROR: arm-none-linux-gnueabihf-gcc no encontrado en PATH"
    exit 1
fi

export CROSS_COMPILE=arm-none-linux-gnueabihf-

# --- Limpiar repo ---
cd linux
git reset --hard
git clean -fd

cd ..
# -- Copia el config-----
cp linux-patch-6.16.9/config linux/.config

## Firmware
mkdir -p linux/lib/firmware/rtlwifi
cp linux-patch-6.16.9/rtl8723bu_nic.bin linux/lib/firmware/rtlwifi/

cd "$SCRIPT_DIR"

# --- Copiar device tree y config ---
cp linux-patch-6.16.9/sun8i-t113s-saxo-gateway.dts \
   linux/arch/arm/boot/dts/allwinner

cp linux-patch-6.16.9/sunxi-d1s-t113s-saxo.dtsi \
   linux/arch/arm/boot/dts/allwinner

# ELIMINADO: archivo incorrecto para ARM
# cp linux-patch-6.16.9/sunxi-d1s-t113.dtsi ...


cd linux

# --- Aplicar patch del DTB ---
patch -N -d . -p1 < ../linux-patch-6.16.9/0001-saxo-dtb-reference.patch

# --- Build ---
make ARCH=arm olddefconfig
make ARCH=arm zImage dtbs modules -j10

cd "$SCRIPT_DIR"

# --- Crear uImage ---
LOAD_ADDR=0x41800000
ENTRY_ADDR=0x41800000

mkimage -A arm -O linux -T kernel -C none \
  -a $LOAD_ADDR -e $ENTRY_ADDR \
  -n "SAXO Linux Kernel (T113-S3)" \
  -d ./linux/arch/arm/boot/zImage uImage

echo "SAXO Linux Kernel (T113-S3): uImage generado"
```

- **Strict error handling (`set -euo pipefail`):**  
The script now **stops immediately on any error**, on the use of undefined variables, or on failures within pipelines. This prevents silent failures and ensures that invalid builds are not produced.

- **Toolchain validation and standardization:**  
A check was added to verify that `arm-none-linux-gnueabihf-gcc` is **available in the system**. In this case, it was necessary to **manually download and install the ARM cross-compilation toolchain from the official Arm website** , ensuring compatibility with the target architecture. 

The `CROSS_COMPILE` variable is then explicitly defined to use this hard-float toolchain, avoiding inconsistencies and guaranteeing that the generated binaries match the expected ABI of the platform.

- **Proper repository cleanup before modifications:**  
The commands `git reset --hard` and `git clean -fd` are **executed before** copying any files. This guarantees that the Linux source tree is always in a clean and known state, eliminating interference from previous builds.

- **Correct ordering of operations:**  
File **copying** is now performed only **after the repository has been cleaned**. This fixes the previous issue where changes were unintentionally discarded.

- **Removal of incorrect architecture-specific file:**  
 The file `sunxi-d1s-t113.dtsi` is no longer copied, as it belongs to a different architecture (RISC-V) and is not required for the ARM-based T113. This avoids invalid dependencies and potential build errors.

- **Safe patch application:**  
The patch is applied using the `-N` flag, which **prevents reapplying an already applied patch**. This avoids conflicts and makes the process more robust across repeated executions.

- **Non-interactive and reproducible configuration:**  
`menuconfig` was replaced with `olddefconfig`, ensuring that the build process is fully automated and reproducible.

- **Improved build parallelism:**  
The number of parallel jobs was increased (`-j10`), better utilizing available system resources and reducing build time.

- **Consistent kernel image generation:**  
The final `mkimage` step remains, but now operates on a reliably built kernel, ensuring that the generated `uImage` is valid.

Overall, these changes transform the script into a deterministic and repeatable build pipeline, eliminating hidden state dependencies, architecture mismatches, and execution-order issues present in the original version.

## Kernel Results

The kernel build script is then executed to verify that the compilation process completes successfully. This step allows checking that the applied modifications, patches, and configuration files are consistent and do not introduce compilation errors:

![](Images/F1%201.png)

To verify that the build process completes without errors and to keep a record of the output, the following command can be used to generate a log file:

```bash
./build_kernel.sh 2>&1 | tee build.log
```

This command executes the build script and simultaneously displays the output in the terminal while saving it to the file `build.log`. It captures both standard output and error messages, making it useful for debugging and documentation purposes.

The build process generates the **Linux kernel image, Device Tree binaries (DTBs), and kernel modules** for the target platform.

The **kernel** is compiled into a compressed `zImage`, which is the binary loaded by the bootloader during startup. The **Device Tree source files** (`.dts` and `.dtsi`) are compiled into `.dtb` files, which describe the hardware configuration of the board, enabling the kernel to properly initialize peripherals and system components.

Additionally, the process builds loadable kernel modules that provide optional functionality and can be dynamically loaded after boot.

Finally, the `zImage` is wrapped using `mkimage` to produce a `uImage`, which includes the necessary metadata (such as load address and entry point) required by U-Boot to correctly boot the kernel.

