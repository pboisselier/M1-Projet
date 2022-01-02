/* 
 * Copyright © 2021 Pierre Boisselier
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
 * associated documentation files (the “Software”), to deal in the Software without restriction, including 
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies 
 * or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE 
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
*/

/**
 * @brief Simple wrapper for the libgpiod library that provides a way to have event interrupts
 * 
 * @file gpiod-isr.h
 * @copyright (c) Pierre Boisselier <pb@pboisselier.fr>
 * @date 2021-12-30 
 *
 * @details 
 * This wrapper provides a way to have interrupts with the libgpiod library.
 * It is based on pthread and will start a thread that will keep monitoring a line (or set of lines)
 * and call a handling function that the user provided.  
 * 
 * @warning This wrapper uses pthread, do not forget to add `-pthread` when compiling!
 * @warning If your handler function is too slow some events can be discarded if they are happening during your function! 
 *
 * ## Usage 
 * 
 * This wrapper was designed to feel similar in use to the actual library, instead of using `gpiod_line_request_event` you use `gpiod_isr_request_event`.
 * And when releasing you use `gpiod_isr_release` instead.
 *
 * In the following example we are going to listen for falling edge events on GPIO pin 12 until the user input a character which will stop the program.
 *
 * First thing to do is to prepare a function that will be our "handler", meaning it will be called when an event occurs.
 * The function needs to follow the following signature: `void handler(struct gpiod_line *, struct gpiod_line_event *)`.
 *
 * ```c
 * // Interrupt handler called when an interrupt on a GPIO pin happened.
 * // This prints the type of event, the line on which it happened and when it happened.
 * // The parameter line contains the GPIO line on which the event occured,
 * // the parameter event contains the time and type of event.
 * void gpio_handler(struct gpiod_line *line, struct gpiod_line_event *event)
 * {
 * 	printf("GPIO event (%d) on pin %s happened %lld.%.9ld seconds after boot!\n",
 * 	       event->event_type, gpiod_line_name(line),
 * 	       (long long)event->ts.tv_sec, event->ts.tv_nsec);
 * }
 * ```
 * 
 * We can then register the interrupt. 
 * 
 * ```c
 * // Open gpiochip0 
 * struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip0");
 * 
 * // We will use the GPIO pin 12 
 * const unsigned int gpio_pin = 12;
 * 
 * // Retrieve the GPIO line we want 
 * struct gpiod_line *line = gpiod_chip_get_line(chip, gpio_pin);
 * 
 * // Reserve the line for falling edge detection, also tells which
 * // function will act as the interrupt handler.
 * // In this case gpio_handler is our interrupt handler.
 * // It is important to check for the value of isr, if it is NULL an error occured.
 * struct gpiod_isr *isr = gpiod_isr_request_falling_edge_events(line, "gpiod_interrupts", gpio_handler);
 * ```
 * 
 * When you need to close or remove the interrupt you need to use `gpiod_isr_release` instead of `gpiod_line_release`, this function will also call `gpiod_line_release`.
 * 	
 * ```c
 * // Remove interrupt and release the line 
 * gpiod_isr_release(isr);
 * // Close the GPIO chip and free resources 
 * gpiod_isr_close(chip);
 * ```
 * 
 * This wrapper also provides a way to change the event type or the interrupt handler with functions beginning with `gpiod_isr_change`.
 * There is also support for `bulk` lines object.
 */

#ifndef GPIO_ISR_H
#define GPIO_ISR_H

#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <gpiod.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Structure holding ISR configuration for a single line.
 */
struct gpiod_isr {
	struct gpiod_line *line;
	///< GPIO line on which the event is registered
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *);
	///< Interrupt handler provided
	int event_type;
	///< Event type (rising, falling, both)
	pthread_t thread;
	///< Associated thread
};

/**
 * @brief Structure holding ISR configuration for bulk lines.
 */
struct gpiod_isr_bulk {
	struct gpiod_line_bulk *lines;
	///< GPIO lines on which events are registered
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *);
	///< Interrupt handler provided
	int event_type;
	///< Event type (rising, falling, both)
	pthread_t thread;
	///< Associated thread
};

