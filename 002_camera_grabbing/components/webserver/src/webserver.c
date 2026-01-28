#include "webserver.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "freertos/projdefs.h"

static const char* SERVTAG = "webserver";
static httpd_handle_t server = NULL;

void stop_webserver(httpd_handle_t server) {
  if (server == NULL) {
    return;  // already stopped
  }

  // 1. websockets (if existing)

  // 2. unregister URI handlers

  // 3. stop httpd server core
  int max_retries = 10;
  int itry = 0;
  ESP_LOGI(SERVTAG, "Maximum %d attempts to stop the webserver", max_retries);
  esp_err_t stop_success = ESP_FAIL;
  while (stop_success != ESP_OK) {
    stop_success = httpd_stop(server);
    if (stop_success == ESP_OK) {
      ESP_LOGI(SERVTAG, "The webserver stopped at attempt (%d)", itry);
    }

    ESP_LOGW(SERVTAG, "Stopping failed. Retrying ..");
  } // while (1)
  if (stop_success != ESP_OK) {
    ESP_LOGE(SERVTAG, "The server could not be stopped!");
    return;
  }

  //4. free memory allocated (if any)

  // 5. set handle to NULL
  server = NULL;
}


httpd_handle_t start_webserver(void) {
  /*
  start the webserver
  */

  // get the default config
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  config.max_open_sockets = 2;
  config.lru_purge_enable = false;  //
  config.recv_wait_timeout = 5;  // seconds
  config.send_wait_timeout = 5;  // seconds
  config.stack_size = 6144;  // default 4096, the minimum stack size allowed for the server task

  esp_err_t success = ESP_FAIL;
  int max_retries = 5;
  int itry = 0;
  ESP_LOGI(SERVTAG, "MAXIMUM %d attempts to start the webserver", max_retries);
  while (success != ESP_OK) {
    success = httpd_start(&server, &config);
    if (success == ESP_OK) {
      // break out
      ESP_LOGI(SERVTAG, "The webserver has started at attempt (%d)!", itry);
      break;
    }
    ESP_LOGE(SERVTAG, "Starting the webserver has failed. Retrying ..");
    itry++;
  } // loop maximum retries

  if (success != ESP_OK) {
    ESP_LOGE(SERVTAG, "The webserver FAILED to start after (%d) attempts! ", max_retries);
    return NULL;
  }

  // connection established
  return server;
}

