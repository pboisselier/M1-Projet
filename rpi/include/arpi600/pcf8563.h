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
 * 
 *  - Finding the I2C address
 *           Use: pi@rpi:~ $ i2cdetect -y 1 
 *      This will print all I2C devices connected on the default rasbperry pi bus, 
 *      you can change the I2C address by changing de PCF8563_I2C_ADDR define.
 *      This is easily done using the -D option in most compilers (gcc/clang).
 *      Example: cc -DPCF8553_I2C_ADDR=0x51 -std=c11 test.c -o test
 * 
 */

#ifndef PCF8563_H
#define PCF8563_H

#if __STDC_VERSION__ < 201112L || __cplusplus < 201103L
#error Please use a C11/C++11 (or higher) compliant compiler!
#endif

/* Do not override user-defined i2c address */
#ifndef PCF8563_I2C_ADDR
#define PCF8563_I2C_ADDR 0x51
#endif

#ifndef RPI_I2C_BUS
#define RPI_I2C_BUS I2C_SLAVE
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
#include <linux/i2c-dev.h>
#include <time.h>
#include <errno.h>

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

error_t pcf8563_init_c(const char *i2c_device)
{
	error_t err = 0;

	if (!i2c_device)
		return EINVAL; // TODO: Make it better...

	int fd = open(i2c_device, O_RDWR);
	if (fd < 0)
		return errno;

	if (ioctl(fd, RPI_I2C_BUS, PCF8563_I2C_ADDR) < 0) {
		err = (error_t)errno;
		(void)close(fd);
		return err;
	}

	i2c_fd = fd;

	return err;
}

error_t pcf8563_init(void)
{
	return pcf8563_init_c("/dev/i2c-1");
}

error_t pcf8563_reset(void)
{
	if (i2c_fd == -1)
		return -1001; // TODO

	// TODO

        return 0;
}

error_t pcf8563_read_time(time_t *time)
{
	if (i2c_fd == -1)
		return -1001; // TODO

	if (!time)
		return EINVAL; // TODO: Make it better...

	struct tm tm_pcf = { 0 };

	*time = mktime(&tm_pcf);

        return 0;
}

error_t pcf8563_set_time(const time_t time)
{
	// TODO
        return 0;
}

bool pcf8563_is_voltage_low(void)
{
        // TODO
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