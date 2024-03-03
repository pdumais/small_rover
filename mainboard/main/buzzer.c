#include <esp_log.h>
#include <esp_system.h>
#include <esp_event.h>
#include "driver/gpio.h"

#include "buzzer.h"
#include "hw_def.h"
#include "driver/ledc.h"

static const char *TAG = "buzzer_c";
static QueueHandle_t xQueue;

#define COMMAND_OFF 0
#define COMMAND_ON 1
#define COMMAND_INTERMITENT 2
#define COMMAND_FREQ 3

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
                ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1);
            }
            else if (msg.command == COMMAND_ON)
            {
                ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1);
                ESP_ERROR_CHECK(ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1, 220));
            }
            else if (msg.command == COMMAND_FREQ)
            {
                // TODO: make this configurable
                int f[] = {220, 440, 880};
                int i = 0;
                ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1);
                while (!uxQueueMessagesWaiting(xQueue))
                {
                    ESP_ERROR_CHECK(ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1, f[i]));
                    i++;
                    if (i >= (sizeof(f) / sizeof(int)))
                    {
                        i = 0;
                    }
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                }
            }
            /*else if (msg.command == COMMAND_INTERMITENT)
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
            }*/
        }
    }
}

void buzzer_set_freq(uint32_t f)
{
    q_msg msg;
    msg.command = COMMAND_FREQ;
    msg.flash_cmd.on = f;

    xQueueSend(xQueue, (void *)&msg, 0);
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

    // Don't let the naming fool you. LEDC is not for LED control. It is the name of the peripheral
    // in the ESP32 for controlling a LED but it is really just a PWM controller
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = 10,
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_2,
        .timer_sel = LEDC_TIMER_1,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GPIO_BUZZER1,
        .duty = 75,
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    xTaskCreate(buzzer_task, "buzzer_task", 4096, NULL, 5, NULL);
}