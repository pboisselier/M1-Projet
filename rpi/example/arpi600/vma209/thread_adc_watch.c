/**
 * @brief This examples uses the ArPi600 with the Vellmann VMA209 shield. It will continuously read the value of the potentionmeter and flash the LEDs when a threshold.
 * @example thread_adc_watch.c
 * @date 2022-01-19
 * @copyright (c) Pierre Boisselier
 * 
 * @details 
 * 
 * @warning This uses libpthread and libgpiod so make sure to compile with `-lgpiod` and `-pthread` flags.
 * @warning On the ArPi600 you need to make sure to set the A0 Jumper to T_A0, this will ensure that the potentiometer is tied to the 
 * ADC and not to the GPIO.
 * 
 * ### Setup
 * 
 * - Plug the ArPi600 on the Raspberry Pi GPIO header, then plug the VMA209 on the arduino adapter on the ArPi600.
 * - Make sure to remove the Buzzer from the VMA209 as there is a slight incompatibility with the Raspberry Pi, see note below.
 * - Ensure that the A0 jumper is set on the T_A0 input.
 * 
 * @note The VMA209 is meant to work on 5V logic (as any Arduino shield), however it uses pull-ups for everything. This means that the 3.3V logic the Raspberry Pi uses will not be able to fully switch off the LEDs or the Buzzer for instance.
 * 
 * ### Compilation
 * 
 * ```sh
 * # The -I../../include path is relative to the folder where this file is.
 * gcc -Wall -g -I../../include thread_adc_watch.c -lgpiod -pthread -o thread_adc_watch.out
 * ``` 
 * 
 * ### Run
 * 
 * The program will continuously run until the user inputs the letter 'q' and press return.
 * The value from the ADC will be continuously displayed for reference.
 * You can also start the example with a custom threshold, like so: 
 * `./thread_adc_watch.out 100`
 * DO NOT QUIT WITH SIGINT (Ctrl+C), this may cause the ADC to bug and you will not be able to read correct values from it until you restart the Raspberry Pi.
 */

#include <arpi600/tlc1543.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <gpiod.h>
#include <stdbool.h>

/* GPIO Lines where the LEDs are connected (BCM ordering). */
static const unsigned int gpio_leds[4] = { 11, 9, 10, 8 };
/* Default ADC channel for the potentiometer on the VMA209. */
static const int adc_channel = 0;
/* All LEDs off, HIGH because the VMA209 uses pull-ups. */
static const int leds_off[4] = { 1, 1, 1, 1 };
/* All LEDs on, LOW because the VMA209 uses pull-ups. */
static const int leds_on[4] = { 0, 0, 0, 0 };
/* Flashing tempo. */
static const int led_tempo = 1;
/* Threshold for the potentiometer. */
static int adc_threshold = 512;

/* Shared boolean between threads, beware, this should be atomic for this program to be safer, however this is fine as an example. */
static bool threshold_triggered = false;

/* Mutex used to quit, this will stop infinite loop threads. 
 * This is done like this to show how you can use mutex for this
 * purspose. 
 */
static pthread_mutex_t flag_quit = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Function that will flash the LEDs when the value of the ADC is over the threshold.
 * @param _gpio_leds gpiod_line_bulk object already opened for the LEDs.
 * @return Nothing.
 */
void *flash_leds_thread(void *_gpio_leds)
{
	/* Convet the void pointer. */
	struct gpiod_line_bulk *gpio_leds =
		(struct gpiod_line_bulk *)_gpio_leds;

	/* This will quit if we can get the lock, meaning the user wants to quit. */
	while (pthread_mutex_trylock(&flag_quit) != 0) {
		/* Do nothing until over the threshold. */
		if (!threshold_triggered) {
			continue;
		}

		/* Flash the LEDs. */
		gpiod_line_set_value_bulk(gpio_leds, leds_on);
		sleep(led_tempo);
		gpiod_line_set_value_bulk(gpio_leds, leds_off);
		sleep(led_tempo);
	}

	/* Unlock the mutext used to quit. */
	pthread_mutex_unlock(&flag_quit);

	return NULL;
}

/**
 * @brief Function that will keep reading the value of the potentiometer from the ADC and trigger the alarm.
 * @param _tlc Valid pointer to a tlc1543 structure which was initialized.
 * @return Nothing.
 */
void *adc_watcher(void *_tlc)
{
	/* Convert the void pointer. */
	struct tlc1543 *tlc = (struct tlc1543 *)_tlc;
	/* ADC Value that was read. Goes from 0 to 1023. */
	int value = 0;

	/* This will quit if we can get the lock, meaning the user wants to quit. */
	while (pthread_mutex_trylock(&flag_quit) != 0) {
		/* Get the value of the potentiometer. */
		value = tlc1543_get_sample(tlc, adc_channel);
		/* If unable to read the value, try again. */
		if (value < 0) {
			continue;
		}
		/* Display the value. */
		printf("ADC: %d\n", value);
		/* If the value is over the threshold, trigger the alarm. */
		if (value > adc_threshold) {
			/* NOTE: This should be relatively safe to do. 
                         * But would be better if explicitly atomic. */
			threshold_triggered = true;
		} else {
			threshold_triggered = false;
		}
	}

	/* Unlock the mutex used to quit the main loop. */
	pthread_mutex_unlock(&flag_quit);

	return NULL;
}

void usage(char *cmd)
{
	fprintf(stderr,
		"Usage: %s [threshold]\n\tThreshold must be between 0 and 1023, default = 512.\n",
		cmd);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	/* Change the threshold if there is an argument was provided. */
	if (argc > 1) {
		adc_threshold = atoi(argv[1]);
	}

	/* Initialize the ADC, and take exclusive ownership. */
	struct tlc1543 tlc;
	tlc1543_init(&tlc, TLC1543_OPT_EXCLUSIVE);

	/* Open the GPIO chip. */
	struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip0");
	/* Get the lines where the LEDs are connected to. */
	struct gpiod_line_bulk leds;
	gpiod_chip_get_lines(chip, gpio_leds, 4, &leds);
	/* Reserve LEDs lines and set it as output, OFF by default. */
	gpiod_line_request_bulk_output(&leds, "ADC Watch", leds_off);

	/* Threads id. */
	pthread_t watcher;
	pthread_t flasher;

	/* Lock the flag which is used to quit the threads. */
	pthread_mutex_lock(&flag_quit);

	/* Start the ADC watch thread. */
	if (pthread_create(&watcher, NULL, adc_watcher, (void *)&tlc) < 0) {
		perror("unable to start adc watcher!");
		return EXIT_FAILURE;
	}

	/* Start the LED flasher thread. */
	if (pthread_create(&flasher, NULL, flash_leds_thread, (void *)&leds) <
	    0) {
		perror("unable to start flasher!");
		return EXIT_FAILURE;
	}

	/* Do nothing until the character 'q' is pressed. */
	while (getchar() != 'q')
		;

	/* Unlock the flag which causes threads to quit. */
	pthread_mutex_unlock(&flag_quit);

	/* Join threads at the end to clean resources. */
	pthread_join(watcher, NULL);
	pthread_join(flasher, NULL);

	/* Release all acquired resources. */
	tlc1543_delete(&tlc);
	gpiod_line_release_bulk(&leds);
	gpiod_chip_close(chip);

	return EXIT_SUCCESS;
}