/**
 * @brief Pthread routine that will watch events on a given line and call the interrupt handler.
 * @param _isr Pointer to a gpiod_isr structure.
 * @return Nothing.
 * @warning This thread will override any error happend while waiting for events.
 */
static void *_gpiod_event_watcher(void *_isr)
{
	const struct gpiod_isr *isr = (struct gpiod_isr *)_isr;

	/* Allow pthread_cancel to stop this thread at any time
         * This should be safe and pretty clean as we do not have anything to clean
         * in this this thread.
         * Unless gpiod_line_event_wait or read are not clean...
         */
	(void)pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	struct gpiod_line_event event;

	for (;;) {
		/* Wait for an event to happen */
		/* NOTE: This overrides error! */
		while (gpiod_line_event_wait(isr->line, NULL) != 1)
			;

		if (gpiod_line_event_read(isr->line, &event) == 0) {
			/* Call provided handler */
			/* WARNING: While in the handler the thread is not watching for other interrupts */
			isr->handler(isr->line, &event);
		}
	}

	return NULL;
}

/**
 * @brief Pthread routing that will watch events on a set of line.
 * @param _isr Pointer to a gpiod_isr_bulk structure.
 * @return  Nothing.
 * @warning This thread will override any error happend while waiting for events.
 */
static void *_gpiod_event_watcher_bulk(void *_isr)
{
	const struct gpiod_isr_bulk *isr = (struct gpiod_isr_bulk *)_isr;

	/* Allow pthread_cancel to stop this thread at any time
         * This should be safe and pretty clean as we do not have anything to clean
         * in this this thread.
         * Unless gpiod_line_event_wait or read are not clean...
         */
	(void)pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	struct gpiod_line_bulk event_bulk;
	struct gpiod_line_event event;

	for (;;) {
		/* Wait for events on all lines */
		while (gpiod_line_event_wait_bulk(isr->lines, NULL,
						  &event_bulk) != 1)
			;
		/* Call handler for every event on each line */
		for (unsigned int i = 0; i < event_bulk.num_lines; ++i) {
			if (gpiod_line_event_read(event_bulk.lines[i],
						  &event) == 0)
				isr->handler(event_bulk.lines[i], &event);
		}
	}

	return NULL;
}

/**
 * @brief Request the correct event for the line.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param event_type Event request type.
 * @return 0 on success, -1 on failure 
 * 
 * This is done this way to prevent someone from requesting something that is not an event, 
 * for instance a GPIOD_LINE_REQUEST_DIRECTION_INPUT.
 */
static int _gpiod_request_event(struct gpiod_line *line, const char *consumer,
				int event_type)
{
	switch (event_type) {
	case GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE:
		return gpiod_line_request_falling_edge_events(line, consumer);
	case GPIOD_LINE_REQUEST_EVENT_RISING_EDGE:
		return gpiod_line_request_rising_edge_events(line, consumer);
	case GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES:
		return gpiod_line_request_both_edges_events(line, consumer);
	default:
		errno = EINVAL;
		return -1;
	}
}

/**
 * @brief Request the correct event for a set of lines.
 * @param line GPIO bulk line object.
 * @param consumer Name of the consumer.
 * @param event_type Event request type.
 * @return 0 on success, -1 on failure 
 * 
 * This is done this way to prevent someone from requesting something that is not an event, 
 * for instance a GPIOD_LINE_REQUEST_DIRECTION_INPUT.
 */
static int _gpiod_request_bulk_event(struct gpiod_line_bulk *bulk,
				     const char *consumer, int event_type)
{
	switch (event_type) {
	case GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE:
		return gpiod_line_request_bulk_falling_edge_events(bulk,
								   consumer);
	case GPIOD_LINE_REQUEST_EVENT_RISING_EDGE:
		return gpiod_line_request_bulk_rising_edge_events(bulk,
								  consumer);
	case GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES:
		return gpiod_line_request_bulk_both_edges_events(bulk,
								 consumer);
	default:
		errno = EINVAL;
		return -1;
	}
}

