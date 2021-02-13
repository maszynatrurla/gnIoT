/*
 * Main user task, entry point for application.
 * Based on hello-world template from ESP8266_RTOS_SDK.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "humtemp.h"

/**
 * Basic test of humtemp module.
 * Read temparature and humidity, print them to console
 * and sleep 3 seconds. Repeat 100 times (for a 5 minute test).
 */
static void ht_test(void)
{
    int idx;

    humtemp_init();

    for (idx = 0; idx < 100; ++idx)
    {
        Humidity_t humidity;
        Temperature_t temperature;
        int result;

        result = humtemp_read(&humidity, &temperature);

        if (0 == result)
        {
            printf("%d.%d C  %d.%d RH\n", (int) temperature.integer,
                    (int) temperature.decimal, (int) humidity.integer,
                    (int) humidity.decimal);
            fflush(stdout);
        }
        else
        {
            printf("err %d\n", result);
        }


        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }

}

void app_main()
{
    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP8266 chip with %d CPU cores, WiFi, ",
            chip_info.cores);

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    ht_test();


    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
