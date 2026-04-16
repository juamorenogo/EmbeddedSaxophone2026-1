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

# ❌ ELIMINADO: archivo incorrecto para ARM
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