#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define SLEEP_TIME_MS 2000
#define LED_NODE DT_ALIAS(led5180)

static const struct gpio_dt_spec led =
	GPIO_DT_SPEC_GET(LED_NODE, gpios);

int main(void)
{
	int ret;
	bool led_state = false;

	printk("Blinky starting: controller=%s pin=%u\n", led.port->name,
	       (unsigned int)led.pin);

	if (!gpio_is_ready_dt(&led)) {
		printk("ERROR: GPIO controller not ready\n");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		printk("ERROR: GPIO configuration failed: %d\n", ret);
		return 0;
	}

	while (1) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			printk("ERROR: GPIO toggle failed: %d\n", ret);
			return 0;
		}

		led_state = !led_state;
		printk("LED state: %s\n", led_state ? "ON" : "OFF");
		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}
