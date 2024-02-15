idf_component_register(SRCS "app_main.c" "hardware.c" "wifi.c" "led.c" "buzzer.c" "arm.c" "servo.c" "common/deepsleep.c" "routerecorder.c" "obstructions.c" "uart.c" "hcsr04.c" "laser.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES driver nvs_flash esp_event esp_wifi app_update esp_http_client  esp_https_ota               )

