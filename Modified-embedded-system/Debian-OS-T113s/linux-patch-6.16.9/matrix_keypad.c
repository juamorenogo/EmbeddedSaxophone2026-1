// SPDX-License-Identifier: GPL-2.0-only
/*
 *  GPIO driven matrix keyboard driver
 *
 *  Copyright (c) 2008 Marek Vasut <marek.vasut@gmail.com>
 *
 *  Based on corgikbd.c
 */

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/input/matrix_keypad.h>
#include <linux/slab.h>
#include <linux/of.h>

// Añadido para pull-down
//#include <linux/gpio/consumer.h>  //Ya estaba antes
#include <linux/pinctrl/pinconf-generic.h>

struct matrix_keypad {
	struct input_dev *input_dev;
	unsigned int row_shift;

	unsigned int col_scan_delay_us;
	unsigned int all_cols_on_delay_us;
	/* key debounce interval in milli-second */
	unsigned int debounce_ms;
	bool drive_inactive_cols;

	struct gpio_desc *row_gpios[MATRIX_MAX_ROWS];
	unsigned int num_row_gpios;

	struct gpio_desc *col_gpios[MATRIX_MAX_ROWS];
	unsigned int num_col_gpios;

	unsigned int row_irqs[MATRIX_MAX_ROWS];
	DECLARE_BITMAP(wakeup_enabled_irqs, MATRIX_MAX_ROWS);

	uint32_t last_key_state[MATRIX_MAX_COLS];
	struct delayed_work work;
	spinlock_t lock;
	bool scan_pending;
	bool stopped;
};

/*
 * NOTE: If drive_inactive_cols is false, then the GPIO has to be put into
 * HiZ when de-activated to cause minmal side effect when scanning other
 * columns. In that case it is configured here to be input, otherwise it is
 * driven with the inactive value.
 */

 // Funcion cambiada
static void __activate_col(struct matrix_keypad *keypad, int col, bool on)
{
	if (on) {
		gpiod_direction_output(keypad->col_gpios[col], 1);

		pr_info("COL%d OUT=%d\n",
				col,
				gpiod_get_value_cansleep(keypad->col_gpios[col]));

	} else {
		gpiod_direction_output(keypad->col_gpios[col], 0);

		pr_info("COL%d OUT=%d\n",
				col,
				gpiod_get_value_cansleep(keypad->col_gpios[col]));
	}
}

// Funcion cambiada
static void activate_col(struct matrix_keypad *keypad, int col, bool on)
{
    pr_info("COL%d -> %s\n", col, on ? "ON" : "OFF");

    __activate_col(keypad, col, on);

    if (on && keypad->col_scan_delay_us)
        fsleep(keypad->col_scan_delay_us);
}

static void activate_all_cols(struct matrix_keypad *keypad, bool on)
{
	int col;

	for (col = 0; col < keypad->num_col_gpios; col++)
		__activate_col(keypad, col, on);

	if (on && keypad->all_cols_on_delay_us)
		fsleep(keypad->all_cols_on_delay_us);
}

// Funcion cambiada

static bool row_asserted(struct matrix_keypad *keypad, int row)
{
    int v = gpiod_get_value_cansleep(keypad->row_gpios[row]);

    pr_info("ROW%d = %d\n", row, v);

    return v;
}

static void enable_row_irqs(struct matrix_keypad *keypad)
{
	int i;

	for (i = 0; i < keypad->num_row_gpios; i++) {
		pr_info("ENABLE IRQ %d\n", keypad->row_irqs[i]);
		enable_irq(keypad->row_irqs[i]);
	}
}

static void disable_row_irqs(struct matrix_keypad *keypad)
{
	int i;

	for (i = 0; i < keypad->num_row_gpios; i++) {
		pr_info("DISABLE IRQ %d\n", keypad->row_irqs[i]);
		disable_irq_nosync(keypad->row_irqs[i]);
	}
}

// Funcion modificada
static uint32_t read_row_state(struct matrix_keypad *keypad)
{
    int row;
    u32 row_state = 0;

    for (row = 0; row < keypad->num_row_gpios; row++) {

        bool asserted = row_asserted(keypad, row);

        pr_info("READ ROW%d=%d\n", row, asserted);

        if (asserted)
            row_state |= BIT(row);
    }

    return row_state;
}

/*
 * This gets the keys from keyboard and reports it to input subsystem
 */
