#include <esp_event.h>
#include <esp_system.h>
#include "hardware.h"
#include "wifi.h"
#include "api.h"
#include <nvs_flash.h>

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
    hardware_init();
    wifi_init();
    start_api();

    hardware_run();
}
