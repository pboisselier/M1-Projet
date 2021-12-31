/**
 * @brief Simple wrapper for the libgpiod library that provides a way to have event interrupts
 * 
 * @file gpiod-isr.h
 * @copyright (c) Pierre Boisselier
 * @date 2021-12-30 
 *
 * @details 
 * This wrapper provides a way to have interrupts with the libgpiod library.
 * It is based on pthread and will start a thread that will keep monitoring a line (or set of lines)
 * and call a handling function that the user provided.  
 * 
 * ## 
 * 
 * @warning This wrapper uses pthread, do not forget to add `-pthread` when compiling!
 * @warning If your handler function is too slow some events can be discarded if they are happening during your function! 
 * 
 * @todo Check whether the gpiod_release function will cause problem for the user when he tries to re-request with the same pointer that he got from gpiod_chip_get_line.
 *
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
 * @return TODO
 * 
 * @todo Find a way to propagate errors and handle them
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
		if (gpiod_line_event_wait(isr->line, NULL) < 0) {
			///@todo Propagate error
			return NULL;
		}

		if (gpiod_line_event_read(isr->line, &event) < 0) {
			///@todo Propagate error
			return NULL;
		} else {
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
 * @return  TODO
 * 
 * @todo Find a way to propagate errors and handle them
 */
static void *_gpiod_event_watcher_bulk(void *_isr)
{
	const struct gpiod_isr_bulk *isr = (struct gpiod_isr_bulk *)_isr;

	(void)pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	struct gpiod_line_bulk event_bulk;
	struct gpiod_line_event event;

	for (;;) {
		/* Wait for events on all lines */
		gpiod_line_event_wait_bulk(isr->lines, NULL, &event_bulk);
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
 * @brief Release a previously registered ISR event.
 * @param isr GPIO ISR object.
 * @return 0 on success, -1 on failure. 
 */
int gpiod_isr_release(struct gpiod_isr *isr)
{
	if (!isr)
		return -1;

	gpiod_line_release(isr->line);
	if (pthread_cancel(isr->thread) != 0)
		return -1;

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

	gpiod_line_release_bulk(isr->lines);
	if (pthread_cancel(isr->thread) != 0)
		return -1;

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
 */
struct gpiod_isr *gpiod_isr_request_events(
	struct gpiod_line *line, const char *consumer, int event_type,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	if (!line || !handler) {
		errno = EINVAL;
		return NULL;
	}

	int request_err = 0;
	switch (event_type) {
	case GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE:
		request_err =
			gpiod_line_request_falling_edge_events(line, consumer);
		break;
	case GPIOD_LINE_REQUEST_EVENT_RISING_EDGE:
		request_err =
			gpiod_line_request_rising_edge_events(line, consumer);
		break;
	case GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES:
		request_err =
			gpiod_line_request_both_edges_events(line, consumer);
		break;
	default:
		errno = EINVAL;
		return NULL;
	}

	if (request_err)
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
 */
struct gpiod_isr_bulk *gpiod_isr_request_bulk_events(
	struct gpiod_line_bulk *bulk, const char *consumer, int event_type,
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *))
{
	if (!bulk || !handler) {
		errno = EINVAL;
		return NULL;
	}

	int request_err = 0;
	switch (event_type) {
	case GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE:
		request_err = gpiod_line_request_bulk_falling_edge_events(
			bulk, consumer);
		break;
	case GPIOD_LINE_REQUEST_EVENT_RISING_EDGE:
		request_err = gpiod_line_request_bulk_rising_edge_events(
			bulk, consumer);
		break;
	case GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES:
		request_err = gpiod_line_request_bulk_both_edges_events(
			bulk, consumer);
		break;
	default:
		errno = EINVAL;
		return NULL;
	}

	if (request_err)
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
	void (*isr)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_request_events(
		line, consumer, GPIOD_LINE_REQUEST_EVENT_RISING_EDGE, isr);
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
	void (*isr)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_request_bulk_events(
		bulk, consumer, GPIOD_LINE_REQUEST_EVENT_RISING_EDGE, isr);
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
	void (*isr)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_request_events(
		line, consumer, GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE, isr);
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
	void (*isr)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_request_bulk_events(
		bulk, consumer, GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE, isr);
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
	void (*isr)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_request_events(
		line, consumer, GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES, isr);
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
	void (*isr)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_request_bulk_events(
		bulk, consumer, GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES, isr);
}

#ifdef __cplusplus
}
#endif

#endif // GPIO_ISR_H