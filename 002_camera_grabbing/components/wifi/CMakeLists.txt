idf_component_register(
  SRCS
    "src/wifi.c"
    "src/wifi_config.c"

  INCLUDE_DIRS
    "include"
    "credentials"

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

# Pass WiFi credentials from CMake to compiler defines
if(DEFINED WIFI_STA_SSID)
    message(STATUS "WiFi: Using WIFI_STA_SSID: ${WIFI_STA_SSID}")
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        WIFI_STA_SSID="${WIFI_STA_SSID}")
endif()

if(DEFINED WIFI_STA_PASS)
    message(STATUS "WiFi: Using WIFI_STA_PASS: [configured]")
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        WIFI_STA_PASS="${WIFI_STA_PASS}")
endif()

if(DEFINED WIFI_AP_SSID)
    message(STATUS "WiFi: Using WIFI_AP_SSID: ${WIFI_AP_SSID}")
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        WIFI_AP_SSID="${WIFI_AP_SSID}")
endif()

if(DEFINED WIFI_AP_PASS)
    message(STATUS "WiFi: Using WIFI_AP_PASS: [configured]")
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        WIFI_AP_PASS="${WIFI_AP_PASS}")
endif()
