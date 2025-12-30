idf_component_register(SRCS "websocket_client.c" "sensor.c" "wifi_mgr.c" "protocol.c" "net_time.c" "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_adc nvs_flash esp_wifi esp_event driver esp_netif esp_websocket_client)