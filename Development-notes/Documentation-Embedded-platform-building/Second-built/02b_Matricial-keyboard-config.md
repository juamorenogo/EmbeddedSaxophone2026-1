
## List of files to change
### Matrix-keypad-driver-enablement (.config)

The Linux kernel already includes integrated support for matrix keyboards through a dedicated driver and associated header files. However, this functionality is typically disabled by default in many embedded kernel configurations. Therefore, it is necessary to explicitly enable the matrix keypad driver before compiling the kernel.

This can be done either through the `menuconfig` interface or directly from the kernel `config` file on the patch folder by searching for the following options and enabling them:

```
CONFIG_INPUT_KEYBOARD=y
CONFIG_KEYBOARD_ATKBD=y
CONFIG_KEYBOARD_MATRIX=y
CONFIG_INPUT_MATRIXKMAP=y
```

### Keypad-pinout (sunxi-d1s-t113s-saxo.dtsi) 

First of all, it is necessary to import the `MATRIX_KEY()` to the system. In this case, it is located on the `dt-bindings/input/input.h` so it must be imported adding the following line:

```
#include <dt-bindings/input/input.h>
```

Additionally, it is necessary to define a `keypad` block inside the body of the `.dtsi` file. This block describes the complete matrix keypad configuration used by the `gpio-matrix-keypad` Linux driver.

The node must contain:

- GPIO definitions for rows and columns
- scan and debounce timing parameters
- pinctrl configuration
- the key translation table (`linux,keymap`)

The complete structure is shown below:

```
keypad {
    compatible = "gpio-matrix-keypad";
    status = "okay";

    pinctrl-names = "default";
    pinctrl-0 = <&keypad_pins>;

    col-scan-delay-us = <500>;
    debounce-delay-ms = <100>;

    wakeup-source;
    linux,no-autorepeat;

    row-gpios = <
        &pio 3 16 GPIO_ACTIVE_LOW
        &pio 3 14 GPIO_ACTIVE_LOW
        &pio 3 13 GPIO_ACTIVE_LOW
        &pio 3 10 GPIO_ACTIVE_LOW
        &pio 3 21 GPIO_ACTIVE_LOW
        &pio 3 6  GPIO_ACTIVE_LOW
        &pio 3 4  GPIO_ACTIVE_LOW
        &pio 3 2  GPIO_ACTIVE_LOW
    >;

    col-gpios = <
        &pio 3 8  GPIO_ACTIVE_HIGH
        &pio 3 1  GPIO_ACTIVE_HIGH
        &pio 3 7  GPIO_ACTIVE_HIGH
    >;

    linux,keymap = <
        MATRIX_KEY(0, 0, KEY_Z)
        MATRIX_KEY(1, 0, KEY_X)
        MATRIX_KEY(2, 0, KEY_C)
        MATRIX_KEY(3, 0, KEY_V)
        MATRIX_KEY(4, 0, KEY_B)
        MATRIX_KEY(5, 0, KEY_N)
        MATRIX_KEY(6, 0, KEY_M)
        MATRIX_KEY(7, 0, KEY_Q)

        MATRIX_KEY(0, 1, KEY_W)
        MATRIX_KEY(1, 1, KEY_E)
        MATRIX_KEY(2, 1, KEY_R)
        MATRIX_KEY(3, 1, KEY_T)
        MATRIX_KEY(4, 1, KEY_Y)
        MATRIX_KEY(5, 1, KEY_U)
        MATRIX_KEY(6, 1, KEY_I)
        MATRIX_KEY(7, 1, KEY_O)

        MATRIX_KEY(0, 2, KEY_A)
        MATRIX_KEY(1, 2, KEY_S)
        MATRIX_KEY(2, 2, KEY_D)
        MATRIX_KEY(3, 2, KEY_F)
        MATRIX_KEY(4, 2, KEY_G)
        MATRIX_KEY(5, 2, KEY_H)
        MATRIX_KEY(6, 2, KEY_J)
        MATRIX_KEY(7, 2, KEY_K)
    >;
};
```

The purpose of each property is the following:

| Property | Description |
|----------|-------------|
| `compatible` | Specifies the Linux driver associated with the node |
| `status` | Enables the device |
| `pinctrl-names` | Declares the active pinctrl configuration |
| `pinctrl-0` | References the GPIO pin configuration block |
| `col-scan-delay-us` | Delay between column activations during matrix scanning |
| `debounce-delay-ms` | Debounce filtering time to avoid false presses |
| `wakeup-source` | Allows the keypad to wake the system from low-power states |
| `linux,no-autorepeat` | Disables automatic key repetition while holding a key |
| `row-gpios` | Defines the GPIOs connected to keypad rows |
| `col-gpios` | Defines the GPIOs connected to keypad columns |
| `linux,keymap` | Defines the translation table between matrix coordinates and Linux keycodes |

The `row-gpios` and `col-gpios` properties describe the physical matrix wiring. During operation, the driver sequentially activates each column and reads the row states to detect pressed keys.

The `linux,keymap` property defines how each `(row, column)` coordinate is translated into a Linux input keycode through the `MATRIX_KEY()` macro.

For example `MATRIX_KEY(0, 0, KEY_Z)` maps the key located at:

- Row 0
- Column 0

to the Linux input event:

- `KEY_Z`

Internally, the macro expands into a 32-bit hexadecimal value encoding, (row, column, keycode), which is later parsed by the `gpio-matrix-keypad` driver during runtime initialization.

