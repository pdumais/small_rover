#pragma once

#define PROV_MODE_NONE 0
#define PROV_MODE_WAP 1

#define MESSAGE_PS5 1
#define MESSAGE_STOP 2
#define MESSAGE_REPLAY_END 3

typedef struct
{
    uint8_t type;
    void *data;
} queue_msg;

void hardware_send_message(queue_msg *msg);
void hardware_init();
void hardware_run();
void hardware_suspend();
void process_throttle(uint8_t t, int8_t direction);
void process_steering(int axis);
