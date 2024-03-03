#pragma once
#include <esp_system.h>

#define BUZZER_CHIME_CUCARACHA()                                                                                                                                                                                                                                                                           \
    {                                                                                                                                                                                                                                                                                                      \
        100, 587, 50, 0, 100, 587, 50, 0, 100, 587, 50, 0, 250, 784, 50, 0, 600, 988, 100, 0, 100, 587, 50, 0, 100, 587, 50, 0, 100, 587, 50, 0, 250, 784, 50, 0, 600, 988, 100, 0, 100, 987, 50, 0, 100, 987, 50, 0, 100, 880, 50, 0, 100, 880, 50, 0, 100, 784, 50, 0, 100, 784, 50, 0, 100, 740, 500, 0 \
    }

void buzzer_init();
void buzzer_set_on();
void buzzer_set_off();
void buzzer_set_freq(uint16_t *seq, int seq_size);
void buzzer_set_intermitent(uint32_t on, uint32_t off);
