/*
buffer_pool.h

contains the buffer pool for the use of a single producer (camera) multiple consumer (display and
vision processing task). Gives more flexibility than the current use of queue which can only do a
single producer, single consumer arrangement

author: Joris Kampman, Saxion SMART, winter 25/26
*/
#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#pragma once  // replaces include guards
#include "esp_camera.h"
#include "freertos/FreeRTOS.h"  // IWYU pragma: keep
#include "freertos/semphr.h"

// constants
#define POOL_SIZE 3   // preprocesser. otherwise typedef does not work

// types and structures
typedef struct {
  camera_fb_t* buffers[POOL_SIZE];  // array of frame buffers
  SemaphoreHandle_t free_sem;       // empty buffers count
  SemaphoreHandle_t used_sem;       // filled buffers count
  SemaphoreHandle_t mutex;          // for display peak access
  int write_idx;                    // index of buffer to write to
  int read_idx;                     // index of buffer to read from
} buffer_pool_t;

buffer_pool_t* get_fb_pool(void);
void buffer_pool_init(void);
void buffer_pool_deinit(void);
void buffer_pool_signal_filled(void);
void buffer_pool_signal_processed(void);
camera_fb_t* buffer_pool_get_empty(void);
camera_fb_t* buffer_pool_get_filled(void);
camera_fb_t* buffer_pool_get_peek(void);

#endif