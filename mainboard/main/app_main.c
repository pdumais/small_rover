#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "esp_flash.h"
#include <esp_system.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>

#include "common.h"
#include "gpio.h"
#include "wifi.h"

void init_nvs()
{
    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }
}

void app_main(void)
{
    init_nvs();

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    dwn_gpio_init();
    wifi_init();

    // ota_mark_good();

    /*
        bool wifi_is_provisioned = wifi_init();
        switch (get_prog_mode())
        {
        case PROV_MODE_WAP:
            wifi_set_ap();
            wifi_start();
            provision_init();
            break;
        default:
            if (!wifi_is_provisioned)
            {
                wifi_set_ap();
                wifi_start();
                provision_init();
            }
            else
            {
                wifi_set_sta();
                wifi_start();
            }
            break;
        }
    */
    // wait_wifi_connected();
    // start_api();
}
