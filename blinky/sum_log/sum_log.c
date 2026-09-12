#include <zephyr/logging/log.h>
#include "sum_log.h"

/* Enable all four severity levels for this lab's demonstration. */
LOG_MODULE_REGISTER(sum_log, LOG_LEVEL_DBG);

int sum_log(int a, int b)
{
	int inputs[] = {a, b};
	int result = a + b;

	LOG_ERR("Demonstration of an ERROR message");
	LOG_WRN("Demonstration of a WARNING message");
	LOG_INF("%d + %d = %d", a, b, result);
	LOG_DBG("Inputs: a=%d, b=%d", a, b);
	LOG_HEXDUMP_INF(inputs, sizeof(inputs), "Input bytes");

	return result;
}
