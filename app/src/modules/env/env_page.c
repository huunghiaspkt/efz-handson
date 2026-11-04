/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Temperature and humidity page.
 */

#include <math.h>

#include "lcd/lcd_page.h"
#include "lcd/lcd_show.h"

#define AMBER	0xFFB020
#define TEAL	0x2EC4B6
#define POINTS	30

static lv_obj_t *temp_value;
static lv_obj_t *temp_unit;
static lv_obj_t *humi_value;
static lv_obj_t *humi_unit;
static lv_obj_t *chart;
static lv_chart_series_t *series;
static lv_obj_t *bar;

static void create(lv_obj_t *p)
{
	lcd_label(p, &lv_font_montserrat_10, LCD_COLOR_MUTED, 6, 4, "Temperature");
	temp_value = lcd_label(p, &lv_font_montserrat_24, LCD_COLOR_TEXT, 6, 15, "--");
	temp_unit = lcd_label(p, &lv_font_montserrat_12, AMBER, 0, 0, "\xC2\xB0" "C");
	lv_obj_align_to(temp_unit, temp_value, LV_ALIGN_OUT_RIGHT_BOTTOM, 3, -4);

	chart = lv_chart_create(p);
	lv_obj_remove_style_all(chart);
	lv_obj_set_size(chart, 116, 18);
	lv_obj_set_pos(chart, 6, 43);
	lv_obj_set_style_border_side(chart, LV_BORDER_SIDE_BOTTOM, 0);
	lv_obj_set_style_border_width(chart, 1, 0);
	lv_obj_set_style_border_color(chart, lv_color_hex(LCD_COLOR_LINE), 0);
	lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);
	lv_obj_set_style_width(chart, 0, LV_PART_INDICATOR);
	lv_obj_set_style_height(chart, 0, LV_PART_INDICATOR);
	lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
	lv_chart_set_div_line_count(chart, 0, 0);
	lv_chart_set_point_count(chart, POINTS);
	series = lv_chart_add_series(chart, lv_color_hex(AMBER), LV_CHART_AXIS_PRIMARY_Y);

	lcd_label(p, &lv_font_montserrat_10, LCD_COLOR_MUTED, 6, 66, "Humidity");
	humi_value = lcd_label(p, &lv_font_montserrat_24, LCD_COLOR_TEXT, 6, 77, "--");
	humi_unit = lcd_label(p, &lv_font_montserrat_12, TEAL, 0, 0, "%");
	lv_obj_align_to(humi_unit, humi_value, LV_ALIGN_OUT_RIGHT_BOTTOM, 3, -4);

	bar = lv_bar_create(p);
	lv_obj_remove_style_all(bar);
	lv_obj_set_size(bar, 116, 4);
	lv_obj_set_pos(bar, 6, 106);
	lv_obj_set_style_bg_color(bar, lv_color_hex(LCD_COLOR_LINE), 0);
	lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
	lv_obj_set_style_radius(bar, 2, 0);
	lv_obj_set_style_bg_color(bar, lv_color_hex(TEAL), LV_PART_INDICATOR);
	lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
	lv_obj_set_style_radius(bar, 2, LV_PART_INDICATOR);
	lv_bar_set_range(bar, 0, 100);
}

/* 0.1 C units, y range follows the data */
static void add_point(float celsius)
{
	int32_t *y;
	int32_t lo = INT32_MAX;
	int32_t hi = INT32_MIN;

	lv_chart_set_next_value(chart, series, (int32_t)lroundf(celsius * 10.0f));
	y = lv_chart_get_series_y_array(chart, series);
	for (int i = 0; i < POINTS; i++) {
		if (y[i] != LV_CHART_POINT_NONE) {
			lo = MIN(lo, y[i]);
			hi = MAX(hi, y[i]);
		}
	}
	lv_chart_set_axis_range(chart, LV_CHART_AXIS_PRIMARY_Y, lo - 5, hi + 5);
}

static void on_msg(const struct zbus_channel *chan, const void *msg)
{
	char v[16];

	if (chan == &lcd_temperature_chan) {
		float t = *(const float *)msg;

		lv_label_set_text(temp_value, lcd_fmt_1dp(v, sizeof(v), t));
		lv_obj_align_to(temp_unit, temp_value, LV_ALIGN_OUT_RIGHT_BOTTOM, 3, -4);
		add_point(t);
	} else if (chan == &lcd_humidity_chan) {
		float rh = *(const float *)msg;

		lv_label_set_text(humi_value, lcd_fmt_1dp(v, sizeof(v), rh));
		lv_obj_align_to(humi_unit, humi_value, LV_ALIGN_OUT_RIGHT_BOTTOM, 3, -4);
		lv_bar_set_value(bar, (int32_t)lroundf(rh), LV_ANIM_OFF);
	}
}

LCD_PAGE_DEFINE(env_page, 1, "ENVIRONMENT", AMBER, create, on_msg);
