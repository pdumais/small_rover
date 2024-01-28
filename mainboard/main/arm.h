#pragma once
#include <esp_system.h>


void arm_move(int8_t axis1, int8_t axis2, int8_t axis3, int8_t axis4);
void arm_enable(bool enabled);
void arm_init();
