/**
 * High level code controlling passing of data between gniot and server.
 * service.c
 *
 *  Created on: 14 lut 2021
 *      Author: andrzej
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "service.h"
#include "storage.h"
#include "client.h"
#include "ota.h"
#include "rtc.h"

#define DEFAULT_PORT    80

enum {
    S_CMD_DUMP_CFG,
    S_CMD_OTA,
};

#define CMD_SET(C)      (s_cmd.flags |= (1 << (C)))
#define IS_ANY_CMD_SET  (s_cmd.flags != 0)
#define IS_CMD_SET(C)   ((s_cmd.flags & (1 << (C))) != 0)
#define CMD_CLEAR(C)    (s_cmd.flag &= ~(1UL << (C)))
#define CMD_CLEAR_ALL() (s_cmd.flags = 0)

typedef struct {
    uint32_t flags;
} Cmd_t;

static char add_buf[18];

static Cmd_t s_cmd = {0};


/**
 * Primitive and rather silly IP address parser/validator.
 * Protects from some errors but does not guarantee that address is correct!
 * Accepts values in form IP:PORT, e.g. 10.1.4.2:8000
 */
static const char * get_address(const char * val, uint16_t * port)
{
    int ci = 0;

    memset(add_buf, 0, sizeof(add_buf));

    while (*val)
    {
        if (ci > 16)
        {
            return NULL;
        }
        if (*val == ':')
        {
            if (ci > 6)
            {
                ++val;
                break;
            }
            else
            {
                return NULL;
            }
        }
        else if ((*val != '.') && ((*val < 0x30) || (*val > 0x39)))
        {
            return NULL;
        }
        add_buf[ci] = *val;

        ++val;
        ++ci;
    }

    if (*val)
    {
        int iv = atoi(val);
        if (iv > 0 && iv <= 65535)
        {
            *port = iv;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        *port = DEFAULT_PORT;
    }

    return add_buf;
}

/**
 * Handler for server commands.
 */
void command_handler(const char * key, const char * val)
{
    const GniotConfig_t * cfg = config_get();

    if (0 == strcmp("timestamp", key))
    {
        printf("Timestamp: %s\n", val);
        set_timestamp((uint32_t) atoll(val));
    }
    else if (0 == strcmp("new_server", key))
    {
        const char * address;
        uint16_t port;
        printf("%s -> %s\n", key, val);
        address = get_address(val, &port);
        if (NULL == address)
        {
            printf("Invalid address\n");
        }
        else
        {
            config_set_server_safe(address, port);
        }

    }
    else if (0 == strcmp("your_id", key))
    {
        int iv = atoi(val);
        printf("%s -> %s\n", key, val);
        config_set_myid((uint32_t) iv);
    }
    else if (0 == strcmp("samples_per_measure", key))
    {
        int iv = atoi(val);
        printf("%s -> %s\n", key, val);
        config_set_measure(cfg->measure_period, (uint16_t) iv);
    }
    else if (0 == strcmp("measure_period", key))
    {
        int iv = atoi(val);
        printf("%s -> %s\n", key, val);
        config_set_measure((uint16_t) iv, cfg->samples_per_measure);
    }
    else if (0 == strcmp("measures_per_sleep", key))
    {
        int iv = atoi(val);
        printf("%s -> %s\n", key, val);
        config_set_sleep((uint16_t) iv, cfg->sleep_length);
    }
    else if (0 == strcmp("sleep_length", key))
    {
        int iv = atoi(val);
        printf("%s -> %s\n", key, val);
        config_set_sleep(cfg->measures_per_sleep, (uint16_t) iv);
    }
    else if (0 == strcmp("switch_server", key))
    {
        printf("switch server \n\t (%s %d) <-> (%s %d)\n",
                cfg->server_address, (int) cfg->server_port,
                cfg->fallback_server_address, (int) cfg-> fallback_server_port);
        config_switch_server();
    }
    else if (0 == strcmp("do_ota", key))
    {
        printf("do_ota requested\n");
        CMD_SET(S_CMD_OTA);
    }
    else if (0 == strcmp("dump_cfg", key))
    {
        printf("dump_cfg requested\n");
        CMD_SET(S_CMD_DUMP_CFG);
    }
    else if (0 == strcmp("clear_store", key))
    {
        printf("clear_store requested\n");
        storage_clear();
    }
}

static void dump_config(const GniotConfig_t * cfg)
{
    Request_t request;
    const char * reqs;
    int r = client_open();

    if (!r)
    {
        request_new(&request, "/kloc");
        request_sets(&request, "server_address", cfg->server_address);
        request_seti(&request, "server_port", cfg->server_port);
        request_sets(&request, "fallback_server_address", cfg->fallback_server_address);
        request_seti(&request, "fallback_server_port", cfg->fallback_server_port);
        request_seti(&request, "measure_period", cfg->measure_period);
        request_seti(&request, "measures_per_sleep", cfg->measures_per_sleep);
        request_seti(&request, "samples_per_measure", cfg->samples_per_measure);
        request_seti(&request, "sleep_length", cfg->sleep_length);
        reqs = request_make(&request);
        client_request(reqs, strlen(reqs));
        client_close();
    }
}

void service_send(int connection_status, uint32_t measurement)
{
    bool clear_storage = false;

    storage_sample_start();

    if (connection_status)
    {
        if (NO_MEASUREMENT != measurement)
        {
            StorageSample_t store_sample = {
                    .data = measurement,
                    .ts = get_timestamp(),
            };
            storage_save_sample(&store_sample);
        }
    }
    else
    {
        const GniotConfig_t * cfg = config_get();
        int r;
        int stored_read = 0;
        Request_t request;
        char keybuf[14];
        const char * rs;

        CMD_CLEAR_ALL();

        /* send old samples */
        do
        {
            int si;
            StorageSample_t stored;

            stored_read = storage_next(&stored);

            if (!stored_read)
            {
                clear_storage = true;
                r = client_open();
                request_new(&request, "/kloc");
                sprintf(keybuf, "m_%u", stored.ts);
                request_setu(&request, keybuf, stored.data);

                for (si = 1; !stored_read && si < 10; ++si)
                {
                    stored_read = storage_next(&stored);
                    if (!stored_read)
                    {
                        sprintf(keybuf, "m_%u", stored.ts);
                        request_setu(&request, keybuf, stored.data);
                    }
                }
                rs = request_make(&request);
                r = client_request(rs, strlen(rs));
                if (!r)
                {
                    r = client_response_iterate(command_handler);
                }
                client_close();
            }
        } while (!r && !stored_read);

        if (clear_storage) clear_storage = !r && stored_read;

        if (!r)
        {
            r = client_open();
            request_new(&request, "/kloc");

            if (NO_MEASUREMENT != measurement)
            {
                char keybuf[14];
                sprintf(keybuf, "m_%u", get_timestamp());
                request_setu(&request, keybuf, measurement);
            }

            rs = request_make(&request);

            r = client_request(rs, strlen(rs));
            if (!r)
            {
                r = client_response_iterate(command_handler);

            }
            client_close();
        }


        if (r && (NO_MEASUREMENT != measurement))
        {
            StorageSample_t store_sample = {
                    .data = measurement,
                    .ts = get_timestamp(),
            };
            storage_save_sample(&store_sample);
        }

        if (IS_CMD_SET(S_CMD_OTA))
        {
            printf("Perform OTA\n");
            storage_sample_finish(clear_storage);
            do_ota_upgrade("/ota");
            return;
        }
        if (IS_CMD_SET(S_CMD_DUMP_CFG))
        {
            printf("Dump cfg\n");
            dump_config(cfg);
        }

        CMD_CLEAR_ALL();
    }

    storage_sample_finish(clear_storage);
}

