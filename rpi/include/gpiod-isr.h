/**
 * @brief Simple wrapper for the libgpiod library that provides a way to have event interrupts
 * 
 * @file gpiod-isr.h
 * @copyright (c) Pierre Boisselier
 * @date 2021-12-30 
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

struct gpiod_isr {
	struct gpiod_line *line;
	void (*handler)(struct gpiod_line *, struct gpiod_line_event *);
	int event_type;
	pthread_t thread;
};

int gpiod_isr_remove_event(struct gpiod_isr *isr)
{
	if (!isr)
		return -1;

	pthread_t thread = isr->thread;
	free(isr);

	if (pthread_cancel(thread) != 0)
		return -1;

	return 0;
}

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
			return;
		}

		if (gpiod_line_event_read(isr->line, &event) < 0) {
			///@todo Propagate error
			return;
		} else {
			/* Call provided handler */
			isr->handler(isr->line, &event);
		}
	}

	return NULL;
}

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
		return NULL;
	}

	isr->line = line;
	isr->handler = handler;
	isr->event_type = event_type;

	if (pthread_create(&isr->thread, NULL, _gpiod_event_watcher,
			   (void *)isr) != 0)
		goto cleanup;

	return isr;

cleanup:
	free(isr);
	return NULL;
}

struct gpiod_isr *gpiod_isr_request_rising_edge_events(
	struct gpiod_line *line, const char *consumer,
	void (*isr)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_request_events(
		line, consumer, GPIOD_LINE_REQUEST_EVENT_RISING_EDGE, isr);
}

struct gpiod_isr *gpiod_isr_request_falling_edge_events(
	struct gpiod_line *line, const char *consumer,
	void (*isr)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_request_events(
		line, consumer, GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE, isr);
}

struct gpiod_isr *gpiod_isr_request_both_edges_events(
	struct gpiod_line *line, const char *consumer,
	void (*isr)(struct gpiod_line *, struct gpiod_line_event *))
{
	return gpiod_isr_request_events(
		line, consumer, GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES, isr);
}

#ifdef __cplusplus
}
#endif

#endif // GPIO_ISR_H