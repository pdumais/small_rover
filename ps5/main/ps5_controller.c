#include "ps5_controller.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <esp_bt_defs.h>
#include <esp_bt_main.h>
#include <esp_bt.h>
#include <esp_timer.h>
#include <string.h>

#include "ps5.h"
#include "ps5_int.h"

static const char *TAG = "ps5_controller";
static ps5_t _data;
static ps5_event_t _event;

#define ESP_BD_ADDR_HEX_PTR(addr)                                \
  (uint8_t *)addr + 0, (uint8_t *)addr + 1, (uint8_t *)addr + 2, \
      (uint8_t *)addr + 3, (uint8_t *)addr + 4, (uint8_t *)addr + 5
static void _event_callback(ps5_t data, ps5_event_t event);
static void _connection_callback(uint8_t isConnected);

callback_t _callback_event = 0;
callback_t _callback_connect = 0;
callback_t _callback_disconnect = 0;

ps5_output_report_t output_report = {0};

ps5_t *ps5_get_data()
{
  return &_data;
}

ps5_event_t *ps5_get_event()
{
  return &_event;
}

bool btStarted()
{
  return (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED);
}

bool btStartMode(esp_bt_mode_t esp_bt_mode)
{
  esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  // esp_bt_controller_enable(MODE) This mode must be equal as the mode in “cfg” of esp_bt_controller_init().
  cfg.mode = esp_bt_mode;

  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED)
  {
    return true;
  }
  esp_err_t ret;
  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE)
  {
    if ((ret = esp_bt_controller_init(&cfg)) != ESP_OK)
    {
      ESP_LOGE(TAG, "initialize controller failed: %s", esp_err_to_name(ret));
      return false;
    }
    while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE)
    {
    }
  }
  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED)
  {
    if ((ret = esp_bt_controller_enable(esp_bt_mode)) != ESP_OK)
    {
      ESP_LOGE(TAG, "BT Enable mode=%d failed %s", BT_MODE, esp_err_to_name(ret));
      return false;
    }
  }
  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED)
  {
    return true;
  }
  ESP_LOGE(TAG, "BT Start failed");
  return false;
}

bool btStart()
{
  return btStartMode(BT_MODE);
}

