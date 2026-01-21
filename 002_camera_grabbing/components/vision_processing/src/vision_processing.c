#include <stdio.h>
#include "vision_processing.h"
#include "esp_log.h"

static const char* VPTAG = "vision processing";

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
