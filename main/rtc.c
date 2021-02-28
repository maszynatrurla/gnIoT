/*
 * rtc.c
 *
 *  Created on: 22 lut 2021
 *      Author: andrzej
 */

#include "rtc.h"

#include "storage.h"

#include "esp_system.h"
#include "esp_log.h"

#include "driver/rtc.h"

#define GNIOT_RTC_MAGIC 0x1331defe
#define STORE_DATA_SIZE sizeof(StorageSample_t)
#define RTC_MEM_SIZE    512
#define RTC_MEM_RESERVED 12
#define STORE_DATA_OFFSET 2

uint32_t s_time;
uint32_t s_time_rtc;

static inline uint32_t read_rtc_mem(uint32_t dwordIdx)
{
    return ((volatile uint32_t *) 0x60001200)[dwordIdx];
}

static inline void write_rtc_mem(uint32_t dwordIdx, uint32_t value)
{
    ((volatile uint32_t *) 0x60001200)[dwordIdx] = value;
}

static inline uint32_t get_rtc_timestamp(void)
{
    return rtc_clk_to_us(rtc_time_get(), pm_rtc_clock_cali_proc());
}

static void init_data_bank(void)
{
    int dwordi = STORE_DATA_OFFSET;
    assert((MEAS_STORAGE_BANK_SIZE * STORE_DATA_SIZE) < (RTC_MEM_SIZE - RTC_MEM_RESERVED));

    for (int i = 0; i < MEAS_STORAGE_BANK_SIZE; ++i)
    {
        write_rtc_mem(dwordi, 0); ++dwordi;
        write_rtc_mem(dwordi, 0); ++dwordi;
    }
}

void time_init(void)
{
    /* check if RTC memory is initialized by us */
    if (GNIOT_RTC_MAGIC == read_rtc_mem(0))
    {
        const GniotConfig_t * cfg = config_get();
        /* second word of memory should contain timestamp saved by us
         * before going into deep-sleep. Therefore we need to assume
         * that time now is time then plus however sleep lasts. */
        s_time = read_rtc_mem(1);
        s_time += cfg->sleep_length * 60;
    }
    else
    {
        /* no timestamp information available (cold reset probably)
         * initialize to zero. */
        s_time = 0;
    }
    s_time_rtc = get_rtc_timestamp();
}

uint32_t get_timestamp(void)
{
    uint32_t rtc_now = get_rtc_timestamp();

    if (rtc_now >= s_time_rtc)
    {
        s_time += ((rtc_now - s_time_rtc) / 1000000UL);
    }
    else
    {
        s_time += ((((0xFFFFFFFFUL - s_time_rtc) + 1UL) + rtc_now) / 1000000UL);
    }

    s_time_rtc = rtc_now;
    return s_time;
}

void set_timestamp(uint32_t timestamp)
{
    s_time = timestamp;
    s_time_rtc = get_rtc_timestamp();
}

void save_timestamp(void)
{
    /* first update it */
    (void) get_timestamp();
    write_rtc_mem(1, s_time);
    if (GNIOT_RTC_MAGIC != read_rtc_mem(0))
    {
        init_data_bank();
        write_rtc_mem(0, GNIOT_RTC_MAGIC);
    }
}


int save_data_in_rtc(const StorageSample_t * data)
{
    if (GNIOT_RTC_MAGIC != read_rtc_mem(0))
    {
        write_rtc_mem(1, 0);
        init_data_bank();
        write_rtc_mem(0, GNIOT_RTC_MAGIC);
    }
    for (int i = 0; i < MEAS_STORAGE_BANK_SIZE; ++i)
    {
        uint32_t dwi = STORE_DATA_OFFSET + 2 * i;

        if ((0 == read_rtc_mem(dwi)) && (0 == read_rtc_mem(dwi + 1)))
        {
            write_rtc_mem(dwi, data->ts);
            write_rtc_mem(dwi+1, data->data);
            return i;
        }
    }
    return -1;
}

int save_data_in_rtc_at(int idx, const StorageSample_t * data)
{
    if (GNIOT_RTC_MAGIC != read_rtc_mem(0))
    {
        write_rtc_mem(1, 0);
        init_data_bank();
        write_rtc_mem(0, GNIOT_RTC_MAGIC);
    }
    if (idx < MEAS_STORAGE_BANK_SIZE)
    {
        uint32_t dwi = STORE_DATA_OFFSET + 2 * idx;
        write_rtc_mem(dwi, data->ts);
        write_rtc_mem(dwi+1, data->data);
        return 0;
    }
    return -1;
}

int read_data_from_rtc(int idx, StorageSample_t * data)
{
    if ((GNIOT_RTC_MAGIC == read_rtc_mem(0)) && (idx < MEAS_STORAGE_BANK_SIZE))
    {
        uint32_t dwi = STORE_DATA_OFFSET + 2 * idx;
        data->ts = read_rtc_mem(dwi);
        data->data = read_rtc_mem(dwi+1);
        if (!((0 == data->ts) && (0 == data->data)))
        {
            return 0;
        }
    }

    return -1;
}

void clear_rtc_data(void)
{
    init_data_bank();
}
