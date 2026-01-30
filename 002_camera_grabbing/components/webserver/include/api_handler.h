#ifndef API_HANDLER_H
#define API_HANDLER_H

#include "esp_http_server.h"

void api_register_handlers(httpd_handle_t);
static esp_err_t api_status_handler(httpd_req_t*);
void api_unregister_handlers(httpd_handle_t);

#endif