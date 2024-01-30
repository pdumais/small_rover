#pragma once

#define PROV_MODE_NONE 0
#define PROV_MODE_WAP 1

void dwn_gpio_init();
void gpio_suspend();
void process_throttle(uint8_t t, int8_t direction);
void process_steering(int axis);
