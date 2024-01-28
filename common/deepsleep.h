#pragma once
#include <esp_system.h>

typedef void (*sleep_callback)();

void init_deep_sleep(uint8_t sleep_button, sleep_callback cb);
