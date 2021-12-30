/**
 * @date 2021-12-30
 * @copyright (c) Pierre Boisselier
 */

#include <gpiod-isr.h>
#include <gpiod.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

void gpio_handler(struct gpiod_line *line, struct gpiod_line_event *event)
{
	printf("GPIO event on pin %s at %lld.%.9ld\n", gpiod_line_name(line),
	       (long long)event->ts.tv_sec, event->ts.tv_nsec);
}

int main(void)
{
	struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip0");
	if (!chip) {
		perror("unable to open gpiochip");
		exit(EXIT_FAILURE);
	}

	int gpio_pin = 12;
	struct gpiod_line *line = gpiod_chip_get_line(chip, gpio_pin);
	if (!line) {
		perror("unable to get line");
		goto cleanup;
	}

	struct gpiod_isr *isr = gpiod_isr_request_falling_edge_events(
		line, "interrupt_with_gpiod", gpio_handler);
	if (!isr) {
		perror("unable to request isr");
		goto cleanup;
	}

	gpio_pin = 13;
	struct gpiod_line *line2 = gpiod_chip_get_line(chip, gpio_pin);
	if (!line2) {
		perror("unable to get line");
		goto cleanup;
	}

	struct gpiod_isr *isr2 = gpiod_isr_request_both_edges_events(
		line2, "interrupt_with_gpiod", gpio_handler);
	if (!isr2) {
		perror("unable to request isr");
		goto cleanup;
	}

	sleep(10);

	gpiod_isr_remove_event(isr);
	gpiod_isr_remove_event(isr2);

cleanup:
	gpiod_line_release(line);
	gpiod_line_release(line2);
	gpiod_chip_close(chip);

	return EXIT_FAILURE;
}
//	if (gpiod_line_request_input(line, "interrupt_with_gpiod") < 0) {
//		perror("unable to reserver line as input");
//		goto cleanup;
//	}
//
//	if (gpiod_line_request_rising_edge_events(line,
//						  "interrupt_with_gpiod") < 0) {
//		perror("unable to request rising edge");
//		goto cleanup;
//	}
//
//	printf("waiting for event...\n");
//	if (gpiod_line_event_wait(line, NULL) != 1) {
//		perror("error event or timeout");
//		goto cleanup;
//	}
//
//	struct gpiod_line_event event;
//	gpiod_line_event_read(line, &event);
//	gpio_handler(line, &event);
//
/* Do something else... */
//	for (;;) {
//		sleep(1);
//		printf("Doing something...\n");
//	}