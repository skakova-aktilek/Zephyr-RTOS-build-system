#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include "bme280_raw.h"

/* Bosch BME280 datasheet, sections 4.2 and 5.4. */
#define REG_CALIBRATION 0x88
#define REG_ID          0xd0
#define REG_RESET       0xe0
#define REG_CTRL_HUM    0xf2
#define REG_STATUS      0xf3
#define REG_CTRL_MEAS   0xf4
#define REG_TEMPERATURE 0xfa

static int wait_idle(const struct i2c_dt_spec *bus)
{
	int64_t deadline = k_uptime_get() + 100;
	uint8_t status;

	do {
		int ret = i2c_reg_read_byte_dt(bus, REG_STATUS, &status);

		if (ret < 0) {
			return ret;
		}
		/* Wait for measurement and calibration-copy operations to finish. */
		if ((status & (BIT(3) | BIT(0))) == 0) {
			return 0;
		}
		k_msleep(2);
	} while (k_uptime_get() < deadline);
	return -ETIMEDOUT;
}

int bme280_raw_init(struct bme280_raw *sensor)
{
	uint8_t id;
	uint8_t calibration[6];
	int ret;

	sensor->initialized = false;
	if (!i2c_is_ready_dt(&sensor->bus)) {
		return -ENODEV;
	}
	k_msleep(3);
	ret = i2c_reg_read_byte_dt(&sensor->bus, REG_ID, &id);
	if (ret < 0) {
		return ret;
	}
	if (id != 0x60) {
		return -ENODEV;
	}
	ret = i2c_reg_write_byte_dt(&sensor->bus, REG_RESET, 0xb6);
	if (ret < 0) {
		return ret;
	}
	k_msleep(3);
	ret = wait_idle(&sensor->bus);
	if (ret < 0) {
		return ret;
	}
	ret = i2c_burst_read_dt(&sensor->bus, REG_CALIBRATION, calibration,
			       sizeof(calibration));
	if (ret < 0) {
		return ret;
	}
	sensor->t1 = sys_get_le16(&calibration[0]);
	sensor->t2 = (int16_t)sys_get_le16(&calibration[2]);
	sensor->t3 = (int16_t)sys_get_le16(&calibration[4]);
	if (sensor->t1 == 0 || sensor->t1 == UINT16_MAX) {
		return -EINVAL;
	}
	/* Only temperature is required for this exercise. */
	ret = i2c_reg_write_byte_dt(&sensor->bus, REG_CTRL_HUM, 0);
	if (ret < 0) {
		return ret;
	}
	sensor->initialized = true;
	return 0;
}

int bme280_raw_read_temperature(struct bme280_raw *sensor, int32_t *temperature)
{
	uint8_t data[3];
	int ret;

	if (!sensor->initialized) {
		return -EACCES;
	}
	/* Temperature oversampling x1, pressure skipped, forced measurement. */
	ret = i2c_reg_write_byte_dt(&sensor->bus, REG_CTRL_MEAS, 0x21);
	if (ret < 0) {
		return ret;
	}
	/* Allow the conversion to start and finish before checking status. */
	k_msleep(10);
	ret = wait_idle(&sensor->bus);
	if (ret < 0) {
		return ret;
	}
	ret = i2c_burst_read_dt(&sensor->bus, REG_TEMPERATURE, data, sizeof(data));
	if (ret < 0) {
		return ret;
	}
	int32_t adc = ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) |
		      (data[2] >> 4);

	if (adc == 0x80000) {
		return -ENODATA;
	}
	/* Factory compensation, using wide intermediates to avoid overflow. */
	int64_t delta = (adc >> 4) - (int64_t)sensor->t1;
	int64_t linear = (((adc >> 3) - 2 * (int64_t)sensor->t1) * sensor->t2) >> 11;
	int64_t quadratic = (((delta * delta) >> 12) * sensor->t3) >> 14;

	*temperature = (int32_t)(((linear + quadratic) * 5 + 128) >> 8);
	return 0;
}
