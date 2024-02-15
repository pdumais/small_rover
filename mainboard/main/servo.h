#pragma once
#include <esp_system.h>
#include "driver/mcpwm_prelude.h"
#include "driver/mcpwm_timer.h"

void servo_init();
void servo_start();
void servo_create(uint8_t pin, mcpwm_cmpr_handle_t *comparator);
void servo_set_angle(mcpwm_cmpr_handle_t *comparator, int angle);
void servo_enable(mcpwm_cmpr_handle_t *comparator, bool on);