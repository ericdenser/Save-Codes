idf_component_register(SRCS "main.cpp" "src/WifiManager.cpp" "src/HttpService.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES esp_adc nvs_flash esp_wifi esp_event esp_http_client esp_timer mbedtls)
