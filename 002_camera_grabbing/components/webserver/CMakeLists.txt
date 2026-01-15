idf_component_register(
  SRCS
    "src/webserver.c"
    "src/stream_handler.c"
    "src/api_handler.c"

  INCLUDE_DIRS
    "include"

  REQUIRES
    common
    esp_http_server   # http server from esp-idf
    esp_netif         # network interface
    esp_wifi              # wifi support
    nvs_flash         # for storing settings

  # # private requirements
  # PRIV_REQUIRES
)
