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
#include <stdbool.h>

// constants
#define POOL_SIZE 3   // preprocesser. otherwise typedef does not work

// types and structures
typedef struct {
  camera_fb_t* buffers[POOL_SIZE+1];  // array of frame buffers. the 1 is the display pool
  SemaphoreHandle_t display_mutex;  // display
  volatile bool buffer_in_use[POOL_SIZE];  // array to check if buffer is in use
  volatile int write_idx;           // index of buffer to WRITE To
  volatile int proc_idx;            // index of buffer to PROCESS
  volatile int disp_idx;            // index of buffer to DISPLAY
} buffer_pool_t;

void inc_idx(volatile int*);
void upd_idx(volatile int*, int);
void print_buffer_use(void);
void upd_buffer_in_use(int, bool);
buffer_pool_t* get_buffer_pool(void);
void buffer_pool_init(void);
void buffer_pool_deinit(void);
void buffer_pool_signal_filled(void);
void buffer_pool_signal_processed(void);
camera_fb_t* buffer_pool_get_write_buffer(void);
void buffer_pool_signal_filled(void);
camera_fb_t* buffer_pool_proc_get_filled(void);
void buffer_pool_signal_processed(void);

#endif