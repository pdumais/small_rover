#include "common/deepsleep.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include <esp_event.h>
#include <esp_log.h>

static const char *TAG = "deepsleep_c";

static struct
{
    uint8_t sleep_button;
    sleep_callback cb;
} sleep_info;

static void check_sleep(void *arg)
{
    // unsigned long last_rpm_check = (esp_timer_get_time() / 1000ULL);
    while (1)
    {
        //  Check counter every second
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Since this is a timed thread, we'll piggy back here to verify the value of the sleep button
        if (gpio_get_level(sleep_info.sleep_button) == 1)
        {
            ESP_LOGI(TAG, "Going into deep sleep");
            if (sleep_info.cb)
            {
                sleep_info.cb();
            }
            esp_deep_sleep_start();
        }
    }
}

void init_deep_sleep(uint8_t sleep_button, sleep_callback cb)
{
    sleep_info.sleep_button = sleep_button;
    sleep_info.cb = cb;

    ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(sleep_button, 1));
    ESP_ERROR_CHECK(rtc_gpio_pullup_dis(sleep_button));
    ESP_ERROR_CHECK(rtc_gpio_pulldown_en(sleep_button));

    gpio_config_t i_conf = {};
    i_conf.intr_type = GPIO_INTR_DISABLE;
    i_conf.mode = GPIO_MODE_INPUT;
    i_conf.pull_up_en = 0;
    i_conf.pull_down_en = 1;
    i_conf.pin_bit_mask = 1 << sleep_button;
    ESP_ERROR_CHECK(gpio_config(&i_conf));

    if (gpio_get_level(sleep_button) == 1)
    {
        ESP_LOGI(TAG, "SLEEP button being pressed while chip is booting. Maybe it's a rebound from button release to go to sleep");
        while (gpio_get_level(sleep_button) == 1)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Going back to deep sleep");

        esp_deep_sleep_start();
    }

    xTaskCreate(check_sleep, "poll_ps5", 2048, NULL, 5, 0);
}