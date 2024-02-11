#pragma once
#include <stdint.h>
typedef struct __attribute((packed))
{
    uint8_t command;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
} controller_message;
