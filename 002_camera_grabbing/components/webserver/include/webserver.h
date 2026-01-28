#ifdef WEBSERVER_H
#define WEBSERVER_H
#pragma once

#include "esp_http_server.h"
#include "esp_log.h"

httpd_handle_t start_webserver(void);
void stop_webserver(httpd_handle_t server);

#endif