#pragma once
#include <esp_event_base.h>

ESP_EVENT_DECLARE_BASE(GPIO_EVENT);
ESP_EVENT_DECLARE_BASE(PROV_EVENT);

#define TASKPOOL_SIZE 5
#define GPIO_UP 1
#define GPIO_DOWN 2

typedef struct
{
    uint16_t throttle;
    int8_t direction;
    uint16_t pwm;
    int16_t steering_angle;
    uint8_t horn;
    uint16_t rpm1;
    uint16_t rpm2;
    uint16_t rpm3;
    uint16_t rpm4;
    uint16_t speed;
    uint16_t angle_rotator;
    uint16_t angle_boom;
    uint16_t angle_arm;
    uint16_t angle_grapple;
    uint8_t controller_battery;
    uint16_t main_battery;
    uint16_t temperature;
    uint16_t humidity;
    uint16_t pressure;
    int16_t roll;
    int16_t pitch;
    int16_t heading;
} metrics_t;
