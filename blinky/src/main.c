#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#ifdef CONFIG_SUM_PRINT
#include "sum_printk.h"
#endif

int main(void)
{
	/* Give the serial terminal time to connect after reset. */
	k_sleep(K_SECONDS(2));

#ifdef CONFIG_SUM_PRINT
	int result = sum_printk(3, 5);

	printk("Returned result: %d\n", result);
#endif

	return 0;
}