/**
 * @brief Release a previously registered ISR event.
 * @param isr GPIO ISR object.
 * @return 0 on success, -1 on failure. 
 */
int gpiod_isr_release(struct gpiod_isr *isr)
{
	if (!isr)
		return -1;

	/* Wait for thread to be completely finished so it frees all resources */
	if (pthread_cancel(isr->thread) != 0)
		return -1;
	pthread_join(isr->thread, NULL);

	gpiod_line_release(isr->line);

	free(isr);
	return 0;
}

/**
 * @brief Release a previously registered ISR event.
 * @param isr GPIO ISR object.
 * @return 0 on success, -1 on failure.
 */
int gpiod_isr_release_bulk(struct gpiod_isr_bulk *isr)
{
	if (!isr)
		return -1;

	/* Wait for thread to be completely finished so it frees all resources */
	if (pthread_cancel(isr->thread) != 0)
		return -1;
	pthread_join(isr->thread, NULL);

	gpiod_line_release_bulk(isr->lines);
	free(isr);
	return 0;
}

/**
 * @brief Request event detection ISR on a single line.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param event_type Event type (type of request).
 * @param handler Interrupt handling function.
 * @return Pointer to the GPIO ISR handler or NULL on failure.
 * 
 * The parameter event_type can be:
 * - GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE
 * - GPIOD_LINE_REQUEST_EVENT_RISING_EDGE
 * - GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES
 */
struct gpiod_isr *gpiod_isr_request_events(
	struct gpiod_line *line, const char *consumer, const int event_type,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	if (!line || !handler) {
		errno = EINVAL;
		return NULL;
	}

	if (_gpiod_request_event(line, consumer, event_type) < 0)
		return NULL;

	struct gpiod_isr *isr = malloc(sizeof(*isr));
	if (!isr) {
		gpiod_line_release(line);
		return NULL;
	}

	isr->line = line;
	isr->handler = handler;
	isr->event_type = event_type;

	if (pthread_create(&isr->thread, NULL, _gpiod_event_watcher,
			   (void *)isr) != 0) {
		free(isr);
		gpiod_line_release(line);
		return NULL;
	}

	return isr;
}

/**
 * @brief Request event detection ISR on a set of lines.
 * @param bulk Set of GPIO lines to watch event on. 
 * @param consumer Name of the consumer.
 * @param event_type Event type (type of request).
 * @param handler Interrupt handling function.
 * @return Pointer to the GPIO ISR handler or NULL on failure.
 * 
 * The parameter event_type can be:
 * - GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE
 * - GPIOD_LINE_REQUEST_EVENT_RISING_EDGE
 * - GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES
 */
struct gpiod_isr_bulk *gpiod_isr_request_bulk_events(
	struct gpiod_line_bulk *bulk, const char *consumer,
	const int event_type,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	if (!bulk || !handler) {
		errno = EINVAL;
		return NULL;
	}

	if (_gpiod_request_bulk_event(bulk, consumer, event_type) < 0)
		return NULL;

	struct gpiod_isr_bulk *isr = malloc(sizeof(*isr));
	if (!isr) {
		gpiod_line_release_bulk(bulk);
		return NULL;
	}

	isr->lines = bulk;
	isr->handler = handler;
	isr->event_type = event_type;

	if (pthread_create(&isr->thread, NULL, _gpiod_event_watcher_bulk,
			   (void *)isr) != 0) {
		free(isr);
		gpiod_line_release_bulk(bulk);
		return NULL;
	}

	return isr;
}

/**
 * @brief Request rising edge event ISR on a single line.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param handler Interrupt handling function.
 * @return Pointer to the GPIO ISR handler or NULL on failure.
 */
struct gpiod_isr *gpiod_isr_request_rising_edge_events(
	struct gpiod_line *line, const char *consumer,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_request_events(
		line, consumer, GPIOD_LINE_REQUEST_EVENT_RISING_EDGE, handler);
}

/**
 * @brief Request rising edge event ISR on a set of lines.
 * @param bulk Set of GPIO lines to watch.
 * @param consumer Name of the consumer.
 * @param handler Interrupt handling function.
 * @return Pointer to the GPIO ISR handler or NULL on failure.
 */
