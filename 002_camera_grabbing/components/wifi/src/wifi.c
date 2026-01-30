#include "wifi.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types_generic.h"
#include "esp_netif.h"
#include "esp_log.h"


static const char* WTAG = "wifi";

void start_wifi_sta(void) {
  // 1. Create default STA network interface
  esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
  if (!sta_netif) {
    ESP_LOGE(WTAG, "Failed to create STA network interface");
    return;
  }

  // Initialize wifi with default configuration
  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

  // Set wifi mode to station (client)
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  // Configure STA settings - REPLACE WITH YOUR NETWORK CREDENTIALS
  wifi_config_t wifi_config = {
    .sta = {
      .ssid = "router_jomasef",      // Your network name
      .password = "76GJUYGTBMIK",
      .threshold.authmode = WIFI_AUTH_WPA2_PSK, // Most common
    }
  };

  // Apply this configuration
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

  // Start the wifi station
  ESP_ERROR_CHECK(esp_wifi_start());

  // Connect to the AP
  ESP_ERROR_CHECK(esp_wifi_connect());

  // Wait for connection (optional but useful)
  ESP_LOGI(WTAG, "Connecting to WiFi: %s", wifi_config.sta.ssid);

  wifi_ap_record_t ap_info;
  int retries = 0;
  while (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK && retries < 20) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    retries++;
    ESP_LOGI(WTAG, "Waiting for connection... %d", retries);
  }

  if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
    // Get and log the IP address
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(sta_netif, &ip_info);

    ESP_LOGI(WTAG, "WiFi STA connected!");
    ESP_LOGI(WTAG, "IP Address: " IPSTR, IP2STR(&ip_info.ip));
    ESP_LOGI(WTAG, "Navigate to: http://" IPSTR, IP2STR(&ip_info.ip));
  } else {
    ESP_LOGW(WTAG, "Failed to connect to WiFi");
  }
}


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
      .max_connection = 10,        // 1 connection allowed
      .beacon_interval = 100,     // how often AP announces itself
    }
  };

  // apply this configuration
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

  // start the wifi access point
  ESP_ERROR_CHECK(esp_wifi_start());

  // logs
  ESP_LOGI(WTAG, "WiFi AP started: SSID='%s'", wifi_config.ap.ssid);
  ESP_LOGI(WTAG, "Connect to WiFi: %s with password: %s",
      wifi_config.ap.ssid, wifi_config.ap.password);
      ESP_LOGI(WTAG, "Then navigate to: http://192.168.4.1");
}