/*
 * storage.h
 *
 *  Created on: 13 lut 2021
 *      Author: andrzej
 */

#ifndef MAIN_STORAGE_H_
#define MAIN_STORAGE_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * Size of storage slot for measurements
 * (in number of measurements)
 * Chosen to fit into RTC memory.
 */
#define MEAS_STORAGE_BANK_SIZE  60

typedef struct
{
    char server_address [32];
    uint16_t server_port;

    char fallback_server_address [32];
    uint16_t fallback_server_port;

    uint32_t my_id;

    uint16_t measure_period;
    uint16_t samples_per_measure;

    uint16_t measures_per_sleep;
    uint16_t sleep_length;
} GniotConfig_t;

typedef struct
{
    uint32_t ts;
    uint32_t data;
}
StorageSample_t;


void storage_init(void);

void config_init(void);
const GniotConfig_t * config_get(void);
int config_set_server(const char * server_address, uint16_t port);
int config_set_server_safe(const char * server_address, uint16_t port);
int config_switch_server(void);
int config_set_fallback_server(const char * server_address, uint16_t port);
int config_set_myid(uint32_t my_id);
int config_set_measure(uint16_t measure_period, uint16_t samples_per_measure);
int config_set_sleep(uint16_t measures_per_sleep, uint16_t sleep_length);

void storage_sample_start(void);
int storage_next(StorageSample_t * sample);
void storage_sample_finish(bool clear_all);
void storage_save_sample(const StorageSample_t * sample);
void storage_clear(void);


#endif /* MAIN_STORAGE_H_ */
