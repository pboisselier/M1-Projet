/**
 * @brief Simple example using the gpiod-isr wrapper, it register an intterupt for falling edge events on GPIO pin 12.
 * @copyright (c) Pierre Boisselier
 * @date 2021-12-30
 * @example libgpiod_interrupts.c
 * This register an interrupt for falling edge events on GPIO pin 12 and wait inside an infinite loop until the user
 * writes something on the console to stop the program.
 */

#include <gpiod-isr.h>
#include <gpiod.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * Interrupt handler called when an interrupt on a GPIO pin happened.
 */
void gpio_handler(struct gpiod_line *line, struct gpiod_line_event *event)
{
	printf("GPIO event (%d) on pin %s happened %lld.%.9ld seconds after boot!\n",
	       event->event_type, gpiod_line_name(line),
	       (long long)event->ts.tv_sec, event->ts.tv_nsec);
}

int main(void)
{
	/* Open gpiochip0 */
	struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip0");
	if (!chip) {
		perror("unable to open gpiochip");
		exit(EXIT_FAILURE);
	}

	/* We will use the GPIO pin 12 */
	const unsigned int gpio_pin = 12;

	/* Retrieve the GPIO line we want */
	struct gpiod_line *line = gpiod_chip_get_line(chip, gpio_pin);
	if (!line) {
		perror("unable to get line");
		goto cleanup;
	}

	/* Reserve the line for falling edge detection, also tells which
	 * function will act as the interrupt handler.
	 * In this case gpio_handler is our interrupt handler.
	 */
	struct gpiod_isr *isr = gpiod_isr_request_falling_edge_events(
		line, "gpiod_interrupts", gpio_handler);
	if (!isr) {
		perror("unable to register interrupt");
		goto cleanup;
	}

	/* Wait until something is written into the console */
	for (;;) {
		if (getchar())
			break;
	}

	/* Remove interrupt and release line */
	gpiod_isr_release(isr);
	/* Do not forget to free your acquired resources */
	gpiod_chip_close(chip);

	return EXIT_SUCCESS;

cleanup:
	gpiod_line_release(line);
	gpiod_chip_close(chip);

	return EXIT_FAILURE;
}
