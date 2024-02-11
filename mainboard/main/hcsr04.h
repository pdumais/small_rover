#pragma once
#include <esp_system.h>
#include <driver/rmt.h>
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"

typedef struct
{
    uint8_t trigger_pin;
    uint8_t echo_pin;
    uint16_t distance;
    rmt_channel_handle_t tx_channel;
    rmt_channel_handle_t rx_channel;
    rmt_encoder_handle_t encoder;
} hcsr04_handle_t;

void hcsr04_init(uint8_t trigger, uint8_t echo, hcsr04_handle_t *h);
uint16_t hcsr04_read(hcsr04_handle_t *h, uint32_t wait_time);
void hcsr04_trigger(hcsr04_handle_t *h);
