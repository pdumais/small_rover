#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "ps5_controller.h"
#include "common/i2c.h"
#include "common/deepsleep.h"

#define DUALSENSE_MAC "D0:BC:C1:EC:AD:AC"
#define GPIO_CTRL_LED 5
#define GPIO_PAIR 18
#define GPIO_SLEEP_BUTTON GPIO_NUM_4

#define I2C_MASTER_NUM 0
#define I2C_SLAVE_SCL_IO 22
#define I2C_SLAVE_SDA_IO 23

static const char *TAG = "ps5_main";
static volatile ps5_t old_data = {0};

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

static esp_err_t i2c_init(void)
{
    ESP_LOGI(TAG, "i2c_init");
    int i2c_master_port = 0;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SLAVE_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_SLAVE_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
        .clk_flags = 0,
    };

    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

void set_checksum(ps5_t *data)
{
    data->checksum = 0;
    uint8_t c = 0;
    for (int i = 0; i < sizeof(ps5_t) - 1; i++)
    {
        c += ((uint8_t *)data)[i];
    }
    data->checksum = c;
}

void block_trigger(int side, bool state)
{
    ps5_trigger_effect(0, state ? 1 : 0);
}

void rumble(bool state)
{
    if (state)
    {
        ps5_set_rumble(255, 255);
    }
    else
    {
        ps5_set_rumble(0, 0);
    }
}

void set_pad_led(uint8_t r, uint8_t g, uint8_t b)
{
    ps5_set_led(r, g, b);
}

void ps5_check(void *arg)
{
    bool was_connected = 0;

    while (1)
    {

        if (!ps5_isConnected())
        {
            if (was_connected)
            {
                ESP_LOGI(TAG, "Controller disconnected");
                was_connected = false;
                ps5_t *data = ps5_get_data();
                data->controller_connected = 0;
                data->latestPacket = 1;
                set_checksum(data);
                i2c_master_write_to_device(I2C_MASTER_NUM, ESP_SLAVE_ADDR, data, sizeof(ps5_t), 1000);
            }

            gpio_set_level(GPIO_CTRL_LED, 0);
            ESP_LOGI(TAG, "Not connected to PS5 controller");
            vTaskDelay(3000 / portTICK_PERIOD_MS);
            continue;
        }

        // At this point, we know the ps5 controller is connected
        if (!was_connected)
        {
            ESP_LOGI(TAG, "Controller Connected");
            was_connected = false;
            ps5_t *data = ps5_get_data();
            data->controller_connected = 1;
            data->latestPacket = 1;
            set_checksum(data);
            i2c_master_write_to_device(I2C_MASTER_NUM, ESP_SLAVE_ADDR, data, sizeof(ps5_t), 1000);

            was_connected = 1;
        }

        gpio_set_level(GPIO_CTRL_LED, 1);
        ps5_t *data = ps5_get_data();

        bool has_changes = (data->button.right != old_data.button.right ||
                            data->button.left != old_data.button.left ||
                            data->button.up != old_data.button.up ||
                            data->button.down != old_data.button.down ||
                            data->button.square != old_data.button.square ||
                            data->button.circle != old_data.button.circle ||
                            data->button.cross != old_data.button.cross ||
                            data->button.triangle != old_data.button.triangle ||
                            data->button.l1 != old_data.button.l1 ||
                            data->button.r1 != old_data.button.r1 ||
                            data->button.l2 != old_data.button.l2 ||
                            data->button.r2 != old_data.button.r2 ||
                            data->button.l3 != old_data.button.l3 ||
                            data->button.r3 != old_data.button.r3 ||
                            abs(data->analog.stick.lx - old_data.analog.stick.lx) > 2 ||
                            abs(data->analog.stick.ly - old_data.analog.stick.ly) > 2 ||
                            abs(data->analog.stick.rx - old_data.analog.stick.rx) > 2 ||
                            abs(data->analog.stick.ry - old_data.analog.stick.ry) > 2 ||
                            abs(data->analog.button.l2 - old_data.analog.button.l2) > 2 ||
                            abs(data->analog.button.r2 - old_data.analog.button.r2) > 2);

        if (has_changes)
        {

            if (data->button.up)
            {
                ESP_LOGI(TAG, "Button UP");
                block_trigger(0, true);
            }
            else
            {
                block_trigger(0, false);
            }
            /*if (data->button.down)
            {
                set_pad_led(0, 255, 0);
            }
            else
            {
                set_pad_led(0, 255, 255);
            }*/

            ESP_LOGI(TAG, "PS5 Event");

            memcpy(&old_data, data, sizeof(ps5_t));
            old_data.latestPacket = true;
            old_data.controller_connected = true;

            set_checksum(&old_data);

            esp_err_t err = i2c_master_write_to_device(I2C_MASTER_NUM, ESP_SLAVE_ADDR, &old_data, sizeof(ps5_t), 1000);
            if (err != ESP_OK)
            {
                if (err == ESP_ERR_INVALID_STATE)
                {
                    ESP_LOGE(TAG, "i2c bus send: ESP_ERR_INVALID_STATE");
                    /*//This is just temporary to help debug
                    while (1)
                    {
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                    } */
                }
                ESP_LOGE(TAG, "Error sending on i2c bus: %i", err);
            }
        }

        vTaskDelay(50 / portTICK_PERIOD_MS); // Debounce
    }
}

void on_start_sleep()
{
    gpio_set_level(GPIO_CTRL_LED, 0);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_CTRL_LED, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_CTRL_LED, 0);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_CTRL_LED, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_CTRL_LED, 0);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_CTRL_LED, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_CTRL_LED, 0);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_CTRL_LED, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_CTRL_LED, 0);
}

void app_main(void)
{
    init_nvs();

    init_deep_sleep(GPIO_SLEEP_BUTTON, on_start_sleep);

    gpio_config_t i_conf = {};
    i_conf.pin_bit_mask = 0;
    i_conf.pin_bit_mask |= (1ULL << GPIO_PAIR);
    i_conf.intr_type = GPIO_INTR_DISABLE;
    i_conf.mode = GPIO_MODE_INPUT;
    i_conf.pull_up_en = 1;
    gpio_config(&i_conf);

    gpio_config_t o_conf = {};
    o_conf.pin_bit_mask |= (1ULL << GPIO_CTRL_LED);
    o_conf.intr_type = GPIO_INTR_DISABLE;
    o_conf.mode = GPIO_MODE_OUTPUT;
    o_conf.pull_up_en = 1;
    o_conf.pull_down_en = 0;
    gpio_config(&o_conf);

    ESP_ERROR_CHECK(i2c_init());
    ps5_begin(DUALSENSE_MAC);
    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    if (gpio_get_level(GPIO_PAIR) == 0)
    {
        ESP_LOGI(TAG, "Waiting to pair");
        ps5_try_pairing();
    }

    xTaskCreate(ps5_check, "ps5_check", 4096, NULL, 5, NULL);
}
