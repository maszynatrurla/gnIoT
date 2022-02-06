/*
 * Process sensor data.
 * measurements.c
 *
 *  Created on: 14 lut 2021
 *      Author: andrzej
 */

#include <stdlib.h>

#include "measurements.h"
#include "esp_log.h"

typedef struct {
    uint16_t h;
    uint16_t t;
} Sample_t;

static Sample_t * s_buf = NULL;
static uint32_t s_idx;
static uint32_t s_total;

static uint16_t sample_convert(DHT_fixedpoint sample)
{
    return sample * 10;
}

void measurement_init(const GniotConfig_t * cfg)
{
    s_idx = 0;
    s_total = cfg->samples_per_measure;

    if (s_buf)
    {
        free(s_buf);
    }

    s_buf = malloc(sizeof(Sample_t) * s_total);

    if (!s_buf)
    {
        ESP_LOGE("meas", "Memory alloc error\n");
        abort();
    }
}

void measurement_add_sample(Humidity_t h, Temperature_t t)
{
    uint16_t sh = sample_convert(h);
    uint16_t st = sample_convert(t);
    s_buf[s_idx].h = sh;
    s_buf[s_idx].t = st;
    ++s_idx;
}

static void measurement_reset(void)
{
    s_idx = 0;
}

static void sort_buf(void)
{
    for (int i = 1; i < s_idx; ++i)
    {
        for (int j = i; (j > 0) && (s_buf[j-1].h > s_buf[j].h); --j)
        {
            Sample_t tmp = s_buf[j];
            s_buf[j] = s_buf[j - 1];
            s_buf[j - 1] = tmp;
        }
    }
}

int measurement_get(uint32_t * out)
{
    uint32_t sum_h = 0, sum_t = 0;

    if (s_idx == 0)
    {
        measurement_reset();
        return -1;
    }

    if (s_idx > 2)
    {
        /* median filter */
        sort_buf();
        sum_h = s_buf[s_idx / 2].h;
        sum_t = s_buf[s_idx / 2].t;
    }
    else
    {
#if 0
        /* simple average */
        int i;
        for (i = 0; i < s_idx; ++i)
        {
            sum_h += s_buf[i].h;
            sum_t += s_buf[i].t;
        }
        sum_h /= s_idx;
        sum_t /= s_idx;
#endif
        sum_h = s_buf[0].h;
        sum_t = s_buf[0].t;
    }

    *out = (((uint32_t) sum_h) << 16) | sum_t;
    measurement_reset();
    return 0;
}

