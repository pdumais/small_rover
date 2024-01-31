#include <string.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_event.h>
#include <freertos/event_groups.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "nvs_flash.h"
#include "nvs.h"
#include <netdb.h>
#include "gpio.h"
#include "wifi.h"
#include "led.h"
#include "common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

static const char *TAG = "wifi_c";
static esp_netif_t *sta_netif;
static esp_netif_t *ap_netif;
static volatile int wifi_state;

/* Signal Wi-Fi events on this event-group */
const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifi_event_group;
static int sock;

int get_wifi_state()
{
    return wifi_state;
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        // ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "REDIRECT");
        break;
    }
    return ESP_OK;
}

void start_ota_upgrade()
{
    ESP_LOGI(TAG, "Starting OTA Upgrade");
    gpio_suspend();

    led_pattern p = LED_COLORFUL;

    led_set_rotating_pattern(&p, 50, 6);

    esp_http_client_config_t config = {0};
    config.url = "http://192.168.4.2:8114/rover.bin";
    config.event_handler = _http_event_handler;

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK)
    {
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "Firmware upgrade failed");
    }
}

void upgrade_udp_command_server(void *params)
{
    struct sockaddr_in6 dest_addr;
    char rx_buffer[128];
    char addr_str[128];

    struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
    dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_ip4->sin_family = AF_INET;
    dest_addr_ip4->sin_port = htons(242);

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    while (1)
    {
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, NULL, NULL);
        if (len == 4)
            start_ota_upgrade();
    }
}

bool wifi_init()
{
    wifi_state = WIFI_STATE_NONE;

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = 0;

    wifi_config_t ap_cfg = {
        .ap = {
            .ssid = "ROVER",
            .ssid_len = 5,
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_state = WIFI_STATE_AP;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
    ESP_ERROR_CHECK(esp_wifi_start());

    xTaskCreate(upgrade_udp_command_server, "udp_server", 4096, NULL, 5, NULL);

    return true;
}

void broadcast_metric(metrics_t *metrics)
{
    if (!sock)
    {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
    }

    int n = 0;

    char fixed_num[10] = {0};
    n = sprintf(fixed_num, "%i", metrics->speed);
    if (n > 1)
    {
        fixed_num[n] = fixed_num[n - 1];
        fixed_num[n - 1] = '.';
    }

    char str[256];
    n = 0;
    n += sprintf(str, "throttle: %i, ", metrics->throttle);
    n += sprintf(str + n, "pwm: %i, ", metrics->pwm);
    n += sprintf(str + n, "direction: %i, ", metrics->direction);
    n += sprintf(str + n, "steering_angle: %i, ", metrics->steering_angle);
    n += sprintf(str + n, "rotator_angle: %hhd ", metrics->angle_rotator);
    n += sprintf(str + n, "boom_angle: %hhd, ", metrics->angle_boom);
    n += sprintf(str + n, "arm_angle: %hhd, ", metrics->angle_arm);
    n += sprintf(str + n, "grapple_angle: %hhd, ", metrics->angle_grapple);
    n += sprintf(str + n, "horn: %i, ", metrics->horn);
    n += sprintf(str + n, "rpm1: %i, ", metrics->rpm1);
    n += sprintf(str + n, "rpm2: %i, ", metrics->rpm2);
    n += sprintf(str + n, "rpm3: %i, ", metrics->rpm3);
    n += sprintf(str + n, "rpm4: %i ", metrics->rpm4);
    n += sprintf(str + n, "battery: %i, ", metrics->controller_battery);
    n += sprintf(str + n, "speed: %s\n", fixed_num);
    ESP_LOGI(TAG, "%s", str);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(242);
    addr.sin_addr.s_addr = inet_addr("192.168.4.2");
    int err = sendto(sock, str, n + 1, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (err < 0)
    {
    }
}

void broadcast_log(const char *str)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(242);
    addr.sin_addr.s_addr = inet_addr("192.168.4.2");
    int err = sendto(sock, str, strlen(str), 0, (struct sockaddr *)&addr, sizeof(addr));
}