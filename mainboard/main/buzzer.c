#include <esp_log.h>
#include <esp_system.h>
#include <esp_event.h>
#include "driver/gpio.h"

#include "buzzer.h"
#include "hw_def.h"

static const char *TAG = "buzzer_c";
static QueueHandle_t xQueue;

#define COMMAND_OFF 0
#define COMMAND_ON 1
#define COMMAND_INTERMITENT 2

typedef struct
{
    uint8_t command;
    union
    {
        uint32_t on;
        uint32_t off;
    } flash_cmd;
} q_msg;

void buzzer_task(void *arg)
{
    q_msg msg;
    bool blink_state = false;

    while (1)
    {
        ESP_LOGI(TAG, "Checking for BUZZER queue msg");
        if (xQueueReceive(xQueue, &msg, 10000 / portTICK_PERIOD_MS))
        {
            ESP_LOGI(TAG, "LED queue msg received");
            if (msg.command == COMMAND_OFF)
            {
                gpio_set_level(GPIO_BUZZER1, 0);
            }
            else if (msg.command == COMMAND_ON)
            {
                gpio_set_level(GPIO_BUZZER1, 1);
            }
            else if (msg.command == COMMAND_INTERMITENT)
            {
                uint8_t op = 0;
                while (!uxQueueMessagesWaiting(xQueue))
                {
                    if (op == 0)
                    {
                        gpio_set_level(GPIO_BUZZER1, 1);
                        op = 1;
                        vTaskDelay(msg.flash_cmd.on / portTICK_PERIOD_MS);
                    }
                    else
                    {
                        gpio_set_level(GPIO_BUZZER1, 0);
                        op = 0;
                        vTaskDelay(msg.flash_cmd.off / portTICK_PERIOD_MS);
                    }
                }
            }
        }
    }
}

void buzzer_set_on()
{
    q_msg msg;
    msg.command = COMMAND_ON;

    xQueueSend(xQueue, (void *)&msg, 0);
}

void buzzer_set_off()
{
    q_msg msg;
    msg.command = COMMAND_OFF;

    xQueueSend(xQueue, (void *)&msg, 0);
}

void buzzer_set_intermitent(uint32_t on, uint32_t off)
{
    q_msg msg;
    msg.command = COMMAND_INTERMITENT;
    msg.flash_cmd.on = on;
    msg.flash_cmd.off = off;
    xQueueSend(xQueue, (void *)&msg, 0);
}

void buzzer_init()
{
    xQueue = xQueueCreate(4, sizeof(q_msg));

    gpio_config_t o_conf = {};
    o_conf.pin_bit_mask |= (1ULL << GPIO_BUZZER1);
    o_conf.intr_type = GPIO_INTR_DISABLE;
    o_conf.mode = GPIO_MODE_OUTPUT;
    o_conf.pull_up_en = 0;
    o_conf.pull_down_en = 1;
    gpio_config(&o_conf);

    xTaskCreate(buzzer_task, "buzzer_task", 4096, NULL, 5, NULL);
}