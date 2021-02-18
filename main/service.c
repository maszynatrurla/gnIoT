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

#include "service.h"
#include "storage.h"
#include "client.h"

#define DEFAULT_PORT    80

static char request_buf [256];
static char add_buf[18];

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

    if (0 == strcmp("new_server", key))
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
}

void service_send(int connection_status, uint32_t measurement)
{
    const GniotConfig_t * cfg = config_get();

    if (NO_MEASUREMENT == measurement)
    {
        sprintf(request_buf, "GET /kloc?id=%u HTTP/1.0\r\n"
                "Host: 192.168.1.100\r\n"
                "User-Agent: esp-idf/1.0 esp32\r\n"
                "\r\n", cfg->my_id);
    }
    else
    {
        sprintf(request_buf, "GET /kloc?id=%u&m_0=%u HTTP/1.0\r\n"
                "Host: 192.168.1.100\r\n"
                "User-Agent: esp-idf/1.0 esp32\r\n"
                "\r\n", cfg->my_id, measurement);
    }

    int r = client_open();
    if (!r)
    {
        r = client_request(request_buf, strlen(request_buf));
        if (!r)
        {
            r = client_response_iterate(command_handler);
            if (r)
            {
                printf("client_response %d\n", r);
            }

        }
        client_close();
    }
}

