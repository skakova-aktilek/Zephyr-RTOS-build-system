#include <errno.h>
#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/byteorder.h>
#include "bme280_raw.h"

#define SENSOR_NODE DT_NODELABEL(bme5180)

static uint8_t registers[256];
static int fail_read;
static int fail_write;
static bool busy;

/* Mock the bus, not the application: the real I2C register calls run here. */
static int mock_transfer(const struct device *dev, struct i2c_msg *msgs,
			uint8_t count, uint16_t addr)
{
	ARG_UNUSED(dev);
	if (addr != 0x77 || count == 0 || msgs[0].len == 0) {
		return -EIO;
	}
	uint8_t reg = msgs[0].buf[0];

	if (count == 2 && msgs[0].len == 1 && !(msgs[0].flags & I2C_MSG_READ) &&
	    (msgs[1].flags & I2C_MSG_READ)) {
		if (fail_read == reg || reg + msgs[1].len > sizeof(registers)) {
			return -EIO;
		}
		memcpy(msgs[1].buf, &registers[reg], msgs[1].len);
		if (reg == 0xf3 && busy) {
			msgs[1].buf[0] = BIT(3);
		}
		return 0;
	}
	if (count == 1 && msgs[0].len == 2 && !(msgs[0].flags & I2C_MSG_READ)) {
		if (fail_write == reg) {
			return -EIO;
		}
		registers[reg] = msgs[0].buf[1];
		return 0;
	}
	return -EIO;
}

static DEVICE_API(i2c, mock_api) = {
	.transfer = mock_transfer,
};

DEVICE_DT_DEFINE(DT_NODELABEL(i2c1), NULL, NULL, NULL, NULL,
		 POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, &mock_api);

static struct bme280_raw sensor = {
	.bus = I2C_DT_SPEC_GET(SENSOR_NODE),
};

static void set_raw(uint32_t raw)
{
	registers[0xfa] = raw >> 12;
	registers[0xfb] = raw >> 4;
	registers[0xfc] = (raw & 0xf) << 4;
}

static void before_test(void *fixture)
{
	ARG_UNUSED(fixture);
	memset(registers, 0, sizeof(registers));
	fail_read = -1;
	fail_write = -1;
	busy = false;
	sensor.initialized = false;
	registers[0xd0] = 0x60;
	sys_put_le16(27504, &registers[0x88]);
	sys_put_le16(26435, &registers[0x8a]);
	sys_put_le16((uint16_t)-1000, &registers[0x8c]);
	set_raw(519888);
}

ZTEST(bme280_test_suite, test_devicetree)
{
	zassert_true(DT_NODE_HAS_STATUS(SENSOR_NODE, okay), "Sensor node must be enabled");
	zassert_true(DT_ON_BUS(SENSOR_NODE, i2c), "Sensor must be on I2C");
	zassert_equal(DT_REG_ADDR(SENSOR_NODE), 0x77, "Check the sensor address");
	zassert_equal(DT_PROP(DT_BUS(SENSOR_NODE), clock_frequency), 100000);
	zassert_true(i2c_is_ready_dt(&sensor.bus), "Mock I2C controller must be ready");
}

ZTEST(bme280_test_suite, test_calibrated_temperature)
{
	zassert_ok(bme280_raw_init(&sensor));
	zassert_equal(sensor.t1, 27504);
	zassert_equal(sensor.t2, 26435);
	zassert_equal(sensor.t3, -1000, "Calibration must be decoded as signed");
	zassert_equal(registers[0xe0], 0xb6, "Initialization should reset the sensor");
	int32_t temperature;

	zassert_ok(bme280_raw_read_temperature(&sensor, &temperature));
	zassert_equal(registers[0xf4], 0x21, "Use temperature x1 and forced mode");
	zassert_equal(temperature, 2508, "Reference sample should give 25.08 C");
}

ZTEST(bme280_test_suite, test_wrong_chip_id)
{
	registers[0xd0] = 0x58;
	zassert_equal(bme280_raw_init(&sensor), -ENODEV);
	zassert_false(sensor.initialized);
}

ZTEST(bme280_test_suite, test_invalid_calibration)
{
	sys_put_le16(0, &registers[0x88]);
	zassert_equal(bme280_raw_init(&sensor), -EINVAL);
	zassert_false(sensor.initialized);
}

ZTEST(bme280_test_suite, test_init_read_failure)
{
	fail_read = 0x88;
	zassert_equal(bme280_raw_init(&sensor), -EIO);
	zassert_false(sensor.initialized);
}

ZTEST(bme280_test_suite, test_measurement_read_failure)
{
	zassert_ok(bme280_raw_init(&sensor));
	fail_read = 0xfa;
	int32_t temperature = 12345;

	zassert_equal(bme280_raw_read_temperature(&sensor, &temperature), -EIO);
	zassert_equal(temperature, 12345, "Failure must not replace the output value");
}

ZTEST(bme280_test_suite, test_measurement_write_failure)
{
	zassert_ok(bme280_raw_init(&sensor));
	fail_write = 0xf4;
	int32_t temperature;

	zassert_equal(bme280_raw_read_temperature(&sensor, &temperature), -EIO);
}

ZTEST(bme280_test_suite, test_measurement_timeout)
{
	zassert_ok(bme280_raw_init(&sensor));
	busy = true;
	int32_t temperature;

	zassert_equal(bme280_raw_read_temperature(&sensor, &temperature), -ETIMEDOUT);
}

ZTEST(bme280_test_suite, test_skipped_temperature)
{
	zassert_ok(bme280_raw_init(&sensor));
	set_raw(0x80000);
	int32_t temperature;

	zassert_equal(bme280_raw_read_temperature(&sensor, &temperature), -ENODATA);
}

ZTEST(bme280_test_suite, test_requires_initialization)
{
	int32_t temperature;

	zassert_equal(bme280_raw_read_temperature(&sensor, &temperature), -EACCES);
}

ZTEST_SUITE(bme280_test_suite, NULL, NULL, before_test, NULL, NULL);
