#include <stdio.h>
#include "vision_processing.h"
#include "esp_log.h"
#include "common.h"

static const char* VPTAG = "vision processing";

void vision_processing_task(void* pvParameters) {
  // get a frame buffer  from pool
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t REFRESH_INTERVAL_TICK = pdMS_TO_TICKS(b2m_f/get_vidproc_rate_hz_f());  // 2 Hz = 200 ms

  while (1) {
    // get the previous wake time
    camera_fb_t* fb = buffer_pool_get_filled();

    // check if the fb is correct
    if (!fb || !fb->buf || fb->len == 0) {
      ESP_LOGE(VPTAG, "Received frame from buffer pool is incorrect!");
      if (fb != NULL) {
        // move read pointer 1 further
        buffer_pool_signal_processed();  // do semaphore handling correctly
      }  // fb itself is not NULL

      // move to end
      continue;
    }  // if: check buffer correctness

    // check over, buffer correct
    ESP_LOGI(VPTAG, "Processing frame: %d bytes, %dx%d, format: %d",
            fb->len, fb->width, fb->height, fb->format);

    print_frame_data(fb->buf, fb->len);

    // move semaphores again to signal it is done
    buffer_pool_signal_processed();

    // add the delay
    vTaskDelayUntil(&xLastWakeTime, REFRESH_INTERVAL_TICK);
  }  // while (1)

}  // vision_processing_task


void print_frame_data(const uint8_t *p_buf, size_t len) {
  // corner case: no buffer available
  if (p_buf == NULL || len == 0) {
    ESP_LOGE(VPTAG, "Not a valid frame buffer found. NULL pointers");
    return;
  }

  // todo: calculate some stats, for now print something

  // print a set of pixel values
  for (uint8_t ipix = 0; ipix < 10; ipix++) {
    printf("  %3.0f ", (float)p_buf[ipix]);
  }

  printf("\n");
}  // print_frame_data()
