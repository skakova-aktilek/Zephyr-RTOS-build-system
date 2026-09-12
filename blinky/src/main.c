#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "bme280_raw.h"

LOG_MODULE_REGISTER(temperature_app, LOG_LEVEL_INF);

#define SENSOR_NODE DT_NODELABEL(bme5180)
BUILD_ASSERT(DT_NODE_HAS_STATUS(SENSOR_NODE, okay), "Enable the BME280 node");
BUILD_ASSERT(DT_ON_BUS(SENSOR_NODE, i2c), "BME280 must be on an I2C bus");

static struct bme280_raw sensor = {
	.bus = I2C_DT_SPEC_GET(SENSOR_NODE),
};

int main(void)
{
	k_sleep(K_SECONDS(2));
	LOG_INF("BME280: bus=%s address=0x%02x", sensor.bus.bus->name, sensor.bus.addr);

	int ret = bme280_raw_init(&sensor);

	if (ret < 0) {
		LOG_ERR("BME280 initialization failed (%d). Check power, wiring and I2C address.", ret);
		return 0;
	}
	LOG_INF("BME280 detected; factory temperature calibration loaded");

	while (1) {
		int32_t temperature;

		ret = bme280_raw_read_temperature(&sensor, &temperature);
		if (ret < 0) {
			LOG_ERR("Temperature read failed (%d)", ret);
		} else {
			int32_t magnitude = temperature < 0 ? -temperature : temperature;

			LOG_INF("Temperature: %s%d.%02d C", temperature < 0 ? "-" : "",
				(int)(magnitude / 100), (int)(magnitude % 100));
		}
		k_sleep(K_SECONDS(2));
	}
	return 0;
}
