#pragma once
#define ESP_SLAVE_ADDR 0x28

typedef struct __attribute((packed))
{
    uint8_t command;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
} i2c_slave_message;
