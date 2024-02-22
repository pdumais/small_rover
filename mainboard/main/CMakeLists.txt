idf_component_register(SRCS "app_main.c" "hardware.c" "wifi.c" "led.c" "buzzer.c" "arm.c" "servo.c" "common/deepsleep.c" "routerecorder.c" "obstructions.c" "uart.c" "hcsr04.c" "laser.c" "http_server.c" "api.c" "taskpool.c" "sensors.c"
                    INCLUDE_DIRS "."
                    EMBED_FILES "index.html" 
                    PRIV_REQUIRES driver nvs_flash esp_event esp_wifi app_update esp_http_client json  esp_https_ota esp_http_server              )

