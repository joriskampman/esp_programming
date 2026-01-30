#ifndef STREAM_HANDLER_H
#define STREAM_HANDLER_H
#pragma once

#include "esp_http_server.h"
#include "esp_camera.h"

void stream_register_handlers(httpd_handle_t);
void stream_unregister_handlers(httpd_handle_t);

camera_fb_t* stream_get_latest_frame(void);

static esp_err_t root_handler(httpd_req_t*);
static esp_err_t stream_mjpeg_handler(httpd_req_t*);

#endif