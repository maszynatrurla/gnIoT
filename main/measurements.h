/**
 * API for collecting temp+humidity samples and processing
 * them.
 * measurements.h
 *
 *  Created on: 14 lut 2021
 *      Author: andrzej
 */

#ifndef MAIN_MEASUREMENTS_H_
#define MAIN_MEASUREMENTS_H_

#include "humtemp.h"
#include "storage.h"

/**
 * Initialize measurement.
 * @param cfg system configuration
 */
void measurement_init(const GniotConfig_t * cfg);
/**
 * Add single sample from sensor.
 * @param h humidity
 * @param t temperature
 */
void measurement_add_sample(Humidity_t h, Temperature_t t);
/**
 * Get processed, encoded value of measurement.
 * @param out output buffer
 * @return 0 - success or error code
 */
int measurement_get(uint32_t * out);

#endif /* MAIN_MEASUREMENTS_H_ */
