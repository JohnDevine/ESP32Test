idf_component_register(SRCS "ESP32Test.c"
                    EMBED_FILES "index.html"
                    PRIV_REQUIRES esp_wifi nvs_flash esp_http_server esp_netif esp_event lwip esp_timer
                    INCLUDE_DIRS ".")
