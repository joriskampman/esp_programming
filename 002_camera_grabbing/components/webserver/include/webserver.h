#ifndef WEBSERVER_H
#define WEBSERVER_H
#pragma once

#include "esp_http_server.h"

void start_webserver(void);

httpd_handle_t get_webserver_handle(void);

void stop_webserver(httpd_handle_t server);

#endif