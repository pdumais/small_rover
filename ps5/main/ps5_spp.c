#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"
#include "esp_spp_api.h"
#include "ps5.h"
#include "ps5_int.h"

#define ps5_TAG "ps5_SPP"

/********************************************************************************/
/*              L O C A L    F U N C T I O N     P R O T O T Y P E S            */
/********************************************************************************/
static void sppCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

/********************************************************************************/
/*                      P U B L I C    F U N C T I O N S                        */
/********************************************************************************/

/*******************************************************************************
**
** Function         sppInit
**
** Description      Initialise the SPP server to allow to be connected to
**
** Returns          void
**
*******************************************************************************/
void sppInit()
{
  esp_err_t ret;
  
  if ((ret = esp_spp_register_callback(sppCallback)) != ESP_OK)
  {
    ESP_LOGE(ps5_TAG, "%s spp register failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }

  if ((ret = esp_spp_init(ESP_SPP_MODE_CB)) != ESP_OK)
  {
    ESP_LOGE(ps5_TAG, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }
}

/********************************************************************************/
/*                      L O C A L    F U N C T I O N S                          */
/********************************************************************************/

/*******************************************************************************
**
** Function         sppCallback
**
** Description      Callback for SPP events, only used for the init event to
**                  configure the SPP server
**
** Returns          void
**
*******************************************************************************/

static void sppCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  if (event == ESP_SPP_INIT_EVT)
  {
    ESP_LOGI(ps5_TAG, "ESP_SPP_INIT_EVT");

    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE /*ESP_BT_NON_DISCOVERABLE*/);
    // esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE);
  }
}
