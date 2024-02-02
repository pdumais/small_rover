#ifndef ps5_H
#define ps5_H

#include <stdbool.h>
#include <stdint.h>
#include "common/ps5_data.h"
#define DS5_FLAG0_COMPATIBLE_VIBRATION BIT(0)
#define DS5_FLAG0_HAPTICS_SELECT BIT(1)
#define DS5_FLAG1_LIGHTBAR BIT(2)
#define DS5_FLAG1_PLAYER_LED BIT(4)
#define DS5_FLAG2_LIGHTBAR_SETUP_CONTROL_ENABLE BIT(1)
#define DS5_LIGHTBAR_SETUP_LIGHT_OUT BIT(1)
#define DS5_FLAG2_LIGHTBAR_SETUP_CONTROL_ENABLE BIT(1)
#define DS_OUTPUT_VALID_FLAG0_RIGHT_TRIGGER_MOTOR_ENABLE BIT(2)
#define DS_OUTPUT_VALID_FLAG0_LEFT_TRIGGER_MOTOR_ENABLE BIT(3)
#define DS_OUTPUT_VALID_FLAG0_HEADPHONE_VOLUME_ENABLE BIT(4)

typedef struct __attribute((packed))
{
    uint8_t transaction_type;
    uint8_t report_id; /* 0x31 */
    uint8_t seq_tag;
    uint8_t tag;

    // Common to Bluetooth and USB (although we don't support USB).
    uint8_t valid_flag0;
    uint8_t valid_flag1;

    /* For DualShock 4 compatibility mode. */
    uint8_t motor_right;
    uint8_t motor_left;

    /* Audio controls */
    uint8_t reserved1[4];
    uint8_t mute_button_led; // 10

    uint8_t power_save_control;
    uint8_t right_trigger_motor_mode;
    uint8_t right_trigger_param[10];

    /* right trigger motor */
    uint8_t left_trigger_motor_mode;
    uint8_t left_trigger_param[10];
    uint8_t reserved2[6];

    /* LEDs and lightbar */
    uint8_t valid_flag2; // 40
    uint8_t reserved3[2];
    uint8_t lightbar_setup;
    uint8_t led_brightness; // 44
    uint8_t player_leds;
    uint8_t lightbar_red; // 46
    uint8_t lightbar_green;
    uint8_t lightbar_blue;

    //
    uint8_t reserved4[24]; // 49
    uint32_t crc32;
} ps5_output_report_t;

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
void ps5_send_output_report(ps5_output_report_t *out);

#endif
