#ifndef CAMERA_H
#define CAMERA_H

#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "common.h"

// DEFINE PINOUT FOR CAMERA
// from: ../pics/pinout.avif
#define CAM_SIOD  4
#define CAM_SIOC  5
#define CAM_VSYNC 6
#define CAM_HREF  7
#define CAM_XCLK  15
#define CAM_Y9    16
#define CAM_Y8    17
#define CAM_Y7    18
#define CAM_Y4    8
#define CAM_Y3    9
#define CAM_Y5    10
#define CAM_Y2    11
#define CAM_Y6    12
#define CAM_PCLK  13

#define FRAMESIZE FRAMESIZE_QVGA
#define PIXFMT PIXFORMAT_GRAYSCALE

// typedefs
typedef struct {
  QueueHandle_t frame_event_queue;
} camera_tasks_params_t;

// declare functions
static const float DATA_RATE_HZ = 10.0f*M2b_f;
float get_xclk_freq_hz();

static const uint32_t FB_COUNT = 2;
float get_fb_count();

esp_err_t init_camera(void);

void camera_task(void* pvParameters);

#endif