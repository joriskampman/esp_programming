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

static const char* BUFTAG = "buffer pool";
static buffer_pool_t* fb_pool = NULL;

buffer_pool_t* get_fb_pool(void) {
  return fb_pool;
}

void buffer_pool_deinit(void) {
  buffer_pool_t* pool = get_fb_pool();
  if (!pool) {
    return;
  }

  // remove frames from the buffer
  for (int ibuf=0; ibuf < POOL_SIZE; ibuf++) {
    if (!pool->buffers[ibuf]) {
      esp_camera_fb_return(pool->buffers[ibuf]);  // returns the fb to the driver. frees memory
    }
  }

  vSemaphoreDelete(pool->free_sem);
  vSemaphoreDelete(pool->used_sem);
  vSemaphoreDelete(pool->mutex);

  // free the entire pool
  free(pool);
}  // buffer_pool_deinit


void buffer_pool_init(void) {
  fb_pool = malloc(sizeof(buffer_pool_t));
  if (!fb_pool) {
    ESP_LOGE(BUFTAG, "The buffer pool malloc failed!");
    return;
  }

  // initialize with zeros
  memset(fb_pool, 0, sizeof(buffer_pool_t));

  // initialize semaphores
  fb_pool->free_sem = xSemaphoreCreateCounting(POOL_SIZE, POOL_SIZE);  // 3 options, all free buffers
  fb_pool->used_sem = xSemaphoreCreateCounting(POOL_SIZE, 0);  // 3 options, no filled buffer
  fb_pool->mutex = xSemaphoreCreateMutex();  // even read-only is adviced to be done with mutex

  // error checking semaphores
  if (!fb_pool->free_sem || !fb_pool->used_sem || !fb_pool->mutex) {
    goto error;
  }
  // allocate camera buffers
  for (int ibuf = 0; ibuf < POOL_SIZE; ibuf++) {
    printf(ANSI_SPECIAL "A" ANSI_RESET);
    fb_pool->buffers[ibuf] = malloc(sizeof(camera_fb_t));

    // check if there
    printf(ANSI_SPECIAL "B" ANSI_RESET);
    if (!fb_pool->buffers[ibuf]) {
      goto error;
    }

    printf(ANSI_SPECIAL "C" ANSI_RESET);
    // initialize to all zeros
    memset(fb_pool->buffers[ibuf], 0, sizeof(camera_fb_t));

    size_t buffer_size = 320*240; // QVGA black and white
    fb_pool->buffers[ibuf]->buf = malloc(buffer_size);
    if (!fb_pool->buffers[ibuf]->buf) {
      printf(ANSI_SPECIAL "D" ANSI_RESET);
      goto error;
    }

    printf(ANSI_SPECIAL "E" ANSI_RESET);
    fb_pool->buffers[ibuf]->len = buffer_size;
    fb_pool->buffers[ibuf]->format = PIXFMT;
    // auto-calculate the size from a lookup table
    get_frame_dims(FRAMESIZE, &fb_pool->buffers[ibuf]->width, &fb_pool->buffers[ibuf]->height);
    printf(ANSI_SPECIAL "F" ANSI_RESET);
  }

  printf(ANSI_SPECIAL "G" ANSI_RESET);
  // initialize the indices
  fb_pool->write_idx = 0;
  fb_pool->read_idx = 0;

  return;

error:
  printf(ANSI_SPECIAL "H" ANSI_RESET);
  buffer_pool_deinit();
  fb_pool = NULL;
  printf(ANSI_SPECIAL "I" ANSI_RESET);
}  // buffer_pool_init

// -----------------------------------------------------------------------------------------
// helper functions
// -----------------------------------------------------------------------------------------
// producter: get empty file buffer
camera_fb_t* buffer_pool_get_empty(void) {
  buffer_pool_t* pool = get_fb_pool();
  xSemaphoreTake(pool->free_sem, portMAX_DELAY);  // infinite wait
  return pool->buffers[pool->write_idx];
}

// producer:signal buffer filled
void buffer_pool_signal_filled(void) {
  buffer_pool_t* pool = get_fb_pool();
  pool->write_idx = (pool->write_idx + 1)%POOL_SIZE;
  xSemaphoreGive(pool->used_sem);
}

// consumer: get filled buffer
camera_fb_t* buffer_pool_get_filled(void) {
  buffer_pool_t* pool = get_fb_pool();
  xSemaphoreTake(pool->used_sem, portMAX_DELAY);
  return pool->buffers[pool->read_idx];
}

// consumer:signal buffer processed
void buffer_pool_signal_processed(void) {
  buffer_pool_t* pool = get_fb_pool();
  pool->read_idx = (pool->read_idx + 1)%POOL_SIZE;
  xSemaphoreGive(pool->free_sem);
}

// display: peek at current buffer (non-destructive)
camera_fb_t* buffer_pool_peek(void) {
  buffer_pool_t* pool = get_fb_pool();
  // very short semaphore action.
  xSemaphoreTake(pool->mutex, portMAX_DELAY);
  camera_fb_t* frame = pool->buffers[pool->read_idx];
  xSemaphoreGive(pool->mutex);
  return frame;
}