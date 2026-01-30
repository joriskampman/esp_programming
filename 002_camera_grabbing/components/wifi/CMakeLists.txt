idf_component_register(
  SRCS
    "src/wifi.c"

  INCLUDE_DIRS
    "include"

  REQUIRES
    esp_wifi              # wifi support
    # common
    # esp_http_server   # http server from esp-idf
    # esp_netif         # network interface
    # nvs_flash         # for storing settings
    # display
    # esp32-camera

  # # private requirements
  # PRIV_REQUIRES
)
