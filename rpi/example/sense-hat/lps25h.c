/**
 * @brief Example using the Pressure Sensor LPS25H.
 * @date 2022-01-05
 * 
 * @copyright (c) Pierre Boisselier
 * @example lps25h.c
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <sense-hat/lps25h.h>

int main(void)
{
	struct lps25h lps;
        lps25h_init_c_l(&lps, "/dev/i2c-1", 0x5c, LPS25H_OPT_WAKEUP);

	double pressure;
        double temperature;
	for (;;) {
		pressure = lps25h_get_pressure(&lps);
		temperature = lps25h_get_temperature(&lps);
		printf("Pressure: %lf hPa\n", pressure);
		printf("Temperature: %lf °C\n", temperature);
	}

        return 0;
}