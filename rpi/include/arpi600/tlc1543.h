/**
 * @brief Library for the TLC1543 10-bit ADC (Mode 2)
 * 
 * @file tlc1543.h
 * @copyright (c) Pierre Boisselier
 * 
 * @details
 * This library allows one to use the TLC1543 10-bit ADC from Texas Instruments in their project.
 * The communication is done using a simple serial protocol.
 * Please refer to https://www.ti.com/lit/ds/symlink/tlc1543.pdf for more informations
 * 
 * @warning This requires the libgpiod library to be installed (libgpiod-devgpio)!
 * @warning Do not forget to compile with "-lgpiod" flag!
 * 
 * ARPI600 Implementation specific:
 *      - Set the REF jumper to either 5V or 3.3V
 *      - Pins EOC and CS are not connected
 */

#ifndef TLC1543_H
#define TLC1543_H

/* Default gpiochip device the TLC on the ArPi is using */
#ifndef TLC1543_GPIO_CHIP_DEV
#define TLC1543_GPIO_CHIP_DEV "/dev/gpiochip0"
#endif

/* Default TLC1543 pins on the GPIO (BCM ordering) */
#ifndef TLC1543_PIN_IOCLK
#define TLC1543_PIN_IOCLK 16
#endif
#ifndef TLC1543_PIN_ADDR
#define TLC1543_PIN_ADDR 20
#endif
#ifndef TLC1543_PIN_DATA
#define TLC1543_PIN_DATA 21
#endif

/* Sampling time of the TLC1543 in micro seconds (us) */
#define TLC1543_SAMPLING_TIME 1000

/* Returned error values */
#define TLC1543_SUCCESS 0
#define TLC1543_ERR -1
#define TLC1543_ERR_ARG -10
#define TLC1543_ERR_NOINIT -11
#define TLC1543_ERR_OPEN_CHIP -20
#define TLC1543_ERR_OPEN_LINE -21
#define TLC1543_ERR_WRITE -30
#define TLC1543_ERR_READ -31

#include <error.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <gpiod.h>

