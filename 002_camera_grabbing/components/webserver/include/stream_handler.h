#ifndef STREAM_HANDLER_H
#define STREAM_HANDLER_H
#pragma once

#include "esp_http_server.h"

void stream_mjpeg_handler_task(httpd_req_t*);

#endif