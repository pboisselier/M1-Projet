/**
 * @brief Example using the TLC1543 library
 * @copyright (c) Pierre Boisselier 
 * 
 * @example tlc1543.c
 * This example gets 20 samples from the ADC and display them. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpi600/arpi600.h>

int main(void)
{
	struct tlc1543 tlc;
	int value = 0;

	/* Initialize the TLC1543 ADC and use the EXCLUSIVE flag. */
	if (tlc1543_init(&tlc, TLC1543_OPT_EXCLUSIVE) < 0) {
		perror("unable to init the TLC1543");
		return EXIT_FAILURE;
	}

	for (int i = 0; i < 20; ++i) {
		/* Acquire sample from the ADC. */
		value = tlc1543_get_sample(&tlc, 0);
		if (value < 0) {
			perror("unable to read from the ADC");
		}
		/* Display value read. */
		printf("ADC read: %d\n", value);
	}

	/* Free up resources. */
	tlc1543_delete(&tlc);

	return EXIT_SUCCESS;
}