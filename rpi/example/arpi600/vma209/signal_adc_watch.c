/**
 * @brief This examples uses the ArPi600 with the Vellmann VMA209 shield. It will continuously read the value of the potentionmeter and flash the LEDs when a threshold.
 * @example signal_adc_watch.c
 * @date 2022-01-20
 * @copyright (c) Pierre Boisselier
 * 
 * @details 
 * 
 * @warning This uses libgpiod so make sure to compile with the `-lgpiod` flag.
 * @warning On the ArPi600 you need to make sure to set the A0 Jumper to T_A0, this will ensure that the potentiometer is tied to the 
 * ADC and not to the GPIO.
 * 
 * ### Setup
 * 
 * - Plug the ArPi600 on the Raspberry Pi GPIO header, then plug the VMA209 on the arduino adapter on the ArPi600.
 * - Make sure to remove the Buzzer from the VMA209 as there is a slight incompatibility with the Raspberry Pi, see note below.
 * - Ensure that the A0 jumper is set on the T_A0 input.
 * - Ensure that the SPI interface is disabled in raspi-config.
 * 
 * @note The VMA209 is meant to work on 5V logic (as any Arduino shield), however it uses pull-ups for everything. This means that the 3.3V logic the Raspberry Pi uses will not be able to fully switch off the LEDs or the Buzzer for instance.
 * 
 * ### Compilation
 * 
 * ```sh
 * # The -I../../include path is relative to the folder where this file is.
 * gcc -Wall -g -I../../include thread_adc_watch.c -lgpiod -o signal_adc_watch.out
 * ``` 
 * 
 * ### Run
 * 
 * The program will continuously run until the user inputs the letter 'q' and press return.
 * The value from the ADC will be continuously displayed for reference.
 * You can also start the example with a custom threshold, like so: 
 * `./signal_adc_watch.out 100`
 * DO NOT QUIT WITH SIGINT (Ctrl+C), this may cause the ADC to bug and you will not be able to read correct values from it until you restart the Raspberry Pi.
 * 
 * ### Troubleshooting
 * 
 * If the LEDs do not blink, this is probably because you enabled the SPI interface on the RaspberryPi, this interferes with the GPIO pins the LEDs are connected to.
 */

#include <arpi600/tlc1543.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <gpiod.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

/* ADC Channel where the potentiometer is connected. */
static const int adc_channel = 0;
/* LEDs flash tempo. */
static const int led_flash_tempo = 1;

/* GPIO Lines where the LEDs are connected (BCM ordering). */
static const unsigned int gpio_leds[4] = { 11, 9, 10, 8 };
/* All LEDs off, HIGH because the VMA209 uses pull-ups. */
static const int leds_off[4] = { 1, 1, 1, 1 };
/* All LEDs on, LOW because the VMA209 uses pull-ups. */
static const int leds_on[4] = { 0, 0, 0, 0 };

/* Atomic variables for use in signal handler. */
static sig_atomic_t led_flash = 0;
static sig_atomic_t led_flasher_run = 1;
static sig_atomic_t adc_watcher_run = 1;

/**
 * @brief Signal handler for the LED flasher process.
 * @param signo Signal number.
 */
void proc_led_flasher_sighandler(int signo)
{
	/* SIGUSR2  = STOP */
	/* SIGUSR1  = START */
	/* SIGTERM = QUIT */

	printf("LED Flasher: received %d\n", signo);
	switch (signo) {
	case SIGUSR1:
		led_flash = 1;
		break;
	case SIGUSR2:
		led_flash = 0;
		break;
	case SIGTERM:
		led_flasher_run = 0;
		break;
	}
}

/**
 * @brief LED flasher process.
 */
