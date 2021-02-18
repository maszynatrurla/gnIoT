/*
 * Main user task, entry point for application.
 * Based on hello-world template from ESP8266_RTOS_SDK.
 */


#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_sleep.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "storage.h"
#include "measurements.h"
#include "humtemp.h"
#include "wifi.h"
#include "client.h"
#include "service.h"

#define MEAS_FAILED     0xFFFFFFFF
#define MEAS_FINISHED   0xFFFFFFF0

static xQueueHandle s_measurement_queue = NULL;

static void debug_hello(void)
{
    esp_chip_info_t chip_info;

    printf("\nGNIOT MALY\n\n");

    esp_chip_info(&chip_info);
    printf("This is ESP8266 chip with %d CPU cores, WiFi, ",
            chip_info.cores);
    printf("silicon revision %d, ", chip_info.revision);
    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
}

static void measurements_task(void * arg)
{
    const GniotConfig_t * cfg = config_get();
    int sidx;
    int total_reads = 0;
    uint32_t meas;
    int result;
    int i = 0;

    humtemp_init();

    while (1)
    {
        // re-apply config (in case it changed in the meantime)
        // re-init measurement data
        measurement_init(cfg);
        total_reads = 0;

        // get a few samples from hum+temp sensor,
        // in case of failure try again, but not more
        // than 2 times planned amount
        for (sidx = 0; (sidx < cfg->samples_per_measure)
                && (total_reads < (cfg->samples_per_measure) * 2);
                ++total_reads)
        {
            Humidity_t h;
            Temperature_t t;

            if (0 == humtemp_read(&h, &t))
            {
                printf("%d.%d C %d.%d rh\n", (int) t.integer, (int) t.decimal,
                        (int) h.integer, (int) h.decimal);
                measurement_add_sample(h, t);
                ++sidx;
            }
            vTaskDelay(1500 / portTICK_PERIOD_MS);
        }

        // get final value of measurement
        // we choose median value of samples, to get rid of
        // outliers
        result = measurement_get(&meas);

        // regardless if measurement was successful or not,
        // we will notify main task, that measurement has taken
        // place
        if (!result)
        {
            xQueueSendToBack(s_measurement_queue, &meas, 0);
        }
        else
        {
            uint32_t dummy = MEAS_FAILED;
            xQueueSendToBack(s_measurement_queue, &dummy, 0);
        }

        ++i;

        if (cfg->measures_per_sleep && (i >= cfg->measures_per_sleep))
        {
            break;
        }

        vTaskDelay((1000 * (cfg->measure_period)) / portTICK_PERIOD_MS);
    }

    // send message to main task, that all planned measurements have
    // ended and sleep can be started
    {
        uint32_t dummy = MEAS_FINISHED;
        xQueueSendToBack(s_measurement_queue, &dummy, 0);
    }

    // infinite wait loop
    // we expect that uC goes to sleep now
    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    int conn_result;
    int sleep_min;

    /* Print chip information */
    debug_hello();

    /* read nonvolatile data */
    storage_init();
    config_init();

    /* create a queue for intertask comm */
    s_measurement_queue = xQueueCreate(10, sizeof(uint32_t));
    /* start measurements task */
    xTaskCreate(measurements_task, "measurements_task", 2048, NULL, 10, NULL);

    conn_result = wifi_connect();

    if (0 == conn_result)
    {
        printf("Connected to wifi. IP address = %s\n", wifi_getIpAddress());
    }

    while (1)
    {
        uint32_t meas;

        if (xQueueReceive(s_measurement_queue, &meas, portMAX_DELAY))
        {
            if (MEAS_FAILED == meas)
            {
                // no measurement was taken (failure) but
                // we will try to send message to server anyway
                // to show that we are alive
                printf("Measurement failed\n");
                service_send(conn_result, NO_MEASUREMENT);
            }
            else if (MEAS_FINISHED == meas)
            {
                // leave loop - go to sleep
                break;
            }
            else
            {
                // send message to server
                service_send(conn_result, meas);
            }
        }
    }

    wifi_disconnect();

    // deep sleep - turn everything off except from RTC
    // requires physical connection of WAKE pin with RST pin!
    sleep_min = config_get()->sleep_length;
    printf("Going to sleep for %d minutes\n", sleep_min);
    fflush(stdout);
    esp_deep_sleep(sleep_min * 60 * 1000000);

    // code below should not be executed anymore
    for (int i = 10; i >= 0; i--)
    {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
