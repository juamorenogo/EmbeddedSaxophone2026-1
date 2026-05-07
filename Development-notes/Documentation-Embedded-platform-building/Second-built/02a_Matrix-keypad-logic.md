## Matrix Translating Function

### General understanding of Matrix keypads

In Linux embedded systems, matrix keypads are usually described through the Device Tree (DT). The `gpio-matrix-keypad` driver scans the keypad matrix and translates each `(row, column)` coordinate into a Linux key event.

A matrix keypad reduces GPIO usage by arranging keys into rows and columns. For example:

|      | COL0 | COL1 | COL2 |
|------|------|------|------|
| ROW0 | Z | W | A |
| ROW1 | X | E | S |
| ROW2 | C | R | D |

Instead of requiring one GPIO per key, the system only requires:

- N row GPIOs
- M column GPIOs

The driver scans the matrix sequentially:

1. Activate one column
2. Read all rows
3. Detect pressed keys
4. Translate `(row, column)` into a Linux keycode

The Device Tree representation typically looks like:

- `row-gpios`: GPIOs connected to rows
- `col-gpios`: GPIOs connected to columns
- `linux,keymap`: translation table

The `linux,keymap` property maps:

	(row, column) → Linux keycode


### Linux-keymap

The hexadecimal keymap format is ``0xRRCCKKKK``. Where:

| Field | Meaning       |
| ----- | ------------- |
| RR    | Row index     |
| CC    | Column index  |
| KKKK  | Linux keycode |

Example:

| Hex Value  | Meaning            |
| ---------- | ------------------ |
| 0x0000002c | ROW0, COL0 → KEY_Z |
| 0x01010012 | ROW1, COL1 → KEY_E |

Inside the Device Tree Source (DTS) file, the `linux,keymap` property defines the association between the matrix keypad scan positions and the corresponding Linux input key codes. An example configuration is shown below:

```
keypad {
    linux,keymap = <
        0x0000002c
        0x0100002d
    >;
};
```


Mainline Linux kernels often use the helper macro instead of raw hexadecimal values:

	MATRIX_KEY(row, col, keycode)

### Matrix-translation-function

Therefore, `MATRIX_KEY()` is only syntactic sugar. The driver ultimately receives the same 32-bit integer representation. The macro is usually defined in ``include/dt-bindings/input/matrix-keymap.h``.  

However, many vendor BSP kernels do not include this binding or do not expose it correctly to `dtc`. In those cases, using `MATRIX_KEY()` produces syntax errors during Device Tree compilation. Using raw hexadecimal values avoids:

- missing headers
- preprocessor issues
- dtc include path problems

For this reason, hexadecimal encoding is often the most robust solution for embedded BSP kernels. In Allwinner kernels, GPIOs are usually represented as ``<&pio bank pin flags>``. Example:

```
&pio 3 16 GPIO_ACTIVE_LOW
```

Where the value `3` corresponds to GPIO bank D, the value `16` specifies pin number 16 within that GPIO bank, and `GPIO_ACTIVE_LOW` indicates that the signal operates using active-low logic. GPIO bank mapping:

| GPIO Bank | Numeric Value |
|-----------|----------------|
| PA | 0 |
| PB | 1 |
| PC | 2 |
| PD | 3 |
| PE | 4 |
| PF | 5 |
| PG | 6 |

The matrix keypad system has two main layers:

1. Driver layer
   - `drivers/input/keyboard/matrix_keypad.c`
   - Handles scanning, debouncing, interrupts, and Linux input events

2. Device Tree layer
   - Defines GPIOs, matrix dimensions, and key mappings
