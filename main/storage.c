/*
 * storage.c
 *
 *  Created on: 13 lut 2021
 *      Author: andrzej
 */

#include <string.h>

#include "nvs.h"
#include "nvs_flash.h"
#include "storage.h"
#include "credentials.h"
#include "rtc.h"

#define STO_NAMESPACE   "gniot"

#define STO_KEY_SERVER               "srv_d"
#define STO_KEY_SERVER_FALLBACK      "srv_f"
#define STO_KEY_MY_ID                "my_id"
#define STO_KEY_SLEEP                "sleep"
#define STO_KEY_MEAS                 "meas"

#define STO_KEY_SAMPLE               "m_"

#define DEFAULT_MEASURE_COUNT       3
#define DEFAULT_MEASURE_PERIOD      60
#define DEFAULT_MEASURES_PER_SLEEP  1
#define DEFAULT_SLEEP_LENGTH        3


#define STORAGE_BANK_COUNT  6

static GniotConfig_t s_config;

static struct {
    nvs_handle handle;
    int bank_idx;
    int sample_idx;
    int first_bank;
    int written_banks;
} s_store_read = {0,};

void storage_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
}

void config_init(void)
{
    nvs_handle handle;
    uint64_t tmp64;
    uint32_t tmp32;

    ESP_ERROR_CHECK(nvs_open(STO_NAMESPACE, NVS_READWRITE, &handle));

    if (ESP_OK == nvs_get_u64(handle, STO_KEY_SERVER, &tmp64))
    {
        int tmp0 = tmp64 & 255;
        int tmp1 = (tmp64 >> 8) & 255;
        int tmp2 = (tmp64 >> 16) & 255;
        int tmp3 = (tmp64 >> 24) & 255;
        sprintf(s_config.server_address, "%d.%d.%d.%d", tmp0, tmp1, tmp2, tmp3);
        s_config.server_port = (uint16_t) ((tmp64 >> 32) & 0xFFFF);
    }
    else
    {
        strncpy(s_config.server_address, CRED_DEFAULT_SERVER, sizeof(s_config.server_address));
        s_config.server_port = CRED_DEFAULT_SERVER_PORT;
    }

    if (ESP_OK == nvs_get_u64(handle, STO_KEY_SERVER_FALLBACK, &tmp64))
    {
        int tmp0 = tmp64 & 255;
        int tmp1 = (tmp64 >> 8) & 255;
        int tmp2 = (tmp64 >> 16) & 255;
        int tmp3 = (tmp64 >> 24) & 255;
        sprintf(s_config.fallback_server_address, "%d.%d.%d.%d", tmp0, tmp1, tmp2, tmp3);
        s_config.fallback_server_port = (uint16_t) ((tmp64 >> 32) & 0xFFFF);
    }
    else
    {
        strncpy(s_config.fallback_server_address, CRED_DEFAULT_FALLBACK_SERVER, sizeof(s_config.server_address));
        s_config.fallback_server_port = CRED_DEFAULT_FALLBACK_PORT;
    }

    if (ESP_OK == nvs_get_u32(handle, STO_KEY_MY_ID, &tmp32))
    {
        s_config.my_id = tmp32;
    }
    else
    {
        s_config.my_id = 0;
    }

    if (ESP_OK == nvs_get_u32(handle, STO_KEY_MEAS, &tmp32))
    {
        s_config.samples_per_measure = (uint16_t) tmp32;
        s_config.measure_period = (uint16_t) (tmp32 >> 16);
    }
    else
    {
        s_config.samples_per_measure = DEFAULT_MEASURE_COUNT;
        s_config.measure_period = DEFAULT_MEASURE_PERIOD;
    }

    if (ESP_OK == nvs_get_u32(handle, STO_KEY_SLEEP, &tmp32))
    {
        s_config.measures_per_sleep = (uint16_t) tmp32;
        s_config.sleep_length = (uint16_t) (tmp32 >> 16);
    }
    else
    {
        s_config.measures_per_sleep = DEFAULT_MEASURES_PER_SLEEP;
        s_config.sleep_length = DEFAULT_SLEEP_LENGTH;
    }

    nvs_close(handle);
}

const GniotConfig_t * config_get(void)
{
    return &s_config;
}

static uint32_t silly_address_parser(const char * sip)
{
    uint32_t number = 0;
    uint32_t result = 0;
    int bit = 0;

    while (sip && (bit < 32))
    {
        if (*sip == '.')
        {
            result |= (number << bit);
            number = 0;
            bit += 8;
        }
        else
        {
            number *= 10;
            number += (*sip - 0x30);
        }
        ++sip;
    }
    if (bit < 32)
    {
        result |= (number << bit);
    }


    return result;
}