struct gpiod_isr_bulk *gpiod_isr_request_bulk_rising_edge_events(
	struct gpiod_line_bulk *bulk, const char *consumer,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_request_bulk_events(
		bulk, consumer, GPIOD_LINE_REQUEST_EVENT_RISING_EDGE, handler);
}

/**
 * @brief Request falling edge event ISR on a single line.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param handler Interrupt handling function.
 * @return Pointer to the GPIO ISR handler or NULL on failure.
 */
struct gpiod_isr *gpiod_isr_request_falling_edge_events(
	struct gpiod_line *line, const char *consumer,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_request_events(
		line, consumer, GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE, handler);
}

/**
 * @brief Request falling edge event ISR on a set of lines.
 * @param bulk Set of GPIO lines to watch.
 * @param consumer Name of the consumer.
 * @param handler Interrupt handling function.
 * @return Pointer to the GPIO ISR handler or NULL on failure.
 */
struct gpiod_isr_bulk *gpiod_isr_request_bulk_falling_edge_events(
	struct gpiod_line_bulk *bulk, const char *consumer,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_request_bulk_events(
		bulk, consumer, GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE, handler);
}

/**
 * @brief Request both edge event ISR on a single line.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param handler Interrupt handling function.
 * @return Pointer to the GPIO ISR handler or NULL on failure.
 */
struct gpiod_isr *gpiod_isr_request_both_edges_events(
	struct gpiod_line *line, const char *consumer,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_request_events(
		line, consumer, GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES, handler);
}

/**
 * @brief Request both edge event ISR on a set of lines.
 * @param bulk Set of GPIO lines to watch.
 * @param consumer Name of the consumer.
 * @param handler Interrupt handling function.
 * @return Pointer to the GPIO ISR handler or NULL on failure.
 */
struct gpiod_isr_bulk *gpiod_isr_request_bulk_both_edges_events(
	struct gpiod_line_bulk *bulk, const char *consumer,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_request_bulk_events(
		bulk, consumer, GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES, handler);
}

/**
 * @brief Change an existing GPIOD ISR, this can change the event or the handler.
 * @param isr Pointer to an existing GPIOD_ISR.
 * @param event_type Event type (type of request), or -1 to keep the same.
 * @param handler Interrupt handling function, or NULL to keep the same.
 * @return 0 on success, -1 on failure.
 * 
 * The parameter event_type can be:
 * - GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE
 * - GPIOD_LINE_REQUEST_EVENT_RISING_EDGE
 * - GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES
 */
int gpiod_isr_change_event(struct gpiod_isr *isr, const int event_type,
			   void (*handler)(struct gpiod_line *,
					   struct gpiod_line_event *))
{
	if (!isr) {
		errno = EINVAL;
		return -1;
	}

	/* Do nothing if there is nothing to change. */
	if ((!handler && (event_type == -1 || isr->event_type == event_type)) ||
	    (handler == isr->handler && isr->event_type == event_type))
		return 0;

	/* Terminate watcher */
	if (pthread_cancel(isr->thread) != 0) {
		return -1;
	}
	pthread_join(isr->thread, NULL);

	/* Change event */
	if (event_type != -1 && isr->event_type != event_type) {
		gpiod_line_release(isr->line);
		if (_gpiod_request_event(isr->line, gpiod_line_name(isr->line),
					 event_type) < 0) {
			return -1;
		}
		isr->event_type = event_type;
	}

	/* Change handler */
	if (handler && isr->handler != handler) {
		isr->handler = handler;
	}

	if (pthread_create(&isr->thread, NULL, _gpiod_event_watcher,
			   (void *)isr) != 0) {
		gpiod_line_release(isr->line);
		return -1;
	}

	return 0;
}

/**
 * @brief Change an existing gpiod isr to rising edge with a new handler (or not).
 * @param isr pointer to an existing gpiod_isr.
 * @param handler interrupt handling function, or null to keep the same.
 * @return 0 on success, -1 on failure.
 */
int gpiod_isr_change_rising_edge_events(
	struct gpiod_isr *isr,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_change_event(isr, GPIOD_LINE_REQUEST_EVENT_RISING_EDGE,
				      handler);
}

