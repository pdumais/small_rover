#pragma once
#include <freertos/FreeRTOS.h>

typedef esp_err_t (*worker_handler_t)(void *arg);

void start_pool();
void stop_pool();
esp_err_t schedule_task(worker_handler_t h, void* arg);