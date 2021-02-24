/*
 * rtc.c
 *
 *  Created on: 22 lut 2021
 *      Author: andrzej
 */

#include "driver/rtc.h"

#include "storage.h"

#define GNIOT_RTC_MAGIC 0x1331defe

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
    write_rtc_mem(0, GNIOT_RTC_MAGIC);
}