/**
 * @brief Change an existing gpiod isr to falling edge with a new handler (or not).
 * @param isr pointer to an existing gpiod_isr.
 * @param handler interrupt handling function, or null to keep the same.
 * @return 0 on success, -1 on failure.
 */
int gpiod_isr_change_falling_edge_events(
	struct gpiod_isr *isr,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_change_event(
		isr, GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE, handler);
}

/**
 * @brief Change an existing gpiod isr to both edges with a new handler (or not).
 * @param isr pointer to an existing gpiod_isr.
 * @param handler interrupt handling function, or null to keep the same.
 * @return 0 on success, -1 on failure.
 */
int gpiod_isr_change_both_edges_events(
	struct gpiod_isr *isr,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_change_event(isr, GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES,
				      handler);
}

/**
 * @brief Change an existing bulk GPIOD ISR, this can change the event or the handler.
 * @param isr Pointer to an existing gpiod_isr_bulk object.
 * @param event_type Event type (type of request), or -1 to keep the same.
 * @param handler Interrupt handling function, or NULL to keep the same.
 * @return 0 on success, -1 on failure.
 * 
 * The parameter event_type can be:
 * - GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE
 * - GPIOD_LINE_REQUEST_EVENT_RISING_EDGE
 * - GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES
 */
int gpiod_isr_change_bulk_event(struct gpiod_isr_bulk *isr,
				const int event_type,
				void (*handler)(struct gpiod_line *,
						struct gpiod_line_event *))
{
	if (!isr) {
		errno = EINVAL;
		return -1;
	}

	/* Do nothing if there is nothing to change. */
	if ((!handler && (event_type == -1 || isr->event_type == event_type)) ||
	    (handler == isr->handler && isr->event_type == event_type))
		return 0;

	/* Terminate watcher */
	if (pthread_cancel(isr->thread) != 0) {
		return -1;
	}
	pthread_join(isr->thread, NULL);

	/* Change event */
	if (event_type != -1 && isr->event_type != event_type) {
		gpiod_line_release_bulk(isr->lines);
		if (_gpiod_request_bulk_event(
			    isr->lines, gpiod_line_name(isr->lines->lines[0]),
			    event_type) < 0) {
			return -1;
		}
		isr->event_type = event_type;
	}

	/* Change handler */
	if (handler && isr->handler != handler) {
		isr->handler = handler;
	}

	if (pthread_create(&isr->thread, NULL, _gpiod_event_watcher_bulk,
			   (void *)isr) != 0) {
		gpiod_line_release_bulk(isr->lines);
		return -1;
	}

	return 0;
}

/**
 * @brief Change an existing bulk gpiod isr to rising edge with a new handler (or not).
 * @param isr pointer to an existing gpiod_isr_bulk.
 * @param handler interrupt handling function, or null to keep the same.
 * @return 0 on success, -1 on failure.
 */
int gpiod_isr_change_bulk_rising_edge_events(
	struct gpiod_isr_bulk *isr,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_change_bulk_event(
		isr, GPIOD_LINE_REQUEST_EVENT_RISING_EDGE, handler);
}

/**
 * @brief Change an existing bulk gpiod isr to falling edge with a new handler (or not).
 * @param isr pointer to an existing gpiod_isr_bulk.
 * @param handler interrupt handling function, or null to keep the same.
 * @return 0 on success, -1 on failure.
 */
int gpiod_isr_change_bulk_falling_edge_events(
	struct gpiod_isr_bulk *isr,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_change_bulk_event(
		isr, GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE, handler);
}

/**
 * @brief Change an existing bulk gpiod isr to both edges with a new handler (or not).
 * @param isr pointer to an existing gpiod_isr_bulk.
 * @param handler interrupt handling function, or null to keep the same.
 * @return 0 on success, -1 on failure.
 */
int gpiod_isr_change_bulk_both_edges_events(
	struct gpiod_isr_bulk *isr,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_change_bulk_event(
		isr, GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES, handler);
}

#ifdef __cplusplus
}
#endif

#endif // GPIO_ISR_H