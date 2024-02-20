#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>

#include <esp_log.h>
#include "esp_chip_info.h"
#include <esp_wifi.h>
#include "esp_flash.h"
#include <esp_system.h>
#include "cJSON.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_websocket_client.h"
#include "http_server.h"
#include "common.h"
#include "api.h"

// idf.py add-dependency "espressif/esp_websocket_client^1.0.0"

static const char *TAG = "api_c";
static esp_websocket_client_handle_t client;

static esp_err_t html_handler(httpd_req_t *req)
{
    extern const unsigned char html_start[] asm("_binary_index_html_start");
    extern const unsigned char html_end[] asm("_binary_index_html_end");
    const size_t html_size = (html_end - html_start);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)html_start, html_size);
    return ESP_OK;
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED %i", data->error_handle.esp_ws_handshake_status_code);
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        ESP_LOGI(TAG, "Received opcode=%d", data->op_code);

        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    }
}


void start_api()
{
    start_webserver();
    init_ws();
    register_uri("/", HTTP_GET, html_handler);
}