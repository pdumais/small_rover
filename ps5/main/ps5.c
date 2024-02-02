#include "ps5.h"

#include <esp_system.h>
#include <esp_mac.h>
#include <string.h>
#include <esp_log.h>
#include "rom/crc.h"

#include "ps5_int.h"

static const char *TAG = "ps5_c";
/********************************************************************************/
/*                              C O N S T A N T S */
/********************************************************************************/

// static const uint8_t hid_cmd_payload_ps5_enable[] = {0x43, 0x02};
// void ps5Cmd2();

/********************************************************************************/
/*                         L O C A L    V A R I A B L E S */
/********************************************************************************/

static ps5_connection_callback_t ps5_connection_cb = NULL;

static ps5_event_callback_t ps5_event_cb = NULL;

static bool is_active = false;

/********************************************************************************/
/*                      P U B L I C    F U N C T I O N S */
/********************************************************************************/

/*******************************************************************************
**
** Function         ps5Init
**
** Description      This initializes the bluetooth services to listen
**                  for an incoming ps5 controller connection.
**
**
** Returns          void
**
*******************************************************************************/
void ps5Init()
{
  sppInit();
  ps5_l2cap_init_services();
}

/*******************************************************************************
**
** Function         ps5IsConnected
**
** Description      This returns whether a ps5 controller is connected, based
**                  on whether a successful handshake has taken place.
**
**
** Returns          bool
**
*******************************************************************************/
bool ps5IsConnected() { return is_active; }

uint32_t crc32(uint32_t crc, const uint8_t *data, size_t len)
{
  uint32_t mult;
  int i;

  while (len--)
  {
    crc ^= *data++;
    for (i = 0; i < 8; i++)
    {
      mult = (crc & 1) ? 0xedb88320 : 0;
      crc = (crc >> 1) ^ mult;
    }
  }

  return crc;
}

/*
typedef enum
{
  HID_MESSAGE_TYPE_HANDSHAKE = 0,
  HID_MESSAGE_TYPE_HID_CONTROL,
  HID_MESSAGE_TYPE_RESERVED_2,
  HID_MESSAGE_TYPE_RESERVED_3,
  HID_MESSAGE_TYPE_GET_REPORT,
  HID_MESSAGE_TYPE_SET_REPORT,
  HID_MESSAGE_TYPE_GET_PROTOCOL,
  HID_MESSAGE_TYPE_SET_PROTOCOL,
  HID_MESSAGE_TYPE_GET_IDLE_DEPRECATED,
  HID_MESSAGE_TYPE_SET_IDLE_DEPRECATED,
  HID_MESSAGE_TYPE_DATA,
  HID_MESSAGE_TYPE_DATC_DEPRECATED
} hid_message_type_t;
typedef enum
{
  HID_REPORT_TYPE_RESERVED = 0,
  HID_REPORT_TYPE_INPUT,
  HID_REPORT_TYPE_OUTPUT,
  HID_REPORT_TYPE_FEATURE
} hid_report_type_t;
*/

/*
void ps5Cmd(ps5_cmd_t cmd)
{
  hid_cmd_t hidCommand = {.data = {0x80, 0x00, 0xFF}};
  uint16_t length = sizeof(hidCommand.data);

  hidCommand.code = hid_cmd_code_set_report | hid_cmd_code_type_output;
  hidCommand.identifier = hid_cmd_identifier_ps5_control;

  hidCommand.data[ps5_control_packet_index_small_rumble] = cmd.smallRumble; // Small Rumble
  hidCommand.data[ps5_control_packet_index_large_rumble] = cmd.largeRumble; // Big rumble

  hidCommand.data[ps5_control_packet_index_red] = cmd.r;   // Red
  hidCommand.data[ps5_control_packet_index_green] = cmd.g; // Green
  hidCommand.data[ps5_control_packet_index_blue] = cmd.b;  // Blue

  // Time to flash bright (255 = 2.5 seconds)
  hidCommand.data[ps5_control_packet_index_flash_on_time] = cmd.flashOn;
  // Time to flash dark (255 = 2.5 seconds)
  hidCommand.data[ps5_control_packet_index_flash_off_time] = cmd.flashOff;

  ps5_l2cap_send_hid(&hidCommand, length);
}
*/