int config_set_server(const char * server_address, uint16_t port)
{
    nvs_handle handle;
    uint32_t add;
    uint64_t tmp;
    esp_err_t err;

    strncpy(s_config.server_address, server_address, sizeof(s_config.server_address));
    s_config.server_port = port;

    ESP_ERROR_CHECK(nvs_open(STO_NAMESPACE, NVS_READWRITE, &handle));
    add = silly_address_parser(server_address);
    tmp = ((uint64_t) add);
    tmp |= ((uint64_t) port) << 32;

    err = nvs_set_u64(handle, STO_KEY_SERVER, tmp);

    nvs_commit(handle);
    nvs_close(handle);

    return err;
}

int config_set_server_safe(const char * server_address, uint16_t port)
{
    nvs_handle handle;
    uint32_t add;
    uint64_t tmp;
    esp_err_t err;

    strncpy(s_config.fallback_server_address, s_config.server_address, sizeof(s_config.fallback_server_address));
    s_config.fallback_server_port = s_config.server_port;
    strncpy(s_config.server_address, server_address, sizeof(s_config.server_address));
    s_config.server_port = port;

    ESP_ERROR_CHECK(nvs_open(STO_NAMESPACE, NVS_READWRITE, &handle));
    add = silly_address_parser(server_address);
    tmp = ((uint64_t) add);
    tmp |= ((uint64_t) port) << 32;

    err = nvs_set_u64(handle, STO_KEY_SERVER, tmp);

    add = silly_address_parser(s_config.fallback_server_address);
    tmp = ((uint64_t) add);
    tmp |= ((uint64_t) s_config.fallback_server_port) << 32;

    err = nvs_set_u64(handle, STO_KEY_SERVER_FALLBACK, tmp);

    nvs_commit(handle);
    nvs_close(handle);

    return err;
}

int config_switch_server(void)
{
    nvs_handle handle;
    uint32_t add;
    uint64_t tmp;
    esp_err_t err;

    for (int i = 0; i < sizeof(s_config.server_address); ++i)
    {
        s_config.server_address[i] ^= s_config.fallback_server_address[i];
        s_config.fallback_server_address[i] ^= s_config.server_address[i];
        s_config.server_address[i] ^= s_config.fallback_server_address[i];
    }

    s_config.server_port ^= s_config.fallback_server_port;
    s_config.fallback_server_port ^= s_config.server_port;
    s_config.server_port ^= s_config.fallback_server_port;

    ESP_ERROR_CHECK(nvs_open(STO_NAMESPACE, NVS_READWRITE, &handle));
    add = silly_address_parser(s_config.server_address);
    tmp = ((uint64_t) add);
    tmp |= ((uint64_t) s_config.server_port) << 32;

    err = nvs_set_u64(handle, STO_KEY_SERVER, tmp);

    add = silly_address_parser(s_config.fallback_server_address);
    tmp = ((uint64_t) add);
    tmp |= ((uint64_t) s_config.fallback_server_port) << 32;

    err = nvs_set_u64(handle, STO_KEY_SERVER_FALLBACK, tmp);

    nvs_commit(handle);
    nvs_close(handle);

    return err;
}

int config_set_fallback_server(const char * server_address, uint16_t port)
{
    nvs_handle handle;
    uint32_t add;
    uint64_t tmp;
    esp_err_t err;

    strncpy(s_config.fallback_server_address, server_address, sizeof(s_config.fallback_server_address));
    s_config.fallback_server_port = port;

    ESP_ERROR_CHECK(nvs_open(STO_NAMESPACE, NVS_READWRITE, &handle));
    add = silly_address_parser(server_address);
    tmp = ((uint64_t) add);
    tmp |= ((uint64_t) port) << 32;

    err = nvs_set_u64(handle, STO_KEY_SERVER_FALLBACK, tmp);

    nvs_commit(handle);
    nvs_close(handle);

    return (int) err;
}

int config_set_myid(uint32_t my_id)
{
    nvs_handle handle;
    esp_err_t err;

    s_config.my_id = my_id;

    ESP_ERROR_CHECK(nvs_open(STO_NAMESPACE, NVS_READWRITE, &handle));
    err = nvs_set_u32(handle, STO_KEY_MY_ID, my_id);

    nvs_commit(handle);
    nvs_close(handle);

    return (int) err;
}

int config_set_measure(uint16_t measure_period, uint16_t samples_per_measure)
{
    nvs_handle handle;
    esp_err_t err;
    uint32_t tmp;

    s_config.measure_period = measure_period;
    s_config.samples_per_measure = samples_per_measure;

    ESP_ERROR_CHECK(nvs_open(STO_NAMESPACE, NVS_READWRITE, &handle));
    tmp = (uint32_t) samples_per_measure;
    tmp |= ((uint32_t) measure_period) << 16;
    err = nvs_set_u32(handle, STO_KEY_MEAS, tmp);

    nvs_commit(handle);
    nvs_close(handle);

    return (int) err;
}

