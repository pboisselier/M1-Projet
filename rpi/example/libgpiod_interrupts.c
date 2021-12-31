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

	setvbuf(stdout, NULL, _IONBF, 0);


	struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip0");
	if (!chip) {
		perror("unable to open gpiochip");
		exit(EXIT_FAILURE);
	}

	int gpio_pins[2] = { 12, 13 };
	struct gpiod_line_bulk lines;
	if (gpiod_chip_get_lines(chip, gpio_pins, 2, &lines) < 0) {
		perror("unable to get lines");
		goto cleanup;
	}

	struct gpiod_isr_bulk *isr = gpiod_isr_request_bulk_falling_edge_events(
		&lines, "interrupt_with_gpiod", gpio_handler);
	if (!isr) {
		perror("unable to register ISR");
		goto cleanup;
	}
	//	gpiod_line_request_bulk_falling_edge_events(
	//		&lines, "interrupt_with_gpiod_test");
	//
	//	struct gpiod_line_event event;
	//	struct gpiod_line_bulk line_event;
	//	for (;;) {
	//		printf("waiting...\n");
	//		gpiod_line_event_wait_bulk(&lines, NULL, &line_event);
	//		printf("event: %d\n", line_event.num_lines);
	//		for (int i = 0; i < line_event.num_lines; ++i) {
	//			printf("reading...\n");
	//			printf("wut: %d\n",
	//			       gpiod_line_event_read((line_event.lines[i]),
	//						     &event));
	//			gpio_handler(line_event.lines[i], &event);
	//		}
	//	}

	sleep(10);

cleanup:
	//gpiod_line_release_bulk(&lines);
	gpiod_isr_release_bulk(isr);
	gpiod_chip_close(chip);

	//	int gpio_pin = 12;
	//	struct gpiod_line *line = gpiod_chip_get_line(chip, gpio_pin);
	//	if (!line) {
	//		perror("unable to get line");
	//		goto cleanup;
	//	}
	//
	//	struct gpiod_isr *isr = gpiod_isr_request_falling_edge_events(
	//		line, "interrupt_with_gpiod", gpio_handler);
	//	if (!isr) {
	//		perror("unable to request isr");
	//		goto cleanup;
	//	}
	//
	//	gpio_pin = 13;
	//	struct gpiod_line *line2 = gpiod_chip_get_line(chip, gpio_pin);
	//	if (!line2) {
	//		perror("unable to get line");
	//		goto cleanup;
	//	}
	//
	//	struct gpiod_isr *isr2 = gpiod_isr_request_both_edges_events(
	//		line2, "interrupt_with_gpiod", gpio_handler);
	//	if (!isr2) {
	//		perror("unable to request isr");
	//		goto cleanup;
	//	}
	//
	//	sleep(10);
	//
	//	gpiod_isr_release(isr);
	//	gpiod_isr_release(isr2);
	//
	//cleanup:
	//	gpiod_line_release(line);
	//	gpiod_line_release(line2);
	//	gpiod_chip_close(chip);

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
