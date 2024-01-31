#include <esp_log.h>

#include "led.h"
#include "led_strip.h"
#include "hw_def.h"
#include "string.h"

#define COMMAND_STEADY_PATTERN 1
#define COMMAND_FLASHING_PATTERN 2
#define COMMAND_ROTATING_PATTERN 3

typedef struct
{
    uint8_t command;
    union
    {
        struct
        {
            uint8_t on;
            uint8_t off;
        } flash_cmd;
        struct
        {
            uint8_t delay;
            uint8_t data2;
        } rotate_cmd;
    } data;
} q_msg;

static led_strip_handle_t led_strip;
static QueueHandle_t xQueue;
static led_pattern current_pattern;
static const char *TAG = "led_c";

const led_pattern led_pattern_rainbow = RAINBOW;
const led_pattern led_pattern_red = LED_COLOR_RED;
const led_pattern led_pattern_blue = LED_COLOR_BLUE;

void led_set_led(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    q_msg msg;
    msg.command = COMMAND_STEADY_PATTERN;

    memset(&current_pattern, 0, sizeof(led_pattern));
    current_pattern[index].r = r / brightness;
    current_pattern[index].g = g / brightness;
    current_pattern[index].b = b / brightness;

    xQueueSend(xQueue, (void *)&msg, 0);
}

void led_set_steady_pattern(led_pattern *p, uint8_t brightness)
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        current_pattern[i].r = (*p)[i].r / brightness;
        current_pattern[i].g = (*p)[i].g / brightness;
        current_pattern[i].b = (*p)[i].b / brightness;
    }

    q_msg msg;
    msg.command = COMMAND_STEADY_PATTERN;
    xQueueSend(xQueue, (void *)&msg, 0);
}

void led_turnoff()
{
    led_set_led(0, 0, 0, 0, 1);
}

void led_set_rotating_pattern(led_pattern *p, uint32_t delay, uint8_t brightness)
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        current_pattern[i].r = (*p)[i].r / brightness;
        current_pattern[i].g = (*p)[i].g / brightness;
        current_pattern[i].b = (*p)[i].b / brightness;
    }

    q_msg msg;
    msg.command = COMMAND_ROTATING_PATTERN;
    msg.data.rotate_cmd.delay = delay;
    xQueueSend(xQueue, (void *)&msg, 0);
}

void led_set_flashing_pattern(led_pattern *p, uint32_t on, uint32_t off, uint8_t brightness)
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        current_pattern[i].r = (*p)[i].r / brightness;
        current_pattern[i].g = (*p)[i].g / brightness;
        current_pattern[i].b = (*p)[i].b / brightness;
    }

    q_msg msg;
    msg.command = COMMAND_FLASHING_PATTERN;
    msg.data.flash_cmd.on = on;
    msg.data.flash_cmd.off = off;
    xQueueSend(xQueue, (void *)&msg, 0);
}

void led_task(void *arg)
{
    q_msg msg;
    bool blink_state = false;

    while (1)
    {
        ESP_LOGI(TAG, "Checking for LED queue msg");
        if (xQueueReceive(xQueue, &msg, 10000 / portTICK_PERIOD_MS))
        {
            ESP_LOGI(TAG, "LED queue msg received");
            if (msg.command == COMMAND_STEADY_PATTERN)
            {
                for (int i = 0; i < LED_COUNT; i++)
                {
                    led_strip_set_pixel(led_strip, i, current_pattern[i].r, current_pattern[i].g, current_pattern[i].b);
                }
                led_strip_refresh(led_strip);
            }
            else if (msg.command == COMMAND_FLASHING_PATTERN)
            {
                uint8_t op = 0;
                while (!uxQueueMessagesWaiting(xQueue))
                {
                    if (op == 0)
                    {
                        for (int i = 0; i < LED_COUNT; i++)
                        {
                            led_strip_set_pixel(led_strip, i, current_pattern[i].r, current_pattern[i].g, current_pattern[i].b);
                        }
                        led_strip_refresh(led_strip);
                        op = 1;
                        vTaskDelay(msg.data.flash_cmd.on / portTICK_PERIOD_MS);
                    }
                    else
                    {
                        led_strip_clear(led_strip);
                        op = 0;
                        vTaskDelay(msg.data.flash_cmd.off / portTICK_PERIOD_MS);
                    }
                }
            }
            else if (msg.command == COMMAND_ROTATING_PATTERN)
            {
                int offset = 0;
                while (!uxQueueMessagesWaiting(xQueue))
                {
                    int n = offset;
                    for (int i = 0; i < LED_COUNT; i++)
                    {
                        led_strip_set_pixel(led_strip, i, current_pattern[n].r, current_pattern[n].g, current_pattern[n].b);
                        n++;
                        if (n >= LED_COUNT)
                        {
                            n = 0;
                        }
                    }
                    led_strip_refresh(led_strip);
                    vTaskDelay(msg.data.rotate_cmd.delay / portTICK_PERIOD_MS);
                    offset++;
                    if (offset >= LED_COUNT)
                    {
                        offset = 0;
                    }
                }
            }
        }
    }
}

void led_init()
{
    xQueue = xQueueCreate(4, sizeof(q_msg));

    led_strip_config_t strip_config = {
        .strip_gpio_num = GPIO_OUTPUT_LED,
        .max_leds = LED_COUNT,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    xTaskCreate(led_task, "led_task", 4096, NULL, 5, NULL);
}
