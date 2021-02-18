/*
 * storage.h
 *
 *  Created on: 13 lut 2021
 *      Author: andrzej
 */

#ifndef MAIN_STORAGE_H_
#define MAIN_STORAGE_H_

#include <stdint.h>

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

typedef uint32_t StorageSample_t;


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

int storage_get_sample(uint32_t idx, StorageSample_t * sample);
int storage_sample_read_finish(void);

int storage_clear_samples(void);
int storage_store_sample(const StorageSample_t * sample);


#endif /* MAIN_STORAGE_H_ */