static uint8_t output_seq = 0;

void ps5_send_output_report(ps5_output_report_t *out)
{
  out->transaction_type = 0xa2;
  out->report_id = 0x31; // taken from HID descriptor
  out->tag = 0x10;       // Magic number must be set to 0x10

  // Highest 4-bit is a sequence number, which needs to be increased every
  // report. Lowest 4-bit is tag and can be zero for now.
  out->seq_tag = (output_seq << 4) | 0x0;
  output_seq = ((output_seq + 1) & 0xF);

  uint32_t crc = crc32(0xffffffff, &out->transaction_type, 1);
  crc = ~crc32(crc, (uint8_t *)&out->report_id, sizeof(*out) - 5);
  out->crc32 = crc;

  ps5_l2cap_send(out, sizeof(ps5_output_report_t)); // sending HID package
}

/*******************************************************************************
**
** Function         ps5Enable
**
** Description      This triggers the ps5 controller to start continually
**                  sending its data.
**
**
** Returns          void
**
*******************************************************************************/
void ps5Enable()
{

  ps5_output_report_t out = {0};
  out.valid_flag2 = DS5_FLAG2_LIGHTBAR_SETUP_CONTROL_ENABLE;
  out.lightbar_setup = DS5_LIGHTBAR_SETUP_LIGHT_OUT;
  out.lightbar_red = 255,
  out.lightbar_green = 255,
  out.lightbar_blue = 255,
  out.valid_flag1 = DS5_FLAG1_LIGHTBAR,

  ps5_send_output_report(&out);
}

/*******************************************************************************
**
** Function         ps5SetConnectionCallback
**
** Description      Registers a callback for receiving ps5 controller
**                  connection notifications
**
**
** Returns          void
**
*******************************************************************************/
void ps5SetConnectionCallback(ps5_connection_callback_t cb)
{
  ps5_connection_cb = cb;
}

/*******************************************************************************
**
** Function         ps5SetEventCallback
**
** Description      Registers a callback for receiving ps5 controller events
**
**
** Returns          void
**
*******************************************************************************/
void ps5SetEventCallback(ps5_event_callback_t cb) { ps5_event_cb = cb; }

/*******************************************************************************
**
** Function         ps5SetBluetoothMacAddress
**
** Description      Writes a Registers a callback for receiving ps5 controller
*events
**
**
** Returns          void
**
*******************************************************************************/
void ps5SetBluetoothMacAddress(const uint8_t *mac)
{
  // The bluetooth MAC address is derived from the base MAC address
  // https://docs.espressif.com/projects/esp-idf/en/stable/api-reference/system/system.html#mac-address
  uint8_t baseMac[6];
  memcpy(baseMac, mac, 6);
  baseMac[5] -= 2;
  esp_base_mac_addr_set(baseMac);
}

/********************************************************************************/
/*                      L O C A L    F U N C T I O N S */
/********************************************************************************/

void ps5ConnectEvent(uint8_t is_connected)
{
  ESP_LOGI(TAG, "======= Connected event");
  if (is_connected)
  {
    ps5Enable();
  }
  else
  {
    is_active = false;
  }
}

void ps5PacketEvent(ps5_t ps5, ps5_event_t event)
{
  //  Trigger packet event, but if this is the very first packet
  //  after connecting, trigger a connection event instead
  if (is_active)
  {
    if (ps5_event_cb != NULL)
    {
      ps5_event_cb(ps5, event);
    }
  }
  else
  {
    is_active = true;

    if (ps5_connection_cb != NULL)
    {
      ps5_connection_cb(is_active);
    }
  }
}