void proc_led_flasher(void)
{
	/* Register all signals. */
	struct sigaction sig;
	sig.sa_flags = 0;
	sig.sa_handler = proc_led_flasher_sighandler;
	sigemptyset(&sig.sa_mask);
	sigaction(SIGUSR1, &sig, NULL);
	sigaction(SIGUSR2, &sig, NULL);
	sigaction(SIGTERM, &sig, NULL);

	/* Open GPIO chip and request lines. */
	struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip0");
	struct gpiod_line_bulk lines;
	gpiod_chip_get_lines(chip, gpio_leds, 4, &lines);
	if (gpiod_line_request_bulk_output(&lines, "ADC Watcher)", leds_off) <
	    0)
		perror("unable to reserve lines (check if SPI interface is disabled)");

	while (led_flasher_run) {
		if (!led_flash) {
			/* Wait for a signal, this keeps CPU utilization low. */
			pause();
		}

		/* Flash the LEDs. */
		gpiod_line_set_value_bulk(&lines, leds_on);
		sleep(led_flash_tempo);
		gpiod_line_set_value_bulk(&lines, leds_off);
		sleep(led_flash_tempo);
	}

	/* Free resources. */
	gpiod_line_release_bulk(&lines);
	gpiod_chip_close(chip);
}

/**
 * @brief Signal handler for the ADC watcher. 
 * @param signo Signal number.
 */
void proc_adc_watcher_sighandler(int signo)
{
	printf("ADC Watcher: received %d\n", signo);
	if (signo == SIGTERM) {
		adc_watcher_run = 0;
	}
}

/**
 * @brief ADC watcher process.
 * @param flasher_pid Flasher process PID.
 * @param adc_threshold Threshold for triggering the alarm (between 0 and 1023).
 */
void proc_adc_watcher(const pid_t flasher_pid, int adc_threshold)
{
	/* Register signal. */
	struct sigaction sig;
	sig.sa_flags = 0;
	sig.sa_handler = proc_adc_watcher_sighandler;
	sigemptyset(&sig.sa_mask);
	sigaction(SIGTERM, &sig, NULL);

	bool threshold_triggered = false;
	int value = 0;

	/* Initialize the ADC and take full ownership. */
	struct tlc1543 tlc;
	tlc1543_init(&tlc, TLC1543_OPT_EXCLUSIVE);

	while (adc_watcher_run) {
		/* Read the potentiometer's value. */
		value = tlc1543_get_sample(&tlc, adc_channel);
		/* Try again on error. */
		if (value < 0) {
			continue;
		}

		printf("ADC Read: %d\n", value);

		/* Trigger alarm or untrigger alarm depending on the value. */
		if (value > adc_threshold && !threshold_triggered) {
			threshold_triggered = true;
			printf("Threshold triggered!\n");
			/* Send signal to flasher process to start flashing. */
			kill(flasher_pid, SIGUSR1);
		} else if (value <= adc_threshold && threshold_triggered) {
			threshold_triggered = false;
			printf("Threshold un-triggered!\n");
			/* Send signal again to flasher process to stop flashing. */
			kill(flasher_pid, SIGUSR2);
		}
	}

	/* Free resources. */
	tlc1543_delete(&tlc);
}

/*
 * You can specify the threshold when launching the program like so:
 * ./signal_adc_watch.out 1000
 */
int main(int argc, char **argv)
{
	/* Default threshold. */
	int adc_threshold = 512;
	/* Change threshold if there is an argument. */
	if (argc > 1) {
		adc_threshold = atoi(argv[1]);
	}

	/* Launching two separate processes for watching the ADC
         * and for flashing the LEDs. */
	pid_t flasher_pid = fork();
	if (!flasher_pid) {
		proc_led_flasher();
		return EXIT_SUCCESS;
	}
	pid_t watcher_pid = fork();
	if (!watcher_pid) {
		proc_adc_watcher(flasher_pid, adc_threshold);
		return EXIT_SUCCESS;
	}

	/* Wait for the user to press 'q' to stop the program. */
	while (getchar() != 'q')
		;

	/* Terminate the two other processes. */
	printf("Killing Watcher PID=%d\nKilling Flasher PID=%d\n", watcher_pid,
	       flasher_pid);
	kill(watcher_pid, SIGTERM);
	wait(NULL);
	kill(flasher_pid, SIGTERM);
	wait(NULL);

	return EXIT_SUCCESS;
}
