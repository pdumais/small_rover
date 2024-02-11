#pragma once
#include "common/i2c.h"

void send_to_uart(controller_message *msg);
void uart_stop();
void uart_init();
