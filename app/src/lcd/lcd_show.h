/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Show data on the LCD. Safe from any thread or ISR.
 */

#ifndef APP_LCD_SHOW_H_
#define APP_LCD_SHOW_H_

#include <stdbool.h>
#include <zephyr/zbus/zbus.h>

struct lcd_accel_msg {
	float x;
	float y;
	float z;
};

struct lcd_io_msg {
	int idx;
	bool on;
};

ZBUS_CHAN_DECLARE(lcd_temperature_chan, lcd_humidity_chan, lcd_accel_chan,
		  lcd_button_chan, lcd_led_chan);

void lcd_show_temperature(float celsius);
void lcd_show_humidity(float percent);
void lcd_show_accel(float x, float y, float z);	/* m/s^2 */
void lcd_show_button(int idx, bool pressed);	/* 0 BN1, 1 BN2, 2 BOOT */
void lcd_show_led(int idx, bool on);		/* 0 LED1, 1 LED2 */

#endif /* APP_LCD_SHOW_H_ */
