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
#include "storage.h"

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
void save_timestamp(uint32_t add);

int save_data_in_rtc(const StorageSample_t * data);
int save_data_in_rtc_at(int idx, const StorageSample_t * data);
int read_data_from_rtc(int idx, StorageSample_t * data);
void clear_rtc_data(void);

#endif /* MAIN_RTC_H_ */
