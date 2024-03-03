#include <esp_log.h>
#include <esp_system.h>
#include <esp_event.h>
#include "driver/gpio.h"
#include "pwm.h"
#include "memory.h"

#include "buzzer.h"
#include "hw_def.h"
#include "pwm.h"

static const char *TAG = "buzzer_c";
static QueueHandle_t xQueue;

#define COMMAND_OFF 0
#define COMMAND_ON 1
#define COMMAND_INTERMITENT 2
#define COMMAND_FREQ 3

typedef struct
{
    uint8_t command;
    uint16_t *seq;
    int seq_size;
} q_msg;

void buzzer_task(void *arg)
{
    q_msg msg;

    while (1)
    {
        if (xQueueReceive(xQueue, &msg, 10000 / portTICK_PERIOD_MS))
        {
            if (msg.command == COMMAND_OFF)
            {
                pwm_set_buzzer_freq(0);
            }
            else if (msg.command == COMMAND_ON)
            {
                pwm_set_buzzer_freq(220);
            }
            else if (msg.command == COMMAND_FREQ)
            {
                // TODO: make this configurable
                int i = 0;
                while (!uxQueueMessagesWaiting(xQueue))
                {
                    // ESP_LOGI(TAG, "BUZZER %i %i", msg.seq[i], msg.seq[i + 1]);
                    pwm_set_buzzer_freq(msg.seq[i + 1]);
                    vTaskDelay(msg.seq[i] / portTICK_PERIOD_MS);
                    i += 2;
                    if (i >= (msg.seq_size / 2))
                    {
                        i = 0;
                    }
                }
                free(msg.seq);
            }
        }
    }
}

void buzzer_set_freq(uint16_t *seq, int seq_size)
{
    q_msg msg;
    msg.command = COMMAND_FREQ;
    msg.seq = malloc(seq_size);
    msg.seq_size = seq_size;
    memcpy(msg.seq, seq, seq_size);

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

void buzzer_init()
{
    xQueue = xQueueCreate(4, sizeof(q_msg));
    xTaskCreate(buzzer_task, "buzzer_task", 4096, NULL, 5, NULL);
}