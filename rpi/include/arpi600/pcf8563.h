/*
 * Library for the PCF8563 real-time clock from NXP
 *
 * (c) Pierre Boisselier
 * 
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
 * 	- Use "pcf8563_init" to initialize the RTC
 * 	- Create a time_t variable to store current time
 * 	- Call "pcf8563_read_time(&variable)" to read current time into the variable
 *      - You can check the return of the function for errors
 * 	- Use "pcf8563_close()" to close the connection to the RTC
 * 
 * - Set current time
 * 	- Use "pcf8563_init" to initialize the RTC
 * 	- Call "pcf8563_set_time(&tm)" to update the RTC time
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

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Enable multithreading functions if supported */
#ifdef _POSIX_THREADS

#include <pthread.h>

/* Local mutex for access to the pcf8563 */
static pthread_mutex_t pcf_mut = PTHREAD_MUTEX_INITIALIZER;

#endif

static int i2c_fd = -1;

static inline int bcd_to_dec(uint8_t value)
{
	return ((value & 0xF0) >> 4) * 10 + (value & 0x0F);
}

static inline uint8_t dec_to_bcd(int value)
{
	return ((uint8_t)(value / 10) << 4) | ((uint8_t)(value % 10));
}

int pcf8563_init_c_ui16(const char *i2c_device, uint16_t bus_addr)
{
	int err = 0;

	if (!i2c_device)
		return EINVAL; // TODO: Make it better...

	int fd = open(i2c_device, O_RDWR);
	if (fd < 0)
		return errno;

	if (ioctl(fd, bus_addr, PCF8563_I2C_ADDR) < 0) {
		err = (int)errno;
		(void)close(fd);
		return err;
	}

	i2c_fd = fd;

	return err;
}

int pcf8563_init_c(const char *i2c_device)
{
	return pcf8563_init_c_ui16(i2c_device, RPI_I2C_BUS);
}

int pcf8563_init(void)
{
	return pcf8563_init_c_ui16("/dev/i2c-1", RPI_I2C_BUS);
}

int pcf8563_close(void)
{
	if (i2c_fd == -1)
		return 0; // Nothing to do

	if (close(i2c_fd) < 0)
		return errno;

	return 0;
}

int pcf8563_reset(void)
{
	if (i2c_fd == -1)
		return -1001; // TODO

	// TODO

	return 0;
}

int pcf8563_read_time(time_t *time)
{
	if (i2c_fd == -1)
		return -1001;
	if (!time)
		return -EINVAL; // TODO: Make it better...

	uint8_t buf[7];

	/* Following recommended method to read time from the datasheet (c 8.5)
         * - Send 0x02 (VL_SEC register)
         * - Read all time registers
         * - Convert BCD values to decimal
         */
	uint8_t start_read = PCF8563_REG_VLSEC;
	if (write(i2c_fd, &start_read, 1) < 0)
		return -errno;
	if (read(i2c_fd, buf, sizeof(buf)) < 0)
		return -errno;

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

int pcf8563_set_time(const time_t *time)
{
	if (i2c_fd == -1)
		return -1001; // TODO
	if (!time)
		return -EINVAL; // TODO

	struct tm *tm_pcf = localtime(time);
	uint8_t buf[8];
	buf[0] =
		PCF8563_REG_VLSEC; // We are going to write from the VLSec reg and onwards
	buf[1] = dec_to_bcd(tm_pcf->tm_sec) & 0x7F;
	buf[2] = dec_to_bcd(tm_pcf->tm_min) & 0x7F;
	buf[3] = dec_to_bcd(tm_pcf->tm_hour) & 0x3F;
	buf[4] = dec_to_bcd(tm_pcf->tm_mday) & 0x3F;
	buf[5] = dec_to_bcd(tm_pcf->tm_wday) & 0x07;
	buf[6] = (dec_to_bcd(tm_pcf->tm_mon + 1) & 0x1F) |
		 (tm_pcf->tm_mon > 99 ? 0x80 : 0x00);
	buf[7] = dec_to_bcd((tm_pcf->tm_year - 100) % 100);

	if (write(i2c_fd, buf, sizeof(buf)) < 0)
		return -errno;

	return 0;
}

bool pcf8563_is_voltage_low(int *errptr)
{
	// TODO
	return false;
}

/* This function tests the STOP bit from the control status 1 register */
bool pcf8563_is_running(int *errptr)
{
	int err = -1001;
	uint8_t buf = PCF8563_REG_CSTATUS_1;

	if (i2c_fd == -1)
		goto err;

	if (write(i2c_fd, &buf, 1) < 0)
		goto set_err;
	if (read(i2c_fd, &buf, 1) < 0)
		goto set_err;

	return ((buf & (1 << 5)) == 0);

set_err:
	err = errno;
err:
	if (errptr)
		*errptr = err; // TODO: Make it better.

	return false;
}

// TODO: Use BCM lib for i2c init and functions?
// TODO: import BCM gpio library for interrupts?
// TODO: Use Fast interrupts requests?
// TODO: Threaded IRQ for i2c writes and reads?
// TODO: Call backs/interrupts after a set time?
// TODO: Pthread
// https://linux-kernel-labs.github.io/refs/heads/master/lectures/interrupts.html
// https://stackoverflow.com/questions/18921933/how-an-i2c-read-as-well-as-write-operation-in-handler-function-of-request-thre

#ifdef __cplusplus
}
#endif

#endif // PCF8563