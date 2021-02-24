/*
 * Manage time and memory persisting over deep sleep.
 * rtc.h
 *
 *  Created on: 22 lut 2021
 *      Author: andrzej
 */

#ifndef MAIN_RTC_H_
#define MAIN_RTC_H_

#include <stdint.h>

/**
 * Initialize timestamp.
 * Read timestamp from RTC memory.
 */
void time_init(void);
/**
 * Return current timestamp [s].
 */
uint32_t get_timestamp(void);
/**
 * Set current timestamp [s].
 */
void set_timestamp(uint32_t timestamp);
/**
 * Save timestamp to RTC memory.
 */
void save_timestamp(void);

#endif /* MAIN_RTC_H_ */
