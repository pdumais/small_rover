#pragma once
#include <esp_system.h>

void laser_init();
void laser_set_position(int8_t horizontal_angle, int8_t vertical_angle); // -127 .. +127
void laser_trigger(bool on);
bool laser_is_triggered();
