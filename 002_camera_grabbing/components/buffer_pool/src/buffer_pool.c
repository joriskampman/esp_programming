/*
buffer_pool.c

contains init and deinit
*/
// #include "freertos/FreeRTOS.h" // IWYU pragma: keep
// #include "freertos/semphr.h"
#include "buffer_pool.h"
#include "esp_log.h"
// #include "sensor.h"
#include "camera.h"
#include "freertos/projdefs.h"

static const char* BUFTAG = "buffer pool";
static buffer_pool_t* fb_pool = NULL;

buffer_pool_t* get_fb_pool(void) {
  return fb_pool;
}

void buffer_pool_deinit(void) {
  buffer_pool_t* pool = get_fb_pool();
  if (!pool) {
    ESP_LOGV(BUFTAG, "The buffer pool points is already NULL");
    return;
  }

  // remove frames from the buffer
  for (int ibuf=0; ibuf < POOL_SIZE; ibuf++) {
    if (pool->buffers[ibuf]) {
      // returns the fb to the JPEG peripheral. frees memory
      esp_camera_fb_return(pool->buffers[ibuf]);
    }
  }

  // free the entire pool
  free(pool);

  // set the pointer to NULL
  fb_pool = NULL;
}  // buffer_pool_deinit


void buffer_pool_init(void) {
  fb_pool = malloc(sizeof(buffer_pool_t));
  if (!fb_pool) {
    ESP_LOGE(BUFTAG, "The buffer pool malloc failed!");
    return;
  }

  // initialize with zeros
  memset(fb_pool, 0, sizeof(buffer_pool_t));

  // allocate camera buffers
  for (int ibuf = 0; ibuf < POOL_SIZE; ibuf++) {
    fb_pool->buffers[ibuf] = malloc(sizeof(camera_fb_t));

    // check if there
    if (!fb_pool->buffers[ibuf]) {
      ESP_LOGE(BUFTAG, "Buffer %d allocation failed!");
      buffer_pool_deinit();
      return;
    }

    // initialize to all zeros
    memset(fb_pool->buffers[ibuf], 0, sizeof(camera_fb_t));

    size_t buffer_size = 320*240; // QVGA black and white
    fb_pool->buffers[ibuf]->buf = malloc(buffer_size);
    if (!fb_pool->buffers[ibuf]->buf) {
      ESP_LOGE(BUFTAG, "buffer data allocation failed!");
      free(fb_pool->buffers[ibuf]);
      fb_pool->buffers[ibuf] = NULL;
      buffer_pool_deinit();
      return;
    }

    fb_pool->buffers[ibuf]->len = buffer_size;
    fb_pool->buffers[ibuf]->format = PIXFMT;
    // auto-calculate the size from a lookup table
    get_frame_dims(FRAMESIZE,
          &fb_pool->buffers[ibuf]->width,
          &fb_pool->buffers[ibuf]->height);
  }

  // initialize the indices
  for (int i=0; i < POOL_SIZE; i++) {
    fb_pool->buffer_in_use[i] = false;
  }
  fb_pool->write_idx = -1;
  fb_pool->proc_idx = -1;
  fb_pool->disp_idx = -1;

  ESP_LOGI(BUFTAG, "Buffer pool initialized with %d buffers.", POOL_SIZE);
}  // buffer_pool_init

// -----------------------------------------------------------------------------------------
// helper functions
// -----------------------------------------------------------------------------------------
void inc_idx(volatile int* idx) {
  *idx = (*idx + 1)%POOL_SIZE;
}

// producter: get empty file buffer
camera_fb_t* buffer_pool_get_empty(void) {
  // get pool
  buffer_pool_t* pool = get_fb_pool();
  if (!pool) {
    return NULL;
  }

  // check which buffer_in_use is false
  bool fnd = false;
  for (int ibuf=0; ibuf < POOL_SIZE; ibuf++) {
    if (!pool->buffer_in_use[ibuf]) {
      ESP_LOGV(BUFTAG, "write buffer (%d) is free", ibuf);
      pool->write_idx = ibuf;
      pool->buffer_in_use[ibuf] = true;
      fnd = true;
      break;
    }
  }

  if (!fnd) {
    ESP_LOGE(BUFTAG, "No free write buffers found!");
    return NULL;
  }
  return pool->buffers[pool->write_idx];
}

// producer:signal buffer filled
void buffer_pool_signal_filled(void) {
  buffer_pool_t* pool = get_fb_pool();
  if (!pool) {
    ESP_LOGW(BUFTAG, "Buffer pool has NULL pointer");
  }

  // update write index
  pool->proc_idx = pool->write_idx;
  inc_idx(&pool->write_idx);
  ESP_LOGV(BUFTAG, "update write_idx = %d", pool->write_idx);


}

// consumer: get filled buffer
camera_fb_t* buffer_pool_proc_get_filled(void) {
  buffer_pool_t* pool = get_fb_pool();
  if (!pool) {
    ESP_LOGW(BUFTAG, "The buffer pool is a NULL pointer");
  }

  // check if buffer is not in use
  if (pool->buffer_in_use[pool->proc_idx]) {
    ESP_LOGW(BUFTAG, "processing buffer index is already in use! (by whom?)");
    return NULL;
  }

  // set the buffer_in_use at proc_idx to TRUE
  pool->buffer_in_use[pool->proc_idx] = true;
  pool->disp_idx = pool->proc_idx;  // same index, but cannot start, because in use
  ESP_LOGV(BUFTAG, "Processing id takes buffer %d", pool->proc_idx);
  return  pool->buffers[pool->proc_idx];
}

// consumer:signal buffer processed
void buffer_pool_signal_processed(void) {
  buffer_pool_t* pool = get_fb_pool();
  if (!pool) {
    ESP_LOGE(BUFTAG, "buffer pool pointer is NULL");
  }

  int iproc = pool->proc_idx;
  // processing done -> release buffer

  // check if buffer is already released (should never happen)
  if (!pool->buffer_in_use[iproc]) {
    ESP_LOGE(BUFTAG, "The buffer for processing (%d) is already released. What gives?", iproc);
    return;
  }

  // release the buffer_in_use
  pool->buffer_in_use[iproc] = false;
  ESP_LOGV(BUFTAG, "Processing buffer (%d) is released", iproc);

}

// display: peek at current buffer (non-destructive)
camera_fb_t* buffer_pool_display_get_filled(void) {
  buffer_pool_t* pool = get_fb_pool();
  if (!pool) {
    ESP_LOGE(BUFTAG, "The buffer point is a NULL pointer");
    return NULL;
  }

  int idisp = pool->disp_idx;

  // ---- claim the buffer -------------
  // check if buffer is in use
  while (pool->buffer_in_use[idisp]) {

    // wait for next iteration
    vTaskDelay(pdMS_TO_TICKS(10.0f));
  }

  // now buffer is free
  pool->buffer_in_use[idisp] = true;
  ESP_LOGV(BUFTAG, "display buffer (%d) is claimed", idisp);

  return pool->buffers[idisp];
}

void buffer_pool_signal_displayed(void) {
  buffer_pool_t* pool = get_fb_pool();
  if (!pool) {
    ESP_LOGW(BUFTAG, "Buffer pool pointer is a NULL pointer");
  }

  int idisp = pool->disp_idx;

  // check if already released (should not happen)
  if (!pool->buffer_in_use[idisp]) {
    ESP_LOGE(BUFTAG, "The display buffer (%d) is already released. That's just weird!");
    return;
  }

  // release the buffer
  pool->buffer_in_use[idisp] = false;
  ESP_LOGV(BUFTAG, "Display buffer (%d) is released!");

}