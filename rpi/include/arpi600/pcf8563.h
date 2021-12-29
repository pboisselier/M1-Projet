/**
 * @brief Library for the PCF8563 real-time clock from NXP
 *
 * @file pcf8563.h
 * @copyright (c) Pierre Boisselier
 * 
 * @details 
 * The PCF8563 is a real-time clock from NXP that communicates over I2C.
 * Refer to https://www.nxp.com/docs/en/data-sheet/PCF8563.pdf for more informations
 * 
 * ARPI600 Implementation specific:
 *      - Set the RTC jumper 
 * 
 * Usage:
 *  - Finding the I2C address
 *           Use: pi@rpi:~ $ i2cdetect -y 1 
 *      This will print all I2C devices connected on the default rasbperry pi bus, 
 *      you can change the I2C address by changing de PCF8563_I2C_ADDR define.
 *      This is easily done using the -D option in most compilers (gcc/clang).
 *      Example: cc -DPCF8553_I2C_ADDR=0x51 -std=c11 test.c -o test
 * 
 * - Get current time:
 * 	- Use "pcf8563_init" to initialize the RTC, and retrieve the file descriptor
 * 	- Create a time_t variable to store current time
 * 	- Call "pcf8563_read_time(fd, &variable)" to read current time into the variable
 *      - You can check the return of the function for errors
 * 	- Use "pcf8563_close(fd)" to close the connection to the RTC
 * 
 * - Set current time
 * 	- Use "pcf8563_init" to initialize the RTC
 * 	- Call "pcf8563_set_time(fd, &tm)" to update the RTC time
 * 
 * Error handling:
 * 	All functions return a NEGATIVE number when an error occured, it is worth checking errno
 *      using perror for instance as most errors might come from I/O operations. 
 */

#ifndef PCF8563_H
#define PCF8563_H

/* Do not override user-defined i2c address */
#ifndef PCF8563_I2C_ADDR
#define PCF8563_I2C_ADDR 0x51
#endif
#ifndef RPI_I2C_BUS
#define RPI_I2C_BUS 0x703
#endif

/* Control registers */
#define PCF8563_REG_CSTATUS_1 0x00
#define PCF8563_REG_CSTATUS_2 0x01
#define PCF8563_REG_CLKOUT 0x0D
/* Time registers - values in BCD */
#define PCF8563_REG_VLSEC 0x02
#define PCF8563_REG_MIN 0x03
#define PCF8563_REG_HOUR 0x04
#define PCF8563_REG_DAY 0x05
#define PCF8563_REG_WEEKDAY 0x06
#define PCF8563_REG_CENTURY_MONTH 0x07
#define PCF8563_REG_YEAR 0x08
/* Alarm registers - values in BCD */
#define PCF8563_REG_ARLM_MIN 0x09
#define PCF8563_REG_ARLM_HOUR 0x0A
#define PCF8563_REG_ARLM_DAY 0x0B
#define PCF8563_REG_ARLM_WEEKDAY 0x0C
/* Timer registers */
#define PCF8563_TIMER_CTRL 0x0E
#define PCF8563_TIMER 0x0F

/* Returned error values */
#define PCF8563_ERR -1
#define PCF8563_ERR_ARG -10
#define PCF8563_ERR_NOPEN -11
#define PCF8563_ERR_READ -20
#define PCF8563_ERR_WRITE -21

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Enable multithreading functions if supported */
#ifdef _POSIX_THREADS

#include <pthread.h>

/* Local mutex for access to the pcf8563 */
static pthread_mutex_t pcf_mut = PTHREAD_MUTEX_INITIALIZER;

#endif

static inline int bcd_to_dec(uint8_t value)
{
	return ((value & 0xF0) >> 4) * 10 + (value & 0x0F);
}

static inline uint8_t dec_to_bcd(int value)
{
	return ((uint8_t)(value / 10) << 4) | ((uint8_t)(value % 10));
}

/**
 * @brief Print PCF8563 errors, useful when getting a negative value from a function
 * 
 * @param error Error number returned by a function 
 * @param msg Optional message to add
 */
