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
#include <esp_timer.h>

#define DUALSENSE_MAC "D0:BC:C1:EC:AD:AC"
#define GPIO_CTRL_LED 5
#define GPIO_PAIR 18
#define GPIO_SLEEP_BUTTON GPIO_NUM_4

#define I2C_MASTER_NUM 0
#define I2C_SLAVE_SCL_IO 22
#define I2C_SLAVE_SDA_IO 23

#define WRITE_BIT I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ   /*!< I2C master read */
#define ACK_CHECK_EN 0x1           /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0          /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                /*!< I2C ack value */
#define NACK_VAL 0x1               /*!< I2C nack value */
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
        //.master.clk_speed = 10000,
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

static esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint8_t *data_rd, size_t size)
{
    if (size == 0)
    {
        return ESP_OK;
    }

    uint8_t addr = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data_rd, (i2c_ack_type_t)ACK_VAL);
    i2c_master_read_byte(cmd, data_rd + 1, (i2c_ack_type_t)ACK_VAL);
    i2c_master_read_byte(cmd, data_rd + 2, (i2c_ack_type_t)ACK_VAL);
    i2c_master_read_byte(cmd, data_rd + 3, (i2c_ack_type_t)I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 10);
    i2c_cmd_link_delete(cmd);

    // When no data is available, it seems the buffer is filled with the read request and then 0xFF
    // I could confirm that on the oscilloscope. I have no idea why that is the case.
    // I would simply expect i2c_master_cmd_begin to timeout instead
    if (data_rd[0] == ((ESP_SLAVE_ADDR << 1) | READ_BIT))
    {
        vTaskDelay(25 / portTICK_PERIOD_MS);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "%02X %02X %02X %02X", (int)data_rd[0], (int)data_rd[1], (int)data_rd[2], (int)data_rd[3]);
    return ret;
}

static unsigned long millis()
{
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

void ps5_check(void *arg)
{
    bool was_connected = 0;

    unsigned long current_millis = millis();
    unsigned long turn_off_rumble_millis = 0;

    while (1)
    {
        current_millis = millis();
        if (turn_off_rumble_millis != 0 && turn_off_rumble_millis < current_millis)
        {
            ESP_LOGI(TAG, "UNSET RUMBLE %lu %lu", current_millis, turn_off_rumble_millis);
            ps5_set_rumble(0, 0);
            turn_off_rumble_millis = 0;
        }

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
                }
                ESP_LOGE(TAG, "Error sending on i2c bus: %i", err);
            }
        }

        i2c_slave_message slave_msg = {0};
        esp_err_t err = i2c_master_read_slave(I2C_MASTER_NUM, &slave_msg, sizeof(i2c_slave_message));
        if (err == ESP_OK && slave_msg.command != 0)
        {
            if (slave_msg.command == 0x01)
            {
                ps5_set_led(slave_msg.data1, slave_msg.data2, slave_msg.data3);
            }
            else if (slave_msg.command == 0x02)
            {
                ps5_trigger_effect(slave_msg.data1, slave_msg.data2, slave_msg.data3);
            }
            else if (slave_msg.command == 0x03)
            {
                ps5_set_rumble(255, 255);
                turn_off_rumble_millis = millis() + ((unsigned long)slave_msg.data1 * 1000ULL);
                ESP_LOGI(TAG, "SET RUMBLE %lu %lu", current_millis, turn_off_rumble_millis);
            }
        }
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
    i_conf.pull_up_en = 0;
    i_conf.pull_down_en = 1;
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

    if (gpio_get_level(GPIO_PAIR) == 1)
    {
        ESP_LOGI(TAG, "Waiting to pair");
        ps5_try_pairing();
    }

    xTaskCreate(ps5_check, "ps5_check", 4096, NULL, 5, NULL);
}
