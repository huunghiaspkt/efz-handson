/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SHT30, once a second.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "lcd/lcd_show.h"

LOG_MODULE_REGISTER(env, CONFIG_APP_LOG_LEVEL);

static const struct device *const sht30 = DEVICE_DT_GET_ONE(sensirion_sht3xd);

static void env_thread(void *p1, void *p2, void *p3)
{
	struct sensor_value t, rh;

	if (!device_is_ready(sht30)) {
		LOG_ERR("SHT30 not ready");
		return;
	}
	LOG_INF("SHT30 ready");

	while (true) {
		if (sensor_sample_fetch(sht30) == 0 &&
		    sensor_channel_get(sht30, SENSOR_CHAN_AMBIENT_TEMP, &t) == 0 &&
		    sensor_channel_get(sht30, SENSOR_CHAN_HUMIDITY, &rh) == 0) {
			LOG_INF("%.1f C, %.1f %%", (double)sensor_value_to_float(&t),
				(double)sensor_value_to_float(&rh));
			lcd_show_temperature(sensor_value_to_float(&t));
			lcd_show_humidity(sensor_value_to_float(&rh));
		} else {
			LOG_WRN("SHT30 read failed");
		}
		k_msleep(1000);
	}
}

K_THREAD_DEFINE(env_tid, 1536, env_thread, NULL, NULL, NULL, 8, 0, 0);
