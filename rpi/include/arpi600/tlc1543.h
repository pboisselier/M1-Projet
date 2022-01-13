/**
 * @brief Library for the TLC1543 10-bit ADC (Mode 2) from Texas Instruments.
 * 
 * @file tlc1543.h
 * @ingroup ArPi600
 * @copyright (c) Pierre Boisselier
 * @date 2021-12-17
 * 
 * @details
 * This library allows one to use the TLC1543 10-bit ADC from Texas Instruments in their project.  
 * The communication is done using a simple serial protocol.  
 * Please refer to https://www.ti.com/lit/ds/symlink/tlc1543.pdf for more informations
 * 
 * ## Channels
 * 
 * There are 10 channels + 3 special channels used for self-test:
 * - Channel 11: @f$ \frac{Vref_{+} - Vref_{-}}{2} @f$ should give 512
 * - Channel 12: @f$ Vref_{-} @f$ should give 0
 * - Channel 13: @f$ Vref_{+} @f$ should give 1023
 * 
 * @note Readings from this ADC vary from 0 to 1023 (10-bit precision).
 * 
 * ## Usage
 * 
 * Basic usage is:
 * ```c
 * // Initialize 
 * struct tlc1543 *tlc;
 * tlc1543_init(&tlc, options);
 * 
 * // Read ADC channel
 * int value = tlc1543_get_sample(&tlc, channel);
 * 
 * // Free access
 * lc1543_delete(&tlc);
 * ``` 
 * 
 * ### Optional flags
 * 
 * Two optional flags are available for the function @ref tlc1543_init, 
 * 	- @ref TLC1543_OPT_WAIT, will wait that all GPIO pins are unused before using them
 * 	- @ref TLC1543_OPT_EXCLUSIVE, will take exclusive control of the GPIO pins for its lifetime (until @ref tlc1543_delete)
 * 
 * @note Using both flags will wait in the @ref tlc1543_init function until all GPIO pins are unused. 
 * 
 * ## Compilation 
 * 
 * This library requires the **libgpiod** library, you need to install it first using your favourite package manager.
 * If you are on a Debian based system you can use the `libgpiod-dev` package.
 * 
 * ```sh
 * cc test.c -I./rpi/include -Wall -lgpiod -o test
 * ```
 * 
 * @warning Do not forget to compile with "-lgpiod" flag!
 * 
  * 
 * ## ArPi600 Implementation specifics
 *      - Set the REF jumper to either 5V or 3.3V
 *      - Pins EOC and CS are not connected
 */

#ifndef TLC1543_H
#define TLC1543_H

/** 
 * @name GPIO pins used on the ArPi600 (BCM ordering)
 * @{
 */

#ifndef TLC1543_GPIO_CHIP_DEV
/** @brief Default gpiochip device the TLC on the ArPi is using */
#define TLC1543_GPIO_CHIP_DEV "/dev/gpiochip0"
#endif

#ifndef TLC1543_PIN_IOCLK
/** @brief GPIO pin where the I/O CLK pin is connected */
#define TLC1543_PIN_IOCLK 16
#endif
#ifndef TLC1543_PIN_ADDR
/** @brief GPIO pin where the ADDR pin is connected */
#define TLC1543_PIN_ADDR 20
#endif
#ifndef TLC1543_PIN_DATA
/** @brief GPIO pin where the DATA_OUT pin is connected */
#define TLC1543_PIN_DATA 21
#endif

/**
 * @}
 * @name Optional flags the user can use
 * @{
 */

/** @brief Flag, will wait for a GPIO line to be free */
#define TLC1543_OPT_WAIT 0x01
/** @brief Flag, will take exclusive control of GPIO lines for the TLC1543 */
#define TLC1543_OPT_EXCLUSIVE 0x02

/**
 * @}
 * @name Error defines returned by functions 
 * @{
 */

/** @brief No error */
#define TLC1543_SUCCESS 0
/** @brief Generic error */
#define TLC1543_ERR -1
/** @brief Bad argument provided to function */
#define TLC1543_ERR_ARG -10
/** @brief TLC1543 not initialized */
#define TLC1543_ERR_NOINIT -11
/** @brief Cannot open the requested GPIO chip */
#define TLC1543_ERR_OPEN_CHIP -20
/** @brief Cannot open the requested GPIO line (pin in used by something else) */
#define TLC1543_ERR_OPEN_LINE -21
/** @brief Cannot set a GPIO pin to a specific value */
#define TLC1543_ERR_WRITE -30
/** @brief Cannot read a GPIO pin value */
#define TLC1543_ERR_READ -31

/**
 * @} 
 */

/** @brief Sampling time of the TLC1543 in micro seconds (us) */
#define TLC1543_SAMPLING_TIME 21

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

/**
 * @brief Access to the TLC1543
 */
struct tlc1543 {
	struct gpiod_chip *chip; ///< GPIO chip used
	struct gpiod_line *ioclk; ///< GPIO line for IOCLK
	struct gpiod_line *addr; ///< GPIO line for ADDR
	struct gpiod_line *data; ///< GPIO line for DATA_OUT
	int options; ///< Optional flags
};

/**
 * @brief Request access to GPIO lines
 * 
 * @param tlc Access to the TLC1543
 * @param options Optional flags
 * @return 0 on success, negative on error 
 */
