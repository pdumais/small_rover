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
#include "ps5_controller.h"

#define DUALSENSE_MAC "D0:BC:C1:EC:AD:AC"
#define GPIO_CTRL_LED 5

#define DATA_LENGTH sizeof(ps5_t)
#define I2C_SLAVE_SCL_IO 22
#define I2C_SLAVE_SDA_IO 23
#define I2C_SLAVE_NUM 0
#define I2C_SLAVE_TX_BUF_LEN (8 * DATA_LENGTH)
#define I2C_SLAVE_RX_BUF_LEN (8 * DATA_LENGTH)
#define ESP_SLAVE_ADDR 0x28

static const char *TAG = "ps5_main";

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

void ps5_check(void *arg)
{
    ps5_t old_data;
    while (1)
    {
        gpio_set_level(GPIO_CTRL_LED, ps5_isConnected() ? 1 : 0);

        while (!ps5_isConnected())
        {
            ESP_LOGI(TAG, "Trying to connect to controller");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        ps5_t *data = ps5_get_data();

        if (
            data->button.right != old_data.button.right ||
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
            abs(data->analog.button.r2 - old_data.analog.button.r2) > 2)
        {
            ESP_LOGI(TAG, "PS5 Event");
            size_t d_size = i2c_slave_write_buffer(I2C_SLAVE_NUM, data, sizeof(data), 1000 / portTICK_PERIOD_MS);
            if (d_size == 0)
            {
                ESP_LOGW(TAG, "i2c slave tx buffer full");
            }
        }

        memcpy(&old_data, data, sizeof(ps5_t));

        vTaskDelay(100 / portTICK_PERIOD_MS); // Debounce to 100ms
    }
}

static esp_err_t i2c_slave_init(void)
{
    int i2c_slave_port = I2C_SLAVE_NUM;
    i2c_config_t conf_slave = {
        .sda_io_num = I2C_SLAVE_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_SLAVE_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .mode = I2C_MODE_SLAVE,
        .slave.addr_10bit_en = 0,
        .slave.slave_addr = ESP_SLAVE_ADDR,
    };
    esp_err_t err = i2c_param_config(i2c_slave_port, &conf_slave);
    if (err != ESP_OK)
    {
        return err;
    }
    return i2c_driver_install(i2c_slave_port, conf_slave.mode, I2C_SLAVE_RX_BUF_LEN, I2C_SLAVE_TX_BUF_LEN, 0);
}

void app_main(void)
{
    init_nvs();

    gpio_config_t o_conf = {};
    o_conf.pin_bit_mask |= (1ULL << GPIO_CTRL_LED);
    o_conf.intr_type = GPIO_INTR_DISABLE;
    o_conf.mode = GPIO_MODE_OUTPUT;
    o_conf.pull_up_en = 1;
    o_conf.pull_down_en = 0;
    gpio_config(&o_conf);

    ESP_ERROR_CHECK(i2c_slave_init());
    ps5_begin(DUALSENSE_MAC);
    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    xTaskCreate(ps5_check, "ps5_check", 4096, NULL, 5, NULL);
}
