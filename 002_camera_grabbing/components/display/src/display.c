/*
display component that copies the framebuffer from the buffer pool and shows it in the web browser

author: Joris Kampman, Saxion SMART, winter 25/26
*/
#include <string.h>
#include "display.h"
#include "esp_log.h"
#include "common.h"
#include "freertos/idf_additions.h"
#include "buffer_pool.h"

static const char* DISPTAG = "display frame";

static camera_fb_t display_fb = {0};  // full frame buffer

void update_display_fb(buffer_pool_t* pool) {
  // create static camera_fb_t variable
  // get the source frame
  camera_fb_t* src = buffer_pool_disp_get_filled();

  // allocate frame buffer if necessary (is it??)
  if (!display_fb.buf || display_fb.len != src->len) {
    free(display_fb.buf);
    display_fb.buf = malloc(src->len);
    display_fb.len = src->len;
  }

  // copy all metadata
  display_fb.width = src->width;
  display_fb.height = src->height;
  display_fb.format = src->format;
  display_fb.timestamp = src->timestamp;

  // copy pixel data
  memcpy(display_fb.buf, src->buf, src->len);

  buffer_pool_signal_dislayed();
} // update_display_frame

camera_fb_t* get_display_fb(void) {
  return &display_fb;
} // get_display_fb


void frame_display_task(void* pvParameters) {
  ESP_LOGD(DISPTAG, "displaying a frame (to be implemented)\n");

  // prevent returning
  TickType_t xLastWakeTime = xTaskGetTickCount();

  const TickType_t DISP_INTV_TICKS = pdMS_TO_TICKS(b2m_f/get_display_rate_hz_f());

  while (1) {
    ESP_LOGV(DISPTAG, ".... frame displayed ...");
    vTaskDelayUntil(&xLastWakeTime, DISP_INTV_TICKS);
  }
} // frame_display_task