static int tlc1543_request_lines(struct tlc1543 *tlc, int options)
{
	/* If option WAIT is used, block until all lines can be used */
	if (options & TLC1543_OPT_WAIT)
		while (!gpiod_line_is_free(tlc->addr) &&
		       !gpiod_line_is_free(tlc->data) &&
		       !gpiod_line_is_free(tlc->ioclk))
			;

	if (gpiod_line_request_output(tlc->ioclk, "tlc1543", 0) < 0)
		return TLC1543_ERR_OPEN_LINE;
	if (gpiod_line_request_output(tlc->addr, "tlc1543", 0) < 0)
		return TLC1543_ERR_OPEN_LINE;
	if (gpiod_line_request_input(tlc->data, "tlc1543") < 0)
		return TLC1543_ERR_OPEN_LINE;

	return TLC1543_SUCCESS;
}

/**
 * @brief Initialize an access to the TLC1543 chip
 * 
 * @param tlc Allocated structure that will serve as the access
 * @param gpio_dev GPIO chip device path (e.g. "/dev/gpiochipX")
 * @param gpio_ioclk GPIO pin where the I/O clock is connected
 * @param gpio_addr GPIO pin where the address line is connected
 * @param gpio_data GPIO pin where the Data Out line is connected
 * @param options Optional flags (e.g. TLC1543_OPT_WAIT), use 0 if unsure
 * @return 0 on success, negative value on error 
 */
int tlc1543_init_c_i_i_i(struct tlc1543 *tlc, const char *gpio_dev,
			 const int gpio_ioclk, const int gpio_addr,
			 const int gpio_data, int options)
{
	if (!tlc)
		return TLC1543_ERR_ARG;

	tlc->chip = gpiod_chip_open(gpio_dev);
	if (!tlc->chip)
		return TLC1543_ERR_OPEN_CHIP;

	tlc->addr = gpiod_chip_get_line(tlc->chip, gpio_addr);
	if (!tlc->addr)
		return TLC1543_ERR_OPEN_LINE;
	tlc->data = gpiod_chip_get_line(tlc->chip, gpio_data);
	if (!tlc->data)
		return TLC1543_ERR_OPEN_LINE;
	tlc->ioclk = gpiod_chip_get_line(tlc->chip, gpio_ioclk);
	if (!tlc->ioclk)
		return TLC1543_ERR_OPEN_LINE;

	tlc->options = options;

	if (options & TLC1543_OPT_EXCLUSIVE)
		tlc1543_request_lines(tlc, options);

	return TLC1543_SUCCESS;
}

/**
 * @brief Initialize an access to the TLC1543 chip with default GPIO pins
 * 
 * @param tlc Allocated structure that will serve as the access
 * @param gpio_dev GPIO chip device path (e.g. "/dev/gpiochipX")
 * @param options Optional flags (e.g. TLC1543_OPT_WAIT), use 0 if unsure
 * @return 0 on success, negative value on error 
 */
int tlc1543_init_c(struct tlc1543 *tlc, const char *gpio_dev, int options)
{
	return tlc1543_init_c_i_i_i(tlc, gpio_dev, TLC1543_PIN_IOCLK,
				    TLC1543_PIN_ADDR, TLC1543_PIN_DATA,
				    options);
}

/**
 * @brief Initialize an access to the TLC1543 chip with default GPIO pins and device path
 * 
 * @param tlc Allocated structure that will serve as the access
 * @param options Optional flags (e.g. TLC1543_OPT_WAIT), use 0 if unsure
 * @return 0 on success, negative value on error 
 */
int tlc1543_init(struct tlc1543 *tlc, int options)
{
	return tlc1543_init_c(tlc, TLC1543_GPIO_CHIP_DEV, options);
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
	if (channel > 13)
		return TLC1543_ERR_ARG;

	/* If option EXCLUSIVE is used there is no need to request lines */
	if (!(tlc->options & TLC1543_OPT_EXCLUSIVE))
		if (tlc1543_request_lines(tlc, tlc->options) < 0)
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

		sample <<= 1;
		sample |= (value & 0x01);

		if (gpiod_line_set_value(tlc->ioclk, 0) < 0)
			return TLC1543_ERR_WRITE;
	}

	/* If option EXCLUSIVE is used there is no need to release lines */
	if (!(tlc->options & TLC1543_OPT_EXCLUSIVE)) {
		gpiod_line_release(tlc->addr);
		gpiod_line_release(tlc->ioclk);
		gpiod_line_release(tlc->data);
	}

	/* Let the ADC have enough time to finish last conversion */
	/* This is necessary as reading the ADC also triggers a new conversion */
	usleep(TLC1543_SAMPLING_TIME);

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

	int value = tlc1543_init(&tlc, 0);
	if (value < 0)
		return value;
	value = tlc1543_get_sample(&tlc, channel);
	tlc1543_delete(&tlc);

	return value;
}

/**
 * @brief Convert what the ADC read to a value in mV (vref_min is 0V on the ArPi6000)
 * 
 * @param value Value acquired from the ADC
 * @param vref_max Voltage (mV) at the VREF+ pin on the TLC1543
 * @return Converted value in mV
 */
inline int tlc1543_convert(const int value, const int vref_max)
{
	return (vref_max / (1 << 10)) * value;
}

#ifdef __cplusplus
}
#endif

#endif // TLC1543_H
