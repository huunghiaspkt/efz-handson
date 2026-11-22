/*
 * SHT30 temperature and humidity: single-shot measurements through the
 * sensor API.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT embeddedfun_sht30

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(efz_sht30, CONFIG_SENSOR_LOG_LEVEL);

#define SHT30_CMD_MEASURE_HIGH 0x2400	/* single shot, high repeatability */
#define SHT30_MEASURE_WAIT_MS  15

struct sht30_data {
	uint16_t t_raw;
	uint16_t rh_raw;
};

struct sht30_config {
	struct i2c_dt_spec i2c;
};

/* Each 16-bit word is followed by a CRC-8 (poly 0x31, init 0xFF). */
static bool sht30_crc_ok(const uint8_t *word)
{
	return crc8(word, 2, 0x31, 0xFF, false) == word[2];
}

static int sht30_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct sht30_config *cfg = dev->config;
	struct sht30_data *data = dev->data;
	uint8_t cmd[2];
	uint8_t rx[6];
	int ret;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	sys_put_be16(SHT30_CMD_MEASURE_HIGH, cmd);
	ret = i2c_write_dt(&cfg->i2c, cmd, sizeof(cmd));
	if (ret < 0) {
		return ret;
	}

	k_msleep(SHT30_MEASURE_WAIT_MS);

	ret = i2c_read_dt(&cfg->i2c, rx, sizeof(rx));
	if (ret < 0) {
		return ret;
	}

	if (!sht30_crc_ok(&rx[0]) || !sht30_crc_ok(&rx[3])) {
		return -EIO;
	}

	data->t_raw = sys_get_be16(&rx[0]);
	data->rh_raw = sys_get_be16(&rx[3]);
	return 0;
}

static int sht30_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	const struct sht30_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* T = -45 + 175 * raw / 65535, in micro-degrees */
		return sensor_value_from_micro(val,
			-45000000LL + (int64_t)data->t_raw * 175000000LL / 65535);
	case SENSOR_CHAN_HUMIDITY:
		/* RH = 100 * raw / 65535, in micro-percent */
		return sensor_value_from_micro(val,
			(int64_t)data->rh_raw * 100000000LL / 65535);
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(sensor, sht30_api) = {
	.sample_fetch = sht30_sample_fetch,
	.channel_get = sht30_channel_get,
};

static int sht30_init(const struct device *dev)
{
	const struct sht30_config *cfg = dev->config;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}
	return 0;
}

#define SHT30_DEFINE(inst)                                                \
	static struct sht30_data sht30_data_##inst;                       \
	static const struct sht30_config sht30_config_##inst = {          \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                        \
	};                                                                \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sht30_init, NULL,              \
				     &sht30_data_##inst, &sht30_config_##inst, \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
				     &sht30_api);

DT_INST_FOREACH_STATUS_OKAY(SHT30_DEFINE)
