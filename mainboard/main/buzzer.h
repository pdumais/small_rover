#pragma once
#include <esp_system.h>

void buzzer_init();
void buzzer_set_on();
void buzzer_set_off();
void buzzer_set_intermitent(uint32_t on, uint32_t off);
