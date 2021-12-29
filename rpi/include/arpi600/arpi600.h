/**
 * @brief Library for the ARPI600 expansion card for the Rasbperry Pi from XBee
 * 
 * @file arpi600.h
 * @defgroup ArPi600 XBee ArPi600 expansion card
 * @copyright (c) Pierre Boisselier
 * 
 * @details
 * This header includes each library for each device on the expansion card.
 * 
 * Libraries availabe:
 *      - PCF8563 Real-Time Clock
 *      - TLC1543 10-Bit ADC
 */

#ifndef ARPI600_H
#define ARPI600_H

#ifdef __cplusplus
extern "C" {
#endif

#include <arpi600/pcf8563.h>
#include <arpi600/tlc1543.h>

#ifdef __cplusplus
}
#endif

#endif // ARPI600_H