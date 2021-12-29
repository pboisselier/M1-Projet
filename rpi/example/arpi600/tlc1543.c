/**
 * @file tl1543.c
 * @author Pierre Boisselier (Raclette2K@users.noreply.github.com)
 * @brief Example using the TLC1543 library
 * 
 * @copyright Copyright (c) 2021
 *
 * Compile:
 *      cc -I../../include tlc1543.c -std=c11 -Wall -g -lgpiod -o tlc1543 
 * 
 */

#include <arpi600/tlc1543.h>

int main(void)
{
	struct tlc1543 tlc;
	int value = 0;
	if (tlc1543_init(&tlc) < 0)
		perror("error initializing");
	
	for (int i = 0; i < 100000; ++i) {
		value = tlc1543_get_sample(&tlc, 0);
		if (value < 0)
			perror("error getting a sample");
		printf("ADC Read: %d\n", value);
	}

	tlc1543_delete(&tlc);

	return EXIT_SUCCESS;
}