static void matrix_keypad_scan(struct work_struct *work)
{
	struct matrix_keypad *keypad =
		container_of(work, struct matrix_keypad, work.work);

	pr_info("MATRIX SCAN RUN\n");
	struct input_dev *input_dev = keypad->input_dev;
	const unsigned short *keycodes = input_dev->keycode;
	uint32_t new_state[MATRIX_MAX_COLS];
	int row, col, code;
	u32 init_row_state, new_row_state;

	/* read initial row state to detect changes between scan */
	init_row_state = read_row_state(keypad);

	/* de-activate all columns for scanning */
	activate_all_cols(keypad, false);

	memset(new_state, 0, sizeof(new_state));

	for (row = 0; row < keypad->num_row_gpios; row++)
		gpiod_direction_input(keypad->row_gpios[row]);

	/* assert each column and read the row status out */
	for (col = 0; col < keypad->num_col_gpios; col++) {

		activate_col(keypad, col, true);

		new_state[col] = read_row_state(keypad);

		pr_info("MATRIX COL=%d STATE=0x%08x\n",
				col,
				new_state[col]);

		activate_col(keypad, col, false);
	}

	for (col = 0; col < keypad->num_col_gpios; col++) {
		uint32_t bits_changed;

		bits_changed = keypad->last_key_state[col] ^ new_state[col];
		if (bits_changed == 0)
			continue;

		for (row = 0; row < keypad->num_row_gpios; row++) {
			if (!(bits_changed & BIT(row)))
				continue;

			code = MATRIX_SCAN_CODE(row, col, keypad->row_shift);
			input_event(input_dev, EV_MSC, MSC_SCAN, code);

			pr_info("KEY row=%d col=%d pressed=%d code=%d\n",
			row, col,
			!!(new_state[col] & BIT(row)),
			keycodes[code]);
			input_report_key(input_dev,
					 keycodes[code],
					 new_state[col] & (1 << row));
		}
	}
	input_sync(input_dev);

	memcpy(keypad->last_key_state, new_state, sizeof(new_state));

	activate_all_cols(keypad, true);

	/* Enable IRQs again */
	scoped_guard(spinlock_irq, &keypad->lock) {
		keypad->scan_pending = false;
		enable_row_irqs(keypad);
	}

	/* read new row state and detect if value has changed */
	new_row_state = read_row_state(keypad);
	if (init_row_state != new_row_state) {
		guard(spinlock_irq)(&keypad->lock);
		if (unlikely(keypad->scan_pending || keypad->stopped))
			return;
		disable_row_irqs(keypad);
		keypad->scan_pending = true;
		schedule_delayed_work(&keypad->work,
				      msecs_to_jiffies(keypad->debounce_ms));
	}
}

// Funcion modificada
static irqreturn_t matrix_keypad_interrupt(int irq, void *id)
{
	struct matrix_keypad *keypad = id;

	pr_info("MATRIX IRQ ENTER irq=%d\n", irq);

	guard(spinlock_irqsave)(&keypad->lock);

	pr_info("IRQ ENTER irq=%d pending=%d stopped=%d\n",
        irq,
        keypad->scan_pending,
        keypad->stopped);

	if (unlikely(keypad->scan_pending || keypad->stopped)) {
		pr_info("IRQ IGNORED pending=%d stopped=%d\n",
				keypad->scan_pending,
				keypad->stopped);
		goto out;
	}

	pr_info("IRQ SCHEDULE\n");

	disable_row_irqs(keypad);
	keypad->scan_pending = true;

	schedule_delayed_work(&keypad->work,
						msecs_to_jiffies(keypad->debounce_ms));

out:
	return IRQ_HANDLED;
}

static int matrix_keypad_start(struct input_dev *dev)
{
	struct matrix_keypad *keypad = input_get_drvdata(dev);

	keypad->stopped = false;
	mb();

	/*
	 * Schedule an immediate key scan to capture current key state;
	 * columns will be activated and IRQs be enabled after the scan.
	 */
	schedule_delayed_work(&keypad->work, 0);

	return 0;
}

static void matrix_keypad_stop(struct input_dev *dev)
{
	struct matrix_keypad *keypad = input_get_drvdata(dev);

	scoped_guard(spinlock_irq, &keypad->lock) {
		keypad->stopped = true;
	}

	flush_delayed_work(&keypad->work);
	/*
	 * matrix_keypad_scan() will leave IRQs enabled;
	 * we should disable them now.
	 */
	disable_row_irqs(keypad);
}

static void matrix_keypad_enable_wakeup(struct matrix_keypad *keypad)
{
	int i;

	for_each_clear_bit(i, keypad->wakeup_enabled_irqs,
			   keypad->num_row_gpios)
		if (enable_irq_wake(keypad->row_irqs[i]) == 0)
			__set_bit(i, keypad->wakeup_enabled_irqs);
}

