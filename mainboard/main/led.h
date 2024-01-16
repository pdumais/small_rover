#pragma once
#include <esp_system.h>

#define LED_COLORFUL                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
    {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
        {0xf2, 0x45, 0x45}, {0x98, 0xdb, 0x5a}, {0x34, 0xb4, 0x43}, {0x88, 0xf0, 0xb9}, {0xfb, 0x8b, 0x00}, {0xf2, 0x45, 0x45}, {0x98, 0xdb, 0x5a}, {0x34, 0xb4, 0x43}, {0x88, 0xf0, 0xb9}, {0xfb, 0x8b, 0x00}, {0xf2, 0x45, 0x45}, {0x98, 0xdb, 0x5a}, {0x34, 0xb4, 0x43}, {0x88, 0xf0, 0xb9}, {0xfb, 0x8b, 0x00}, {0xf2, 0x45, 0x45}, {0x98, 0xdb, 0x5a}, {0x34, 0xb4, 0x43}, {0x88, 0xf0, 0xb9}, {0xfb, 0x8b, 0x00}, {0xf2, 0x45, 0x45}, {0x98, 0xdb, 0x5a}, {0x34, 0xb4, 0x43}, { 0x88, 0xf0, 0xb9 } \
    }
#define LED_COUNT 24
#define RAINBOW                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
    {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
        {0xCC, 0xFF, 0xFF}, {0x99, 0xFF, 0xCC}, {0x66, 0xFF, 0x99}, {0x33, 0xFF, 0x66}, {0x00, 0xFF, 0x33}, {0xFF, 0xCC, 0x00}, {0xCC, 0xCC, 0x33}, {0x99, 0xCC, 0x66}, {0x66, 0xCC, 0x99}, {0x33, 0xCC, 0xCC}, {0x00, 0xCC, 0xFF}, {0xFF, 0x99, 0xCC}, {0xCC, 0x99, 0x99}, {0x99, 0x99, 0x66}, {0x66, 0x99, 0x33}, {0x33, 0x99, 0x00}, {0x00, 0x99, 0x33}, {0xFF, 0x66, 0x66}, {0xCC, 0x66, 0x99}, {0x99, 0x66, 0xCC}, {0x66, 0x66, 0xFF}, {0x33, 0x66, 0xCC}, {0x00, 0x66, 0x99}, { 0xFF, 0x33, 0x66 } \
    }
#define LED_COLOR_RED                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
    {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
        {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, {0xFF, 0x00, 0x00}, { 0xFF, 0x00, 0x00 } \
    }

#define LED_COLOR_BLUE                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   \
    {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
        {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0xFF}, { 0x00, 0x00, 0xFF } \
    }

struct led_pattern_t
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

typedef struct led_pattern_t led_pattern[LED_COUNT];

void led_init();
void led_set_led(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void led_set_steady_pattern(led_pattern *p);
void led_set_flashing_pattern(led_pattern *p, uint32_t on, uint32_t off);
void led_set_rotating_pattern(led_pattern *p, uint32_t delay);
void led_turnoff();

extern const led_pattern led_pattern_rainbow;
extern const led_pattern led_pattern_red;
extern const led_pattern led_pattern_blue;
