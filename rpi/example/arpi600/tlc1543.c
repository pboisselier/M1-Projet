/**
 * @brief Example using the TLC1543 library
 * @copyright (c) Pierre Boisselier 
 * 
 * @example tlc1543.c
 * Compile using:
 * ```sh
 * cc -I../../include tlc1543.c -std=c11 -Wall -g -lgpiod -o tlc1543 
 * ```
 */
#include <stdio.h>
#include <stdlib.h>
#include <arpi600/arpi600.h>

int main(void)
{
	struct tlc1543 tlc;
	int value = 0;

	if (tlc1543_init(&tlc, TLC1543_OPT_EXCLUSIVE) < 0) {
		perror("unable to init the TLC1543");
		return EXIT_FAILURE;
	}

	for (int i = 0; i < 20; ++i) {
		value = tlc1543_get_sample(&tlc, 0);
		if (value < 0) {
			perror("unable to read from the ADC");
		}
		printf("ADC read: %d\n", value);
	}

	tlc1543_delete(&tlc);

	return EXIT_SUCCESS;
}

/* Some tests */
//	if (!fork()) {
//		fflush(stdout);
//		if (tlc1543_init(&tlc, TLC1543_OPT_WAIT) < 0) {
//			perror("error initializing");
//			return EXIT_FAILURE;
//		}
//		for (int i = 0; i < 10; ++i) {
//			value = tlc1543_get_sample(&tlc, 0);
//			if (value < 0)
//				perror("error getting a sample");
//			printf("ADC Read child: %d => %x\n", value, value);
//			fflush(stdout);
//		}
//		tlc1543_delete(&tlc);
//	} else {
//		if (tlc1543_init(&tlc, TLC1543_OPT_EXCLUSIVE) < 0) {
//			perror("error initializing tlc");
//			return EXIT_FAILURE;
//		}
//		for (int i = 0; i < 10; ++i) {
//			value = tlc1543_get_sample(&tlc, 13);
//			if (value < 0)
//				perror("error getting a sample");
//			printf("ADC Read: %d => %x\n", value, value);
//			fflush(stdout);
//		}
//		tlc1543_delete(&tlc);
//	}
