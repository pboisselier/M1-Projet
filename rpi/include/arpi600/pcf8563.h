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
 */

#ifndef PCF8563_H
#define PCF8563_H

#if __STDC_VERSION__ < 201112L || __cplusplus < 201103L
#error Please use a C11/C++11 (or higher) compliant compiler!
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Do not override user-defined i2c address */
#ifndef PCF8563_I2C_ADDR
#define PCF8563_I2C_ADDR 0x51
#endif

/* Enable multithreading functions if supported */
#ifdef _POSIX_THREADS 

#include <pthread.h>

/* Local mutex for access to the pcf8563 */
static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;



#endif

#ifdef __cplusplus
}
#endif

#endif // PCF8563