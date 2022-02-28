/**
 * @brief Simple example using the libgpiod library, it generates a square waveform on the GPIO pin 12
 * @copyright (c) Pierre Boisselier
 * @date 2021-12-30
 * @example libgpiod_square.c
 * This continuously generates a square waveform until SIGINT is received (Ctrl+C).
 * The cleanup routine is not the best way to do it but this is a simple example to demonstrate
 * how to use the libgpiod library, not on how to properly manage resources and signals.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <gpiod.h>
#include <signal.h>

struct gpiod_chip *chip;
struct gpiod_line *line;

/* Free acquired resources when receiving SIGINT */
void cleanup(int signo)
{
	gpiod_line_release(line);
	gpiod_chip_close(chip);
	exit(EXIT_SUCCESS);
}

int main(void)
{
	/* DO NOT DO THIS 
	* signal() is DEPRECATED and should not be used in production code 
	* This call the function cleanup when SIGINT is received, meaning when the user press Ctrl+C.
	*/
	signal(SIGINT, cleanup);

	/* Open gpiochip0 */
	chip = gpiod_chip_open("/dev/gpiochip0");

	/* It is of good practice to check whether our calls worked */
	if (chip == NULL) {
		perror("unable to open gpiochip0");
		return EXIT_FAILURE;
	}

	/* We will use the GPIO pin 12 */
	const unsigned int gpio_pin = 12;

	/* Retrieve the GPIO line we want */
	line = gpiod_chip_get_line(chip, gpio_pin);
	if (line == NULL) {
		perror("unable to retrieve line");
		cleanup(0);
	}

	/* Reserve the GPIO line for us, also set the direction to output and set the default output to 0 (LOW) */
	/* The argument "consumer" is a string that will appear in gpioinfo, it used to tell which program is using the line */
	if (gpiod_line_request_output(line, "square_wvf", 0) < 0) {
		perror("unable to reserver line");
		cleanup(0);
	}

	/* Now we can generate the waveform */
	for (;;) {
		/* Set output to LOW */
		gpiod_line_set_value(line, 0);
		/* Set output to HIGH */
		gpiod_line_set_value(line, 1);
	}

	cleanup(0);

	return EXIT_SUCCESS;
}