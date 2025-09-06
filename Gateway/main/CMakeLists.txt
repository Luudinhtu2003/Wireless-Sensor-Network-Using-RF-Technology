
idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES lora driver log nvs_flash http_server_app esp_wifi)


