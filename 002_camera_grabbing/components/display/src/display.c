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
#include "freertos/projdefs.h"
#include "vision_processing.h"

static const char* DISPTAG = "display";

camera_fb_t* get_display_fb(void) {
  buffer_pool_t* pool = get_buffer_pool();
  if (!pool) {
    ESP_LOGE(DISPTAG, "The buffer pool is NULL");
    return NULL;
  }

  return pool->buffers[POOL_SIZE];
}

void frame_display_task(void* pvParameters) {
  ESP_LOGD(DISPTAG, "displaying a frame (to be implemented)\n");

  // get the correct frame buffer
  buffer_pool_t* pool = get_buffer_pool();
  camera_fb_t* fb = pool->buffers[POOL_SIZE];  // the last one is always the display frame

  // timeing for TaskDelayUntil
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t DISP_INTV_TICKS = pdMS_TO_TICKS(b2m_f/get_display_rate_hz_f());

  while (1) {

    // check if semaphore exists and then quickly check again.
    if (xSemaphoreTake(pool->display_mutex, 0) == pdTRUE) {
      ESP_LOGD(DISPTAG, ".... frame displayed ...");
      print_frame_data(fb->buf, fb->len);
      xSemaphoreGive(pool->display_mutex);

      vTaskDelayUntil(&xLastWakeTime, DISP_INTV_TICKS);
    }

  }
} // frame_display_task