#ifdef __cplusplus
extern "C" {
#endif

// TODO: Simplify this? Maybe no need for all these as it's always to use the TLC chip and not a generic thing?

struct tlc1543 {
	struct gpiod_chip *chip;
	struct gpiod_line *ioclk;
	struct gpiod_line *addr;
	struct gpiod_line *data;
};

/**
 * @brief Initialize an access to the TLC1543 chip
 * 
 * @param tlc Allocated structure that will serve as the access
 * @param gpio_dev GPIO chip device path (e.g. "/dev/gpiochipX")
 * @param gpio_ioclk GPIO pin where the I/O clock is connected
 * @param gpio_addr GPIO pin where the address line is connected
 * @param gpio_data GPIO pin where the Data Out line is connected
 * @return 0 on success, negative value on error 
 */
int tlc1543_init_c_i_i_i(struct tlc1543 *tlc, const char *gpio_dev,
			 const int gpio_ioclk, const int gpio_addr,
			 const int gpio_data)
{
	if (!tlc)
		return TLC1543_ERR_ARG;

	tlc->chip = gpiod_chip_open(gpio_dev);
	if (!tlc->chip)
		return TLC1543_ERR_OPEN_CHIP;

	tlc->addr = gpiod_chip_get_line(tlc->chip, TLC1543_PIN_ADDR);
	if (!tlc->addr)
		return TLC1543_ERR_OPEN_LINE;

	tlc->data = gpiod_chip_get_line(tlc->chip, TLC1543_PIN_DATA);
	if (!tlc->data)
		return TLC1543_ERR_OPEN_LINE;

	tlc->ioclk = gpiod_chip_get_line(tlc->chip, TLC1543_PIN_IOCLK);
	if (!tlc->ioclk)
		return TLC1543_ERR_OPEN_LINE;

	return TLC1543_SUCCESS;
}

/**
 * @brief Initialize an access to the TLC1543 chip with default GPIO pins
 * 
 * @param tlc Allocated structure that will serve as the access
 * @param gpio_dev GPIO chip device path (e.g. "/dev/gpiochipX")
 * @return 0 on success, negative value on error 
 */
int tlc1543_init_c(struct tlc1543 *tlc, const char *gpio_dev)
{
	return tlc1543_init_c_i_i_i(tlc, gpio_dev, TLC1543_PIN_IOCLK,
				    TLC1543_PIN_ADDR, TLC1543_PIN_DATA);
}

/**
 * @brief Initialize an access to the TLC1543 chip with default GPIO pins and device path
 * 
 * @param tlc Allocated structure that will serve as the access
 * @return 0 on success, negative value on error 
 */
int tlc1543_init(struct tlc1543 *tlc)
{
	return tlc1543_init_c(tlc, TLC1543_GPIO_CHIP_DEV);
}

/**
 * @brief Delete and free an access to the TLC1543 chip
 * 
 * @param tlc Valid and initialized access to the TLC1543
 * @return 0 on success, negative value on error
 */
int tlc1543_delete(struct tlc1543 *tlc)
{
	if (!tlc || !tlc->chip)
		return TLC1543_ERR_NOINIT;

	gpiod_line_release(tlc->addr);
	gpiod_line_release(tlc->ioclk);
	gpiod_line_release(tlc->data);
	gpiod_chip_close(tlc->chip);

	return TLC1543_SUCCESS;
}

/**
 * @brief Acquire a sample from the ADC
 * 
 * @param tlc valid and initialized access to the TLC1543
 * @param channel which channel on the ADC to use (0 through 11)
 * @return negative value on error, otherwise the value acquired from the ADC 
 * @warning This function will call "sleep" as it needs to wait for the TLC1543 to sample
 */
int tlc1543_get_sample(struct tlc1543 *tlc, uint8_t channel)
{
	if (!tlc || !tlc->chip)
		return TLC1543_ERR_NOINIT;
	if (channel > 11)
		return TLC1543_ERR_ARG;

	if (gpiod_line_request_output(tlc->ioclk, "tlc1543", 0) < 0)
		return TLC1543_ERR_OPEN_LINE;
	if (gpiod_line_request_output(tlc->addr, "tlc1543", 0) < 0)
		return TLC1543_ERR_OPEN_LINE;
	if (gpiod_line_request_input(tlc->data, "tlc1543") < 0)
		return TLC1543_ERR_OPEN_LINE;

	/* Request a sample */
	for (short i = 0; i < 10; ++i) {
		/* First 4 clocks are used to send the address (channel) */
		if (i < 4) {
			/* Sending MSB first */
			if (gpiod_line_set_value(tlc->addr, (channel & 0x08)) <
			    0)
				return TLC1543_ERR_WRITE;
			channel <<= 1;
		}
		if (gpiod_line_set_value(tlc->ioclk, 1) < 0)
			return TLC1543_ERR_WRITE;
		if (gpiod_line_set_value(tlc->ioclk, 0) < 0)
			return TLC1543_ERR_WRITE;
	}

	/* Sleep for sample acquisition */
	usleep(TLC1543_SAMPLING_TIME);

	uint16_t sample = 0;
	int value = 0;

	/* Read sample */
	for (short i = 0; i < 10; ++i) {
		if (gpiod_line_set_value(tlc->ioclk, 1) < 0)
			return TLC1543_ERR_WRITE;

		value = gpiod_line_get_value(tlc->data);
		if (value < 0)
			return TLC1543_ERR_READ;

		sample |= (value & 0x01);
		sample <<= 1;

		if (gpiod_line_set_value(tlc->ioclk, 0) < 0)
			return TLC1543_ERR_WRITE;
	}

	return (int)sample;
}

/**
 * @brief Acquire a sample from the ADC but open and close access to the chip with default value 
 * 
 * @return int negative value on error, otherwise the value acquired from the ADC
 * @note This is a helper function that simplifies some code, however as it opens and close 
 *       everytime it is called, it is slower. 
 */
int tlc1543_get_sample_standalone(uint8_t channel)
{
	struct tlc1543 tlc;

	int value = tlc1543_init(&tlc);
	if (value < 0)
		return value;
	value = tlc1543_get_sample(&tlc, channel);
	tlc1543_delete(&tlc);

	return value;
}

#ifdef __cplusplus
}
#endif

#endif // TLC1543_H