static void matrix_keypad_disable_wakeup(struct matrix_keypad *keypad)
{
	int i;

	for_each_set_bit(i, keypad->wakeup_enabled_irqs,
			 keypad->num_row_gpios) {
		disable_irq_wake(keypad->row_irqs[i]);
		__clear_bit(i, keypad->wakeup_enabled_irqs);
	}
}

static int matrix_keypad_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct matrix_keypad *keypad = platform_get_drvdata(pdev);

	matrix_keypad_stop(keypad->input_dev);

	if (device_may_wakeup(&pdev->dev))
		matrix_keypad_enable_wakeup(keypad);

	return 0;
}

static int matrix_keypad_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct matrix_keypad *keypad = platform_get_drvdata(pdev);

	if (device_may_wakeup(&pdev->dev))
		matrix_keypad_disable_wakeup(keypad);

	matrix_keypad_start(keypad->input_dev);

	return 0;
}

static DEFINE_SIMPLE_DEV_PM_OPS(matrix_keypad_pm_ops,
				matrix_keypad_suspend, matrix_keypad_resume);

static int matrix_keypad_init_gpio(struct platform_device *pdev,
				   struct matrix_keypad *keypad)
{
	bool active_low;
	int nrow, ncol;
	int err;
	int i;

	nrow = gpiod_count(&pdev->dev, "row");
	ncol = gpiod_count(&pdev->dev, "col");
	if (nrow < 0 || ncol < 0) {
		dev_err(&pdev->dev, "missing row or column GPIOs\n");
		return -EINVAL;
	}

	keypad->num_row_gpios = nrow;
	keypad->num_col_gpios = ncol;

	active_low = device_property_read_bool(&pdev->dev, "gpio-activelow");

	/* initialize strobe lines as outputs, activated */
	for (i = 0; i < keypad->num_col_gpios; i++) {
		keypad->col_gpios[i] = devm_gpiod_get_index(&pdev->dev, "col",
							    i, GPIOD_ASIS);
		err = PTR_ERR_OR_ZERO(keypad->col_gpios[i]);
		if (err) {
			dev_err(&pdev->dev,
				"failed to request GPIO for COL%d: %d\n",
				i, err);
			return err;
		}

		// Añadido para pulls

		err = gpiod_set_config(
        keypad->col_gpios[i],
        pinconf_to_config_packed(
                PIN_CONFIG_BIAS_PULL_DOWN,
                1));

		pr_info("COL%d pull ret=%d\n", i, err);

		pr_info("COL%d level=%d\n",
        i,
        gpiod_get_value_cansleep(keypad->col_gpios[i]));


		gpiod_set_consumer_name(keypad->col_gpios[i], "matrix_kbd_col");

		if (active_low ^ gpiod_is_active_low(keypad->col_gpios[i]))
			gpiod_toggle_active_low(keypad->col_gpios[i]);

		gpiod_direction_output(keypad->col_gpios[i], 1);
	}

	for (i = 0; i < keypad->num_row_gpios; i++) {
		keypad->row_gpios[i] = devm_gpiod_get_index(&pdev->dev, "row",
							    i, GPIOD_IN);
		err = PTR_ERR_OR_ZERO(keypad->row_gpios[i]);


		if (err) {
			dev_err(&pdev->dev,
				"failed to request GPIO for ROW%d: %d\n",
				i, err);
			return err;
		}


		// Añadido para pulls

		err = gpiod_set_config(
        keypad->row_gpios[i],
        pinconf_to_config_packed(
                PIN_CONFIG_BIAS_PULL_DOWN,
                1));

		pr_info("ROW%d pull ret=%d\n", i, err);

		pr_info("ROW%d level=%d\n",
        i,
        gpiod_get_value_cansleep(keypad->row_gpios[i]));


		gpiod_set_consumer_name(keypad->row_gpios[i], "matrix_kbd_row");

		if (active_low ^ gpiod_is_active_low(keypad->row_gpios[i]))
			gpiod_toggle_active_low(keypad->row_gpios[i]);
	}

	return 0;
}

// Cambiada para imprimir
static int matrix_keypad_setup_interrupts(struct platform_device *pdev,
					  struct matrix_keypad *keypad)
{
	int err;
	int irq;
	int i;

	for (i = 0; i < keypad->num_row_gpios; i++) {

		irq = gpiod_to_irq(keypad->row_gpios[i]);

		pr_info("ROW%d gpio->irq=%d\n", i, irq);

		if (irq < 0) {
			err = irq;
			dev_err(&pdev->dev,
				"Unable to convert GPIO line %i to irq: %d\n",
				i, err);
			return err;
		}

		err = devm_request_any_context_irq(
				&pdev->dev,
				irq,
				matrix_keypad_interrupt,
				IRQF_TRIGGER_RISING |
				IRQF_TRIGGER_FALLING,
				"matrix-keypad",
				keypad);

		pr_info("ROW%d request_irq ret=%d\n", i, err);

		if (err < 0) {
			dev_err(&pdev->dev,
				"Unable to acquire interrupt for row %i: %d\n",
				i, err);
			return err;
		}

		err = irq_set_irq_type(
				irq,
				IRQ_TYPE_EDGE_FALLING);

		pr_info("ROW%d irq_set_irq_type ret=%d\n", i, err);

		/* NUEVO */
		pr_info("ROW%d irq_get_trigger_type=0x%x\n",
			i,
			irq_get_trigger_type(irq));

		keypad->row_irqs[i] = irq;

		if (err)
			dev_warn(&pdev->dev,
				 "ROW%d irq_set_irq_type failed (%d)\n",
				 i, err);
	}

	disable_row_irqs(keypad);

	return 0;
}
static int matrix_keypad_probe(struct platform_device *pdev)
{
	struct matrix_keypad *keypad;
	struct input_dev *input_dev;
	bool wakeup;
	int err;

	keypad = devm_kzalloc(&pdev->dev, sizeof(*keypad), GFP_KERNEL);
	if (!keypad)
		return -ENOMEM;

	input_dev = devm_input_allocate_device(&pdev->dev);
	if (!input_dev)
		return -ENOMEM;

	keypad->input_dev = input_dev;
	keypad->stopped = true;
	INIT_DELAYED_WORK(&keypad->work, matrix_keypad_scan);
	spin_lock_init(&keypad->lock);

	keypad->drive_inactive_cols =
		device_property_read_bool(&pdev->dev, "drive-inactive-cols");
	device_property_read_u32(&pdev->dev, "debounce-delay-ms",
				 &keypad->debounce_ms);
	device_property_read_u32(&pdev->dev, "col-scan-delay-us",
				 &keypad->col_scan_delay_us);
	device_property_read_u32(&pdev->dev, "all-cols-on-delay-us",
				 &keypad->all_cols_on_delay_us);

	err = matrix_keypad_init_gpio(pdev, keypad);
	if (err)
		return err;

	keypad->row_shift = get_count_order(keypad->num_col_gpios);

	err = matrix_keypad_setup_interrupts(pdev, keypad);
	if (err)
		return err;

	input_dev->name		= pdev->name;
	input_dev->id.bustype	= BUS_HOST;
	input_dev->open		= matrix_keypad_start;
	input_dev->close	= matrix_keypad_stop;

	err = matrix_keypad_build_keymap(NULL, NULL,
					 keypad->num_row_gpios,
					 keypad->num_col_gpios,
					 NULL, input_dev);
	if (err) {
		dev_err(&pdev->dev, "failed to build keymap\n");
		return -ENOMEM;
	}

	if (!device_property_read_bool(&pdev->dev, "linux,no-autorepeat"))
		__set_bit(EV_REP, input_dev->evbit);

	input_set_capability(input_dev, EV_MSC, MSC_SCAN);
	input_set_drvdata(input_dev, keypad);

	err = input_register_device(keypad->input_dev);
	if (err)
		return err;

	wakeup = device_property_read_bool(&pdev->dev, "wakeup-source") ||
		 /* legacy */
		 device_property_read_bool(&pdev->dev, "linux,wakeup");
	device_init_wakeup(&pdev->dev, wakeup);

	platform_set_drvdata(pdev, keypad);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id matrix_keypad_dt_match[] = {
	{ .compatible = "gpio-matrix-keypad" },
	{ }
};
MODULE_DEVICE_TABLE(of, matrix_keypad_dt_match);
#endif

static struct platform_driver matrix_keypad_driver = {
	.probe		= matrix_keypad_probe,
	.driver		= {
		.name	= "matrix-keypad",
		.pm	= pm_sleep_ptr(&matrix_keypad_pm_ops),
		.of_match_table = of_match_ptr(matrix_keypad_dt_match),
	},
};
module_platform_driver(matrix_keypad_driver);

MODULE_AUTHOR("Marek Vasut <marek.vasut@gmail.com>");
MODULE_DESCRIPTION("GPIO Driven Matrix Keypad Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:matrix-keypad");
