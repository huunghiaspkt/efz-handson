/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Buttons and LEDs page.
 */

#include "lcd/lcd_page.h"
#include "lcd/lcd_show.h"

#define GREEN	0x3DDC84

static lv_obj_t *pills[2];
static lv_obj_t *pill_text[2];
static lv_obj_t *leds[2];
static lv_obj_t *led_text[2];

static void set_pill(int i, bool pressed)
{
	lv_obj_set_style_bg_opa(pills[i], pressed ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_color(pills[i], lv_color_hex(pressed ? GREEN : LCD_COLOR_EDGE), 0);
	lv_obj_set_style_text_color(pill_text[i],
				    lv_color_hex(pressed ? LCD_COLOR_BG : LCD_COLOR_MUTED), 0);
}

static void set_led(int i, bool on)
{
	lv_obj_set_style_bg_color(leds[i], lv_color_hex(on ? GREEN : LCD_COLOR_SURFACE), 0);
	lv_obj_set_style_border_width(leds[i], on ? 0 : 1, 0);
	lv_obj_set_style_shadow_width(leds[i], on ? 8 : 0, 0);
	lv_obj_set_style_text_color(led_text[i],
				    lv_color_hex(on ? LCD_COLOR_TEXT : LCD_COLOR_MUTED), 0);
}

static void create(lv_obj_t *p)
{
	static const char *const pill_names[] = {"BN1", "BN2"};
	static const char *const led_names[] = {"LED1", "LED2"};

	lcd_label(p, &lv_font_montserrat_10, LCD_COLOR_MUTED, 6, 4, "Buttons");
	lcd_label(p, &lv_font_montserrat_10, LCD_COLOR_MUTED, 6, 48, "LEDs");

	for (int i = 0; i < 2; i++) {
		pills[i] = lv_obj_create(p);
		lv_obj_remove_style_all(pills[i]);
		lv_obj_set_size(pills[i], 56, 20);
		lv_obj_set_pos(pills[i], 6 + i * 60, 18);
		lv_obj_set_style_radius(pills[i], 10, 0);
		lv_obj_set_style_bg_color(pills[i], lv_color_hex(GREEN), 0);
		lv_obj_set_style_border_width(pills[i], 1, 0);
		pill_text[i] = lv_label_create(pills[i]);
		lv_obj_set_style_text_font(pill_text[i], &lv_font_montserrat_12, 0);
		lv_label_set_text(pill_text[i], pill_names[i]);
		lv_obj_center(pill_text[i]);
		set_pill(i, false);

		leds[i] = lv_obj_create(p);
		lv_obj_remove_style_all(leds[i]);
		lv_obj_set_size(leds[i], 22, 22);
		lv_obj_set_pos(leds[i], 23 + i * 60, 64);
		lv_obj_set_style_radius(leds[i], LV_RADIUS_CIRCLE, 0);
		lv_obj_set_style_bg_opa(leds[i], LV_OPA_COVER, 0);
		lv_obj_set_style_border_color(leds[i], lv_color_hex(LCD_COLOR_EDGE), 0);
		lv_obj_set_style_shadow_color(leds[i], lv_color_hex(GREEN), 0);
		led_text[i] = lcd_label(p, &lv_font_montserrat_10, LCD_COLOR_MUTED, 6 + i * 60, 90,
					led_names[i]);
		lv_obj_set_width(led_text[i], 56);
		lv_obj_set_style_text_align(led_text[i], LV_TEXT_ALIGN_CENTER, 0);
		set_led(i, false);
	}
}

static void on_msg(const struct zbus_channel *chan, const void *msg)
{
	const struct lcd_io_msg *io = msg;

	if (chan != &lcd_button_chan && chan != &lcd_led_chan) {
		return;
	}
	if (io->idx < 0 || io->idx > 1) {
		return;
	}
	if (chan == &lcd_button_chan) {
		set_pill(io->idx, io->on);
	} else {
		set_led(io->idx, io->on);
	}
}

LCD_PAGE_DEFINE(io_page, 3, "I/O", GREEN, create, on_msg);
