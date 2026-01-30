#include "api_handler.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "http_parser.h"

static const char* APITAG = "api";

// ---------- api endpoint ----------------------------
static esp_err_t api_status_handler(httpd_req_t* req) {
  const char* json_response =
    "{\"status\":\"ok\","
    "\"service\":\"camera_stream\","
    "\"mode\": \"single_client\","
    "\"resolution\":\"QVGA_320X240\","
    "\"format\":\"grayscale\"}";

  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, json_response, strlen(json_response));
  return ESP_OK;
}

void api_register_handlers(httpd_handle_t server) {
  /*
  register API handlers
  */

  // status endpoint
  httpd_uri_t status_uri = {
    .uri = "/api/status",
    .method = HTTP_GET,
    .handler = api_status_handler,
    .user_ctx = NULL
  };
  httpd_register_uri_handler(server, &status_uri);

  ESP_LOGI(APITAG, "API handler '/api/status' registered");
}

void api_unregister_handlers(httpd_handle_t server) {
  httpd_unregister_uri(server, "/api/status");
}