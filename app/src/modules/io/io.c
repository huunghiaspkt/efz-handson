/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LED1 blinks, LED2 follows BN1.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>

#include "lcd/lcd_show.h"

LOG_MODULE_REGISTER(io, CONFIG_APP_LOG_LEVEL);

static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

static void on_key(struct input_event *evt, void *user_data)
{
	if (evt->type != INPUT_EV_KEY) {
		return;
	}

	switch (evt->code) {
	case INPUT_KEY_0:		/* BN1 */
		LOG_INF("BN1 %s", evt->value ? "pressed" : "released");
		lcd_show_button(0, evt->value);
		gpio_pin_set_dt(&led2, evt->value);
		lcd_show_led(1, evt->value);
		break;
	case INPUT_KEY_1:		/* BN2 */
		LOG_INF("BN2 %s", evt->value ? "pressed" : "released");
		lcd_show_button(1, evt->value);
		break;
	default:
		break;
	}
}
INPUT_CALLBACK_DEFINE(NULL, on_key, NULL);

static void io_thread(void *p1, void *p2, void *p3)
{
	bool on = false;

	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);

	while (true) {
		on = !on;
		gpio_pin_set_dt(&led1, on);
		lcd_show_led(0, on);
		k_msleep(500);
	}
}

K_THREAD_DEFINE(io_tid, 1024, io_thread, NULL, NULL, NULL, 8, 0, 0);
