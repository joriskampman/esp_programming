#include "webserver.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "freertos/projdefs.h"
#include "stream_handler.h"
#include "api_handler.h"

// #include "freertos/projdefs.h"

static const char* SERVTAG = "webserver";
static httpd_handle_t server = NULL;

void stop_webserver(httpd_handle_t server) {
  if (server == NULL) {
    return;  // already stopped
  }

  // 1. websockets (if existing)

  // 2. unregister URI handlers
  api_unregister_handlers(server);
  stream_unregister_handlers(server);

  // 3. stop httpd server core
  esp_err_t stop_success;
  int max_retries = 10;
  int itry = 0;
  ESP_LOGI(SERVTAG, "Maximum %d attempts to stop the webserver", max_retries);
  for (int itry=0; itry < max_retries; itry++) {
    stop_success = httpd_stop(server);
    if (stop_success == ESP_OK) {
      ESP_LOGI(SERVTAG, "The webserver stopped at attempt (%d)", itry);
      server = NULL;
      return;
    }
    ESP_LOGW(SERVTAG, "Stopping failed. Retrying ..");
    vTaskDelay(pdMS_TO_TICKS(100));

  } // for tries to stop

  ESP_LOGE(SERVTAG, "The server could not be stopped!");
}


void start_webserver(void) {
  /*
  start the webserver
  */

  if (server) {
    ESP_LOGW(SERVTAG, "Webserver already started");
    return;
  }

  ESP_LOGI(SERVTAG, "Starting HTTP web server");


  // configure the HTTP server
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;  // standard http port
  config.max_uri_handlers = 5;  // root, api/status and /stream
  config.max_open_sockets = 1;  // single client only
  config.lru_purge_enable = false;  //
  config.recv_wait_timeout = 5;  // seconds
  config.send_wait_timeout = 5;  // seconds
  config.stack_size = 8192;  // default 4096, increased for image processing

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

  // ------------- register URI handlers --------------------------------
  ESP_LOGI(SERVTAG, "Registering stream handlers ...");
  stream_register_handlers(server);
  ESP_LOGI(SERVTAG, "Registering api handlers ...");
  api_register_handlers(server);

  if (success != ESP_OK) {
    ESP_LOGE(SERVTAG, "The webserver FAILED to start after (%d) attempts! ", max_retries);
    return;
  }

  ESP_LOGI(SERVTAG, "HTTP server started on port %d", config.server_port);
  ESP_LOGI(SERVTAG, "Connect to http://192.168.4.1 in your browser");
}

httpd_handle_t get_webserver_handle(void) {
  return server;
}

