
idf_component_register(SRCS "http_server_app.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver esp_wifi nvs_flash esp_eth protocol_examples_common esp-tls esp_http_server)
