#include <zephyr/ztest.h>
#include "sum_log.h"

ZTEST(sum_log_test_suite, test_sum_log_basic)
{
	zassert_equal(sum_log(3, 5), 8, "3 + 5 should equal 8");
	zassert_equal(sum_log(5, 3), 8, "5 + 3 should equal 8");
}

ZTEST(sum_log_test_suite, test_sum_log_negative)
{
	zassert_equal(sum_log(-3, -5), -8, "Two negatives should sum to -8");
	zassert_equal(sum_log(-3, 5), 2, "Mixed signs should sum to 2");
	zassert_equal(sum_log(3, -5), -2, "Mixed signs should sum to -2");
}

ZTEST(sum_log_test_suite, test_sum_log_zero)
{
	zassert_equal(sum_log(0, 0), 0, "0 + 0 should equal 0");
	zassert_equal(sum_log(7, 0), 7, "Adding zero should preserve the value");
	zassert_equal(sum_log(0, -7), -7, "Zero plus a negative should preserve it");
}

ZTEST_SUITE(sum_log_test_suite, NULL, NULL, NULL, NULL, NULL);
