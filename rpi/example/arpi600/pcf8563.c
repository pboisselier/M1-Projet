/**
 * @brief Simple example utilizing the PCF8563 module on the ARPI600
 * @copyright (c) Pierre Boisselier
 * 
 * @example pcf8563.c 
 * Read time from the Real-Time clock. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpi600/pcf8563.h>
#include <time.h>
#include <errno.h>

int main(int argc, char **argv)
{
	time_t tm;

	/* Initialize the RTC. */
	int pcf_i2c = pcf8563_init();
	if (pcf_i2c < 0)
		pcf8563_print_err(pcf_i2c, "init pcf8563");

	/* Read time and store it in a time_t structure. */
	if (pcf8563_read_time(pcf_i2c, &tm) < 0)
		pcf8563_print_err(-1, "reading time");

	/* Display time in GMT format. */
	printf("RTC Time: %s\n", asctime(gmtime(&tm)));

	/* Free up resources. */
	if (pcf8563_close(pcf_i2c) < 0)
		pcf8563_print_err(-1, "close pcf8563");

	return EXIT_SUCCESS;
}