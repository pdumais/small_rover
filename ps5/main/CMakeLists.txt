idf_component_register(SRCS "app_main.c" "ps5_l2cap.c" "ps5_parser.c" "ps5_spp.c" "ps5.c" "ps5_controller.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_hw_support bt nvs_flash
                    PRIV_REQUIRES driver
                    )

