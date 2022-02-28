/**
 * @brief Simple example using the libgpiod library, it waits for a falling edge on GPIO Pin 12.
 * @copyright (c) Pierre Boisselier
 * @date 2021-01-02
 * @example libgpiod_events.c
 * This wait for the GPIO pin 12 to go from HIGH to LOW (falling edge) and then display at what time
 * the event happened.
 */

#include <stdio.h>
#include <stdlib.h>
#include <gpiod.h>
#include <errno.h>

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

	/* Reserve the line for falling edge watching */
	if (gpiod_line_request_falling_edge_events(line, "event_with_gpiod") <
	    0) {
		perror("unable to reserve line");
		goto cleanup;
	}

	/* Prepare to store the event information */
	struct gpiod_line_event event;

	/* Wait for an event to happen
        * NOTE: The library allows you to provide a timeout time,
        *       using NULL will wait until an event happened or an 
        *       error occured.
        */
	if (gpiod_line_event_wait(line, NULL) < 0) {
		perror("error while waiting for event");
		goto cleanup2;
	}

	/* Read event informations */
	if (gpiod_line_event_read(line, &event) < 0) {
		perror("unable to read event");
		goto cleanup2;
	}

	/* Now you can do your event handling.
        * The structure gpiod_line_event provides the type of event that
        * happened (GPIOD_LINE_EVENT_RISING_EDGE or GPIOD_LINE_FALLING_EDGE)
        * and the approximate time it happened.
        *
        * As an example we are going to use printf:
        */
	printf("Event (%d) on GPIO pin %d happened %lld.%.9ld seconds after boot!\n",
	       event.event_type, gpio_pin, (long long)event.ts.tv_sec,
	       event.ts.tv_nsec);

	/* Do not forget to free your acquired resources */
	gpiod_line_release(line);
	gpiod_chip_close(chip);

	return EXIT_SUCCESS;

cleanup2:
	gpiod_line_release(line);
cleanup:
	gpiod_chip_close(chip);

	return EXIT_FAILURE;
}