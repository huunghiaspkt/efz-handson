/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LCD pages. The page callbacks run in the LCD thread.
 */

#ifndef APP_LCD_PAGE_H_
#define APP_LCD_PAGE_H_

#include <stddef.h>
#include <zephyr/init.h>
#include <zephyr/zbus/zbus.h>
#include <lvgl.h>

/* 0xRRGGBB */
#define LCD_COLOR_BG		0x0B0F14
#define LCD_COLOR_SURFACE	0x151B23
#define LCD_COLOR_LINE		0x1F2933
#define LCD_COLOR_EDGE		0x3A4552
#define LCD_COLOR_TEXT		0xE8EDF2
#define LCD_COLOR_MUTED		0x8A96A3

/* Page area, between the header and the dots */
#define LCD_PAGE_W		128
#define LCD_PAGE_H		126

struct lcd_page {
	const char *title;
	uint32_t accent;
	/* Called once */
	void (*create)(lv_obj_t *parent);
	/* Called for each lcd_show_*() message, hidden or not */
	void (*on_msg)(const struct zbus_channel *chan, const void *msg);
};

int lcd_page_register(const struct lcd_page *page);

/* order: 1..99, lower comes first */
#define LCD_PAGE_DEFINE(name, order, _title, _accent, _create, _on_msg)	\
	static const struct lcd_page name = {				\
		.title = _title,					\
		.accent = _accent,					\
		.create = _create,					\
		.on_msg = _on_msg,					\
	};								\
	static int name##_register(void)				\
	{								\
		return lcd_page_register(&name);			\
	}								\
	SYS_INIT(name##_register, APPLICATION, order)

lv_obj_t *lcd_label(lv_obj_t *parent, const lv_font_t *font, uint32_t color, int32_t x,
		    int32_t y, const char *text);

/* -1.25 -> "-1.3" */
const char *lcd_fmt_1dp(char *buf, size_t size, float v);

#endif /* APP_LCD_PAGE_H_ */
