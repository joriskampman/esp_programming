#include "wifi.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types_generic.h"
#include "esp_netif.h"

static const char* WTAG = "wifi";

void start_wifi_ap(void) {
  // 1. create default network interface
  esp_netif_t* ap_netif = esp_netif_create_default_wifi_ap();
  if (!ap_netif) {
    ESP_LOGE(WTAG, "Failed to create AP network interface");
    return;
  }

  // initialize wifi with default configuration
  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

  // set wifi mode to access point
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

  // configure AP settings
  wifi_config_t wifi_config = {
    .ap = {
      .ssid = "CablePeeler",      // network name
      .password = "password",
      .ssid_len = strlen("CablePeeler"),
      .channel = 6,                    // pick a channel ---> check best
      .authmode = WIFI_AUTH_WPA2_PSK, // security protocol
      .max_connection = 1,        // 1 connection allowed
      .beacon_interval = 100,     // how often AP announces itself
    }
  };

  // apply this configuration
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

  // start the wifi access point
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(WTAG, "Wifi access point started: SSID=%s, IP=", wifi_config.ap.ssid);
}