int config_set_sleep(uint16_t measures_per_sleep, uint16_t sleep_length)
{
    nvs_handle handle;
    esp_err_t err;
    uint32_t tmp;

    s_config.measures_per_sleep = measures_per_sleep;
    s_config.sleep_length = sleep_length;

    ESP_ERROR_CHECK(nvs_open(STO_NAMESPACE, NVS_READWRITE, &handle));
    tmp = (uint32_t) measures_per_sleep;
    tmp |= ((uint32_t) sleep_length) << 16;
    err = nvs_set_u32(handle, STO_KEY_SLEEP, tmp);

    nvs_commit(handle);
    nvs_close(handle);

    return (int) err;
}


void storage_sample_start(void)
{
    ESP_ERROR_CHECK(nvs_open(STO_NAMESPACE, NVS_READWRITE, &s_store_read.handle));
    s_store_read.bank_idx = 0;
    s_store_read.sample_idx = 0;
    s_store_read.written_banks = 0;

    int old = -1;
    uint32_t oldts = 0xFFFFFFFF;

    for (int bi = 0; bi < STORAGE_BANK_COUNT; ++bi)
    {
        StorageSample_t sbuf[MEAS_STORAGE_BANK_SIZE];
        char key [] = "mb ";
        size_t sblen = sizeof(sbuf);
        key[2] = 0x30 + s_store_read.bank_idx;

        if (ESP_OK == nvs_get_blob(s_store_read.handle, key, sbuf, &sblen))
        {
            ++s_store_read.written_banks;
            if (sbuf[0].ts < oldts)
            {
                oldts = sbuf[0].ts;
                old = bi;
            }
        }
    }

    s_store_read.first_bank = old;

}

int storage_next(StorageSample_t * sample)
{
    if (s_store_read.first_bank >= 0)
    {
        for (; s_store_read.bank_idx < STORAGE_BANK_COUNT; ++s_store_read.bank_idx)
        {
            StorageSample_t sbuf[MEAS_STORAGE_BANK_SIZE];
            char key [] = "mb ";
            size_t sblen = sizeof(sbuf);
            int bank_idx  = (s_store_read.first_bank + s_store_read.bank_idx) % STORAGE_BANK_COUNT;
            key[2] = 0x30 + bank_idx;

            if (ESP_OK == nvs_get_blob(s_store_read.handle, key, sbuf, &sblen))
            {
                *sample = sbuf[s_store_read.sample_idx];
                ++(s_store_read.sample_idx);
                if (s_store_read.sample_idx >= MEAS_STORAGE_BANK_SIZE)
                {
                    s_store_read.sample_idx = 0;
                    ++(s_store_read.bank_idx);
                }
                return 0;
            }
        }
    }
    for (; s_store_read.sample_idx < MEAS_STORAGE_BANK_SIZE; ++s_store_read.sample_idx)
    {

        if (0 == read_data_from_rtc(s_store_read.sample_idx, sample))
        {
            ++s_store_read.sample_idx;
            return 0;
        }
    }
    return -1;
}

void storage_sample_finish(bool clear_all)
{
    if (clear_all)
    {
        storage_clear();
    }
    nvs_close(s_store_read.handle);
}

void storage_save_sample(const StorageSample_t * sample)
{
    if (save_data_in_rtc(sample) < 0)
    {
        StorageSample_t sbuf[MEAS_STORAGE_BANK_SIZE];
        char key [] = "mb ";
        int bank;

        if (s_store_read.written_banks < STORAGE_BANK_COUNT)
        {
            bank = s_store_read.written_banks;
            ++s_store_read.written_banks;
        }
        else
        {
            bank = s_store_read.first_bank;
            s_store_read.first_bank = (s_store_read.first_bank + 1) % STORAGE_BANK_COUNT;
        }

        key[2] = 0x30 + bank;

        for (int si = 0; si < MEAS_STORAGE_BANK_SIZE; ++si)
        {
            read_data_from_rtc(si, &sbuf[si]);
        }

        nvs_set_blob(s_store_read.handle, key, sbuf, sizeof(sbuf));

        clear_rtc_data();
        save_data_in_rtc(sample);
    }
}

void storage_clear(void)
{
    for (int bi = 0; bi < STORAGE_BANK_COUNT; ++bi)
    {
        char key [] = "mb ";
        key[2] = 0x30 + bi;

        nvs_erase_key(s_store_read.handle, key);
    }
    clear_rtc_data();
}

