#ifndef BME280_RAW_H
#define BME280_RAW_H

#include <zephyr/drivers/i2c.h>

struct bme280_raw {
	struct i2c_dt_spec bus;
	uint16_t t1;
	int16_t t2;
	int16_t t3;
	bool initialized;
};

int bme280_raw_init(struct bme280_raw *sensor);
/* Returns temperature in hundredths of a degree Celsius. */
int bme280_raw_read_temperature(struct bme280_raw *sensor, int32_t *temperature);

#endif