void pcf8563_print_err(const int error, const char *msg)
{
	switch (error) {
	case PCF8563_ERR:
		fprintf(stderr,
			"A generic error occured with the PCF8563 module (errno: %s): %s\n",
			strerror(errno), msg);
		break;
	case PCF8563_ERR_ARG:
		fprintf(stderr, "Bad argument provided (errno: %s): %s\n",
			strerror(errno), msg);
		break;
	case PCF8563_ERR_NOPEN:
		fprintf(stderr,
			"Connection to PCF8563 not initialized (errno: %s): %s\n",
			strerror(errno), msg);
		break;
	case PCF8563_ERR_READ:
		fprintf(stderr, "Unable to read to PCF8563 (errno: %s): %s\n",
			strerror(errno), msg);
		break;
	case PCF8563_ERR_WRITE:
		fprintf(stderr, "Unable to write to PCF8563 (errno: %s): %s\n",
			strerror(errno), msg);
		break;
	}
}

/**
 * @brief Open connection to the PFC8563 RTC and return a file descriptor 
 * 
 * @param i2c_device I2C device path (/dev/i2c-1 for instance)
 * @param bus_addr I2C bus address (0x703 for Rapsberry Pi 4 for instance)
 * @return a file descriptor to the PCF8563 RTC (or error if negative)
 */
int pcf8563_init_c_ui16(const char *i2c_device, uint16_t bus_addr)
{
	if (!i2c_device)
		return PCF8563_ERR_ARG;

	int fd = open(i2c_device, O_RDWR);
	if (fd < 0)
		return PCF8563_ERR_NOPEN;

	if (ioctl(fd, bus_addr, PCF8563_I2C_ADDR) < 0) {
		(void)close(fd);
		return PCF8563_ERR_NOPEN;
	}

	return fd;
}

/**
 * @brief Open a connection to the PCF8563 RTC with default Raspberry Pi I2C bus address
 * 
 * @param i2c_device I2C device path (/dev/i2c-1 for instance)
 * @return a file descriptor to the PCF8563 RTC (or error if negative)
 */
int pcf8563_init_c(const char *i2c_device)
{
	return pcf8563_init_c_ui16(i2c_device, RPI_I2C_BUS);
}

/**
 * @brief Open a connection to the PCF8563 RTC with default address
 * 
 * @return a file descriptor to the PCF8563 RTC (or error if negative)
 */
int pcf8563_init(void)
{
	return pcf8563_init_c_ui16("/dev/i2c-1", RPI_I2C_BUS);
}

/**
 * @brief Close I2C connection to the PCF8563 RTC
 * 
 * @param i2c_fd Connection to the PCF8563 as a file descriptor
 * @return 0 on success, negative value on error
 */
int pcf8563_close(const int i2c_fd)
{
	if (i2c_fd < 0)
		return PCF8563_ERR_ARG;

	if (close(i2c_fd) < 0)
		return PCF8563_ERR;

	return 0;
}

/**
 * @brief Read current time from the RTC clock
 * 
 * @param i2c_fd Opened connection to the RTC clock 
 * @param time Pointer to a time_t that will contain the time
 * @return 0 on success, negative value on error
 */
