#ifndef CAMERA_H
#define CAMERA_H

#include "esp_err.h"

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

// declare functions
esp_err_t init_camera(void);

void camera_task(void* pvParameters);

#endif