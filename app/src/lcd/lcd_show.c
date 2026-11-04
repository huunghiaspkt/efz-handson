/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LCD thread, page frame and the lcd_show_*() channels.
 */

#include <stdlib.h>
#include <math.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "lcd_show.h"
#include "lcd_page.h"

LOG_MODULE_REGISTER(lcd, CONFIG_APP_LOG_LEVEL);

#define MAX_PAGES	8
#define HEADER_H	20
#define DOTS_Y		(HEADER_H + LCD_PAGE_H + 5)

ZBUS_MSG_SUBSCRIBER_DEFINE(lcd_sub);

ZBUS_CHAN_DEFINE(lcd_temperature_chan, float, NULL, NULL,
		 ZBUS_OBSERVERS(lcd_sub), ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(lcd_humidity_chan, float, NULL, NULL,
		 ZBUS_OBSERVERS(lcd_sub), ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(lcd_accel_chan, struct lcd_accel_msg, NULL, NULL,
		 ZBUS_OBSERVERS(lcd_sub), ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(lcd_button_chan, struct lcd_io_msg, NULL, NULL,
		 ZBUS_OBSERVERS(lcd_sub), ZBUS_MSG_INIT(0));
ZBUS_CHAN_DEFINE(lcd_led_chan, struct lcd_io_msg, NULL, NULL,
		 ZBUS_OBSERVERS(lcd_sub), ZBUS_MSG_INIT(0));
/* BOOT: next page */
ZBUS_CHAN_DEFINE(lcd_next_page_chan, int, NULL, NULL,
		 ZBUS_OBSERVERS(lcd_sub), ZBUS_MSG_INIT(0));

static const struct gpio_dt_spec backlight = GPIO_DT_SPEC_GET(DT_NODELABEL(tft_bl), gpios);
static const struct device *const display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static const struct lcd_page *pages[MAX_PAGES];
static lv_obj_t *page_obj[MAX_PAGES];
static lv_obj_t *dots[MAX_PAGES];
static size_t page_count;
static size_t current;
static lv_obj_t *title;
static lv_obj_t *counter;
static lv_obj_t *accent_line;

static void publish(const struct zbus_channel *chan, const void *msg)
{
	int err = zbus_chan_pub(chan, msg, K_NO_WAIT);

	if (err) {
		LOG_WRN("%s: dropped (%d)", zbus_chan_name(chan), err);
	}
}

void lcd_show_temperature(float celsius)
{
	publish(&lcd_temperature_chan, &celsius);
}

void lcd_show_humidity(float percent)
{
	publish(&lcd_humidity_chan, &percent);
}

void lcd_show_accel(float x, float y, float z)
{
	struct lcd_accel_msg msg = {.x = x, .y = y, .z = z};

	publish(&lcd_accel_chan, &msg);
}

void lcd_show_button(int idx, bool pressed)
{
	struct lcd_io_msg msg = {.idx = idx, .on = pressed};

	publish(&lcd_button_chan, &msg);
}

void lcd_show_led(int idx, bool on)
{
	struct lcd_io_msg msg = {.idx = idx, .on = on};

	publish(&lcd_led_chan, &msg);
}

static void on_key(struct input_event *evt, void *user_data)
{
	int next = 1;

	if (evt->type == INPUT_EV_KEY && evt->code == INPUT_KEY_2 && evt->value) {
		publish(&lcd_next_page_chan, &next);
	}
}
INPUT_CALLBACK_DEFINE(NULL, on_key, NULL);

int lcd_page_register(const struct lcd_page *page)
{
	if (page_count == MAX_PAGES) {
		return -ENOMEM;
	}
	pages[page_count++] = page;
	return 0;
}

lv_obj_t *lcd_label(lv_obj_t *parent, const lv_font_t *font, uint32_t color, int32_t x,
		    int32_t y, const char *text)
{
	lv_obj_t *label = lv_label_create(parent);

	lv_obj_set_style_text_font(label, font, 0);
	lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
	lv_obj_set_pos(label, x, y);
	lv_label_set_text(label, text);
	return label;
}

const char *lcd_fmt_1dp(char *buf, size_t size, float v)
{
	int t = (int)lroundf(v * 10.0f);

	snprintk(buf, size, "%s%d.%d", t < 0 ? "-" : "", abs(t) / 10, abs(t) % 10);
	return buf;
}

static lv_obj_t *box(lv_obj_t *parent, int32_t w, int32_t h, uint32_t color)
{
	lv_obj_t *obj = lv_obj_create(parent);

	lv_obj_remove_style_all(obj);
	lv_obj_set_size(obj, w, h);
	lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
	lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
	return obj;
}

static void show_page(size_t n)
{
	const struct lcd_page *page = pages[n];
	int32_t x = (LCD_PAGE_W - (int32_t)(page_count - 1) * 8 - 10) / 2;

	current = n;
	LOG_INF("page %u/%u %s", (unsigned int)n + 1, (unsigned int)page_count, page->title);
	for (size_t i = 0; i < page_count; i++) {
		lv_obj_set_hidden(page_obj[i], i != n);
		lv_obj_set_pos(dots[i], x, DOTS_Y);
		lv_obj_set_width(dots[i], i == n ? 10 : 4);
		lv_obj_set_style_bg_color(dots[i],
					  lv_color_hex(i == n ? page->accent : LCD_COLOR_EDGE), 0);
		x += (i == n ? 10 : 4) + 4;
	}
	lv_label_set_text(title, page->title);
	lv_label_set_text_fmt(counter, "%u/%u", (unsigned int)n + 1, (unsigned int)page_count);
	lv_obj_align(counter, LV_ALIGN_TOP_RIGHT, -6, 4);
	lv_obj_set_style_bg_color(accent_line, lv_color_hex(page->accent), 0);
}

static int lcd_init(void)
{
	lv_obj_t *scr = lv_screen_active();

	if (!device_is_ready(display) || !gpio_is_ready_dt(&backlight)) {
		return -ENODEV;
	}

	lv_obj_set_style_bg_color(scr, lv_color_hex(LCD_COLOR_BG), 0);
	lv_obj_set_style_text_color(scr, lv_color_hex(LCD_COLOR_TEXT), 0);

	title = lcd_label(scr, &lv_font_montserrat_10, LCD_COLOR_TEXT, 6, 4, "EFZ-ESP32S3");
	counter = lcd_label(scr, &lv_font_montserrat_10, LCD_COLOR_MUTED, 0, 4, "");
	accent_line = box(scr, LCD_PAGE_W, 2, LCD_COLOR_EDGE);
	lv_obj_set_pos(accent_line, 0, HEADER_H - 2);

	for (size_t i = 0; i < page_count; i++) {
		page_obj[i] = lv_obj_create(scr);
		lv_obj_remove_style_all(page_obj[i]);
		lv_obj_set_size(page_obj[i], LCD_PAGE_W, LCD_PAGE_H);
		lv_obj_set_pos(page_obj[i], 0, HEADER_H);
		pages[i]->create(page_obj[i]);

		dots[i] = box(scr, 4, 4, LCD_COLOR_EDGE);
		lv_obj_set_style_radius(dots[i], 2, 0);
	}

	if (page_count > 0) {
		show_page(0);
	} else {
		lcd_label(scr, &lv_font_montserrat_12, LCD_COLOR_MUTED, 6, 30, "No modules\nenabled");
	}

	lv_timer_handler();
	display_blanking_off(display);
	return gpio_pin_configure_dt(&backlight, GPIO_OUTPUT_ACTIVE);
}

/* Only this thread touches LVGL. */
static void lcd_thread(void *p1, void *p2, void *p3)
{
	const struct zbus_channel *chan;
	union {
		float f;
		int i;
		struct lcd_accel_msg accel;
		struct lcd_io_msg io;
	} msg;
	uint32_t wait_ms = 0;

	if (lcd_init() != 0) {
		LOG_ERR("LCD not ready");
		return;
	}

	while (true) {
		if (zbus_sub_wait_msg(&lcd_sub, &chan, &msg, K_MSEC(wait_ms)) == 0) {
			if (chan == &lcd_next_page_chan) {
				if (page_count > 0) {
					show_page((current + 1) % page_count);
				}
			} else {
				for (size_t i = 0; i < page_count; i++) {
					pages[i]->on_msg(chan, &msg);
				}
			}
		}
		wait_ms = MIN(lv_timer_handler(), 100);
	}
}

K_THREAD_DEFINE(lcd_tid, 8192, lcd_thread, NULL, NULL, NULL, 7, 0, 0);