int pcf8563_read_time(const int i2c_fd, time_t *time)
{
	if (i2c_fd < 0)
		return PCF8563_ERR_NOPEN;
	if (!time)
		return PCF8563_ERR_ARG;

	uint8_t buf[7];
	uint8_t start_read = PCF8563_REG_VLSEC;

	/* Following recommended method to read time from the datasheet (c 8.5)
         * - Send 0x02 (VL_SEC register)
         * - Read all time registers
         * - Convert BCD values to decimal
         */
	if (write(i2c_fd, &start_read, 1) < 0)
		return PCF8563_ERR_WRITE;
	if (read(i2c_fd, buf, sizeof(buf)) < 0)
		return PCF8563_ERR_READ;

	struct tm tm_pcf = { 0 };

	tm_pcf.tm_sec = bcd_to_dec(buf[0] & 0x7F); // See Table 9
	tm_pcf.tm_min = bcd_to_dec(buf[1] & 0x7F); // See Table 10
	tm_pcf.tm_hour = bcd_to_dec(buf[2] & 0x3F); // See Table 11
	tm_pcf.tm_mday = bcd_to_dec(buf[3] & 0x3F); // See Table 12
	tm_pcf.tm_wday = bcd_to_dec(buf[4] & 0x07); // See Table 14
	tm_pcf.tm_mon = bcd_to_dec(buf[5] & 0x1F) - 1; // See Table 16
	tm_pcf.tm_year = bcd_to_dec(buf[6]) + ((buf[5] & 0x80) * 100) +
			 100; // See Table 17 and 15

	*time = mktime(&tm_pcf);

	return 0;
}

/**
 * @brief Set a time on the RTC clock
 * 
 * @param i2c_fd Opened connection to the RTC clock 
 * @param time Pointer to a time_t that will contain the time
 * @return 0 on success, negative value on error
 */
int pcf8563_set_time(const int i2c_fd, const time_t *time)
{
	if (i2c_fd < 0)
		return PCF8563_ERR_NOPEN;
	if (!time)
		return PCF8563_ERR_ARG;

	struct tm *tm_pcf = localtime(time);
	uint8_t buf[8];

	buf[0] = PCF8563_REG_VLSEC; // Writing starts at VLSEC register
	buf[1] = dec_to_bcd(tm_pcf->tm_sec) & 0x7F;
	buf[2] = dec_to_bcd(tm_pcf->tm_min) & 0x7F;
	buf[3] = dec_to_bcd(tm_pcf->tm_hour) & 0x3F;
	buf[4] = dec_to_bcd(tm_pcf->tm_mday) & 0x3F;
	buf[5] = dec_to_bcd(tm_pcf->tm_wday) & 0x07;
	buf[6] = (dec_to_bcd(tm_pcf->tm_mon + 1) & 0x1F) |
		 (tm_pcf->tm_mon > 99 ? 0x80 : 0x00);
	buf[7] = dec_to_bcd((tm_pcf->tm_year - 100) % 100);

	if (write(i2c_fd, buf, sizeof(buf)) < 0)
		return PCF8563_ERR_WRITE;

	return 0;
}

/**
 * @brief Check whether the RTC battery is low or not
 * 
 * @param i2c_fd Opened connection to the RTC clock 
 * @return 0 if voltage is NOT low, 1 if low, negative value on error 
 */
int pcf8563_is_voltage_low(const int i2c_fd)
{
	if (i2c_fd < 0)
		return PCF8563_ERR_NOPEN;

	uint8_t vl = PCF8563_REG_VLSEC;
	if (write(i2c_fd, &vl, sizeof(vl)) < 0)
		return PCF8563_ERR_WRITE;
	if (read(i2c_fd, &vl, sizeof(vl)) < 0)
		return PCF8563_ERR_READ;

	/* Return directly the value of the VL bit */
	return (vl & (1 << 7));
}

/* This function tests the STOP bit from the control status 1 register */
/**
 * @brief Check whether the RTC is running or not
 * 
 * @param i2c_fd Opened connection to the RTC clock 
 * @return 0 if not running, 1 if running, negative value on error
 */
int pcf8563_is_running(const int i2c_fd)
{
	if (i2c_fd < 0)
		return PCF8563_ERR_NOPEN;

	/* Read the Control Status 1 Register */
	uint8_t cstatus = PCF8563_REG_CSTATUS_1;

	if (write(i2c_fd, &cstatus, sizeof(cstatus)) < 0)
		return PCF8563_ERR_WRITE;
	if (read(i2c_fd, &cstatus, sizeof(cstatus)) < 0)
		return PCF8563_ERR_READ;

	return ((cstatus & (1 << 5)) == 0);
}

#ifdef __cplusplus
}
#endif

#endif // PCF8563