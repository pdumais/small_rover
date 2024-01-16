#ifndef ps5_H
#define ps5_H

#include <stdbool.h>
#include <stdint.h>
#include "common/ps5_data.h"


/***************************/
/*    C A L L B A C K S    */
/***************************/

typedef void (*ps5_connection_callback_t)(uint8_t isConnected);

typedef void (*ps5_event_callback_t)(ps5_t ps5, ps5_event_t event);

/********************************************************************************/
/*                             F U N C T I O N S */
/********************************************************************************/

bool ps5IsConnected();
void ps5Init();
void ps5Enable();
void ps5Cmd(ps5_cmd_t ps5_cmd);
void ps5SetConnectionCallback(ps5_connection_callback_t cb);
void ps5SetEventCallback(ps5_event_callback_t cb);
void ps5SetLed(uint8_t r, uint8_t g, uint8_t b);
void ps5SetOutput(ps5_cmd_t prev_cmd);
void ps5SetBluetoothMacAddress(const uint8_t *mac);
long ps5_l2cap_connect(uint8_t addr[6]);
long ps5_l2cap_reconnect(void);

#endif
