#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define POLL_INTERVAL_MS 10
#define DEBOUNCE_MS 30
#define LED_NODE DT_ALIAS(led5180)
#define BUTTON_NODE DT_ALIAS(button5180)

static const struct gpio_dt_spec led =
	GPIO_DT_SPEC_GET(LED_NODE, gpios);
static const struct gpio_dt_spec button =
	GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);

int main(void)
{
	int ret;
	bool led_state = false;
	int previous_reading = 0;
	int stable_state = 0;
	int64_t changed_at = k_uptime_get();

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

	if (!gpio_is_ready_dt(&button)) {
		printk("ERROR: Button GPIO controller not ready\n");
		return 0;
	}

	/* The devicetree supplies the button's pull-up and active-low flags. */
	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		printk("ERROR: Button configuration failed: %d\n", ret);
		return 0;
	}
	printk("Ready: press Button 1 to toggle LED2\n");

	while (1) {
		/* Logical 1 means pressed, even though the physical input is low. */
		ret = gpio_pin_get_dt(&button);
		if (ret < 0) {
			printk("ERROR: Button read failed: %d\n", ret);
			return 0;
		}

		int64_t now = k_uptime_get();

		if (ret != previous_reading) {
			previous_reading = ret;
			changed_at = now;
		}

		/* Accept a change only after 30 ms without contact bounce. */
		if (ret != stable_state && now - changed_at >= DEBOUNCE_MS) {
			stable_state = ret;
			if (stable_state == 1) {
				ret = gpio_pin_toggle_dt(&led);
				if (ret < 0) {
					printk("ERROR: GPIO toggle failed: %d\n", ret);
					return 0;
				}
				led_state = !led_state;
				printk("LED state: %s\n", led_state ? "ON" : "OFF");
			}
		}
		k_msleep(POLL_INTERVAL_MS);
	}

	return 0;
}