bool ps5_begin(const char *mac)
{
  esp_bd_addr_t addr;

  if (sscanf(mac, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", ESP_BD_ADDR_HEX_PTR(addr)) != ESP_BD_ADDR_LEN)
  {
    ESP_LOGE(TAG, "Can't parse MAC");
    return false;
  }

  ps5_l2cap_connect(addr);

  ps5SetEventCallback(&_event_callback);
  ps5SetConnectionCallback(&_connection_callback);

  if (!btStarted() && !btStart())
  {
    return false;
  }

  esp_bluedroid_status_t btState = esp_bluedroid_get_status();
  if (btState == ESP_BLUEDROID_STATUS_UNINITIALIZED)
  {
    if (esp_bluedroid_init())
    {
      ESP_LOGE(TAG, "Can't init bluedroid (1)");
      return false;
    }
  }

  if (btState != ESP_BLUEDROID_STATUS_ENABLED)
  {
    if (esp_bluedroid_enable())
    {
      ESP_LOGE(TAG, "Can't init bluedroid (2)");
      return false;
    }
  }

  ps5Init();
  return true;
}

unsigned long millis()
{
  return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

bool ps5_isConnected()
{
  return ps5IsConnected();
}

void ps5_try_pairing()
{
  while (!ps5IsConnected())
  {
    ps5_l2cap_reconnect();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
void ps5_disable_lightbar()
{
  output_report.valid_flag2 = DS5_FLAG2_LIGHTBAR_SETUP_CONTROL_ENABLE;
  output_report.lightbar_setup = DS5_LIGHTBAR_SETUP_LIGHT_OUT;

  ps5_send_output_report(&output_report);
}

void ps5_set_led(uint8_t r, uint8_t g, uint8_t b)
{
  ps5_output_report_t out = {0};
  output_report.lightbar_red = r;
  output_report.lightbar_green = g;
  output_report.lightbar_blue = b;
  output_report.valid_flag1 = DS5_FLAG1_LIGHTBAR;

  ps5_send_output_report(&output_report);
}

void ps5_set_rumble(uint8_t small, uint8_t large)
{

  output_report.motor_right = small;
  output_report.motor_left = large;
  output_report.valid_flag0 = DS5_FLAG0_HAPTICS_SELECT | DS5_FLAG0_COMPATIBLE_VIBRATION;
  ps5_send_output_report(&output_report);
}

void ps5_trigger_effect(uint8_t effect, uint8_t left, uint8_t right)
{

  if (effect == 0)
  {
    if (right)
    {
      output_report.right_trigger_motor_mode = 0x05;
    }
    if (left)
    {
      output_report.left_trigger_motor_mode = 0x05;
    }
  }
  else if (effect == 1)
  {
    if (right)
    {
      output_report.right_trigger_motor_mode = 0x21;
      output_report.right_trigger_param[0] = 0xFF;
      output_report.right_trigger_param[1] = 0x03;
      output_report.right_trigger_param[2] = 0xFF;
      output_report.right_trigger_param[3] = 0xFF;
      output_report.right_trigger_param[4] = 0xFF;
      output_report.right_trigger_param[5] = 0x3F;
    }
    if (left)
    {
      output_report.left_trigger_motor_mode = 0x21;
      output_report.left_trigger_param[0] = 0xFF;
      output_report.left_trigger_param[1] = 0x03;
      output_report.left_trigger_param[2] = 0xFF;
      output_report.left_trigger_param[3] = 0xFF;
      output_report.left_trigger_param[4] = 0xFF;
      output_report.left_trigger_param[5] = 0x3F;
    }
  }
  else if (effect == 2)
  {
    if (right)
    {
      output_report.right_trigger_motor_mode = 0x26;
      output_report.right_trigger_param[0] = 0xe0;
      output_report.right_trigger_param[1] = 0x03;
      output_report.right_trigger_param[2] = 0x00;
      output_report.right_trigger_param[3] = 0x80;
      output_report.right_trigger_param[4] = 0xFF;
      output_report.right_trigger_param[5] = 0x3F;
      output_report.right_trigger_param[6] = 0x00;
      output_report.right_trigger_param[7] = 0x00;
      output_report.right_trigger_param[8] = 0x1e;
    }
    if (left)
    {
      output_report.left_trigger_motor_mode = 0x26;
      output_report.left_trigger_param[0] = 0xe0;
      output_report.left_trigger_param[1] = 0x03;
      output_report.left_trigger_param[2] = 0x00;
      output_report.left_trigger_param[3] = 0x80;
      output_report.left_trigger_param[4] = 0xFF;
      output_report.left_trigger_param[5] = 0x3F;
      output_report.left_trigger_param[6] = 0x00;
      output_report.left_trigger_param[7] = 0x00;
      output_report.left_trigger_param[8] = 0x1e;
    }
  }
  if (left)
  {
    output_report.valid_flag0 |= DS_OUTPUT_VALID_FLAG0_LEFT_TRIGGER_MOTOR_ENABLE;
  }
  else
  {
    output_report.valid_flag0 &= ~DS_OUTPUT_VALID_FLAG0_LEFT_TRIGGER_MOTOR_ENABLE;
  }

  if (right)
  {
    output_report.valid_flag0 |= DS_OUTPUT_VALID_FLAG0_RIGHT_TRIGGER_MOTOR_ENABLE;
  }
  else
  {
    output_report.valid_flag0 &= ~DS_OUTPUT_VALID_FLAG0_RIGHT_TRIGGER_MOTOR_ENABLE;
  }

  ps5_send_output_report(&output_report);
}

void ps5_setFlashRate(uint8_t onTime, uint8_t offTime)
{
}

void ps5_attach(callback_t callback) { _callback_event = callback; }

void ps5_attachOnConnect(callback_t callback)
{
  _callback_connect = callback;
}

void ps5_attachOnDisconnect(callback_t callback)
{
  _callback_disconnect = callback;
}

static void _event_callback(ps5_t data, ps5_event_t event)
{
  memcpy(&_data, &data, sizeof(ps5_t));
  memcpy(&_event, &event, sizeof(ps5_event_t));

  if (_callback_event)
  {
    _callback_event();
  }
}

static void _connection_callback(uint8_t isConnected)
{
  if (isConnected)
  {
    vTaskDelay(250 / portTICK_PERIOD_MS);

    if (_callback_connect)
    {
      _callback_connect();
    }
  }
  else
  {
    if (_callback_disconnect)
    {
      _callback_disconnect();
    }
  }
}

bool ps5_Right() { return _data.button.right; }
bool ps5_Down() { return _data.button.down; }
bool ps5_Up() { return _data.button.up; }
bool ps5_Left() { return _data.button.left; }

bool ps5_Square() { return _data.button.square; }
bool ps5_Cross() { return _data.button.cross; }
bool ps5_Circle() { return _data.button.circle; }
bool ps5_Triangle() { return _data.button.triangle; }

bool ps5_UpRight() { return _data.button.upright; }
bool ps5_DownRight() { return _data.button.downright; }
bool ps5_UpLeft() { return _data.button.upleft; }
bool ps5_DownLeft() { return _data.button.downleft; }

bool ps5_L1() { return _data.button.l1; }
bool ps5_R1() { return _data.button.r1; }
bool ps5_L2() { return _data.button.l2; }
bool ps5_R2() { return _data.button.r2; }

bool ps5_Share() { return _data.button.share; }
bool ps5_Options() { return _data.button.options; }
bool ps5_L3() { return _data.button.l3; }
bool ps5_R3() { return _data.button.r3; }

bool ps5_PSButton() { return _data.button.ps; }
bool ps5_Touchpad() { return _data.button.touchpad; }

uint8_t ps5_L2Value() { return _data.analog.button.l2; }
uint8_t ps5_R2Value() { return _data.analog.button.r2; }

int8_t ps5_LStickX() { return _data.analog.stick.lx; }
int8_t ps5_LStickY() { return _data.analog.stick.ly; }
int8_t ps5_RStickX() { return _data.analog.stick.rx; }
int8_t ps5_RStickY() { return _data.analog.stick.ry; }

uint8_t ps5_Battery() { return _data.status.battery; }
bool ps5_Charging() { return _data.status.charging; }
bool ps5_Audio() { return _data.status.audio; }
bool ps5_Mic() { return _data.status.mic; }
