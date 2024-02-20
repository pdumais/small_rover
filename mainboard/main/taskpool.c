#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>

#include <esp_log.h>
#include <esp_system.h>
#include "taskpool.h"
#include "common.h"


static QueueHandle_t worker_queue;
static SemaphoreHandle_t worker_ready_count;
static TaskHandle_t worker_handles[TASKPOOL_SIZE];

static const char *TAG = "taskpool_c";

typedef struct {
    void* arg;
    worker_handler_t handler;
} worker_t;


static void worker_task(void *p)
{
    ESP_LOGI(TAG, "starting async req task worker");

    while (true) {
        xSemaphoreGive(worker_ready_count);

        worker_t w;
        if (xQueueReceive(worker_queue, &w, 10)) {
            w.handler(w.arg);
        }
    }

    ESP_LOGW(TAG, "worker stopped");
    vTaskDelete(NULL);
}

void start_pool()
{
    worker_ready_count = xSemaphoreCreateCounting(TASKPOOL_SIZE, 0);
    worker_queue = xQueueCreate(1, sizeof(worker_t));

    for (int i = 0; i < TASKPOOL_SIZE; i++) 
    {
        xTaskCreate(worker_task, "async_req_worker",4096, 0 , 5, &worker_handles[i]);
    }
}

void stop_pool()
{
    //TODO
}

esp_err_t schedule_task(worker_handler_t h, void* arg)
{
    int ticks = 0;
    worker_t w = {
        .handler = h,
        .arg = arg,
    };

    if (xSemaphoreTake(worker_ready_count, ticks) == false) {
        ESP_LOGE(TAG, "No workers are available");
        return ESP_FAIL;
    }

    if (xQueueSend(worker_queue, &w, pdMS_TO_TICKS(100)) == false) {
        ESP_LOGE(TAG, "worker queue is full");
        return ESP_FAIL;
    }

    return ESP_OK;
}