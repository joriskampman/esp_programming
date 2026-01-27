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
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include <stdarg.h>


static const char* BUFTAG = "buffer pool";
static buffer_pool_t* fb_pool = NULL;

buffer_pool_t* get_buffer_pool(void) {
  return fb_pool;
}

void inc_idx(volatile int* idx) {
  *idx = (*idx + 1)%POOL_SIZE;
  print_buffer_use();
} // inc_idx

void upd_idx(volatile int* idx, int val) {
  /* this sets the index immediately */
  // check val
  if (val < 0 || val >= POOL_SIZE) {
    ESP_LOGE(BUFTAG, "The requested index value (%d) is not valid", val);
    return;
  }
  *idx = val;
  print_buffer_use();
}  // upd_idx

void print_buffer_use(void) {
  buffer_pool_t* pool = get_buffer_pool();
  // printf(ANSI_SPECIAL "write/proc index (%d/%d)", pool->write_idx, pool->proc_idx);
  printf(ANSI_SPECIAL "Buffers in use: ");
  for (int ibuf = 0; ibuf < POOL_SIZE; ibuf++) {
    printf(" | ");
    if (pool->buffer_in_use[ibuf]) {
      printf("U");
    } else {
      printf("-");
    }
    // check if there is a pointer to it
    if (pool->write_idx == ibuf) {
      printf("W");
    }
    if (pool->proc_idx == ibuf) {
      printf("P");
    }
    if (pool->disp_idx == ibuf) {
      printf("D");
    // } else {
    //   printf(" ");
    }
  } // loop buffers in pool
  printf(" |\n" ANSI_RESET);
} // print_buffer_use

void upd_buffer_in_use(int idx, bool val) {

  buffer_pool_t* fb = get_buffer_pool();
  ESP_LOGV(BUFTAG, "Setting buffer (%d) to %s", idx, val ? "USED": "FREE");
  fb->buffer_in_use[idx] = val;

  print_buffer_use();
}  // upd_buffer_in_use


void buffer_pool_deinit(void) {
  buffer_pool_t* pool = get_buffer_pool();
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
  for (int ibuf = 0; ibuf < POOL_SIZE+1; ibuf++) {
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
    upd_buffer_in_use(i, false);
  }

  ESP_LOGI(BUFTAG, "set mutex for display");
  fb_pool->display_mutex = xSemaphoreCreateMutex();

  ESP_LOGI(BUFTAG, "Initialize [W] to -1");
  fb_pool->write_idx = -1;
  ESP_LOGI(BUFTAG, "Initialize [P] to -1");
  fb_pool->proc_idx = -1;
  ESP_LOGI(BUFTAG, "Initialize [D] to 3");
  fb_pool->disp_idx = 3;  // fixed buffer, might be split off completely

  ESP_LOGI(BUFTAG, "Buffer pool initialized with %d buffers.", POOL_SIZE);
}  // buffer_pool_init

// -----------------------------------------------------------------------------------------
// helper functions
// -----------------------------------------------------------------------------------------
// producter: get empty file buffer
camera_fb_t* buffer_pool_get_write_buffer(void) {
  ESP_LOGV(BUFTAG, ">> buffer_pool_get_write_buffer()");
  // get pool
  buffer_pool_t* pool = get_buffer_pool();
  if (!pool) {
    ESP_LOGV(BUFTAG, "Buffer pool not valid");
    return NULL;
  }

  // check which buffer_in_use is false
  bool fnd = false;
  for (int ibuf=0; ibuf < POOL_SIZE; ibuf++) {
    if (!pool->buffer_in_use[ibuf]) {
      // ESP_LOGV(BUFTAG, "write buffer (%d) is free", ibuf);
      ESP_LOGV(BUFTAG, "Update [W] to %d", ibuf);
      upd_idx(&pool->write_idx, ibuf);
      fnd = true;
      break;
    }
  }

  if (!fnd) {
    ESP_LOGW(BUFTAG, "No free write buffers found! Overwrite old buffer that is not proc_idx or disp_idx");
    // check the pointers
    int ovw_idx = -1;
    for (int ibuf = 0; ibuf < POOL_SIZE; ibuf++) {
      if (ibuf != pool->proc_idx && ibuf != pool->disp_idx) {
        ovw_idx = ibuf;
      }
    }
    if (ovw_idx == -1) {
      ESP_LOGE(BUFTAG, "There is no buffer that can be overwritten. This cannot happen!!");
      return NULL;
    }
    ESP_LOGW(BUFTAG, "Overwrite [W] to %d", ovw_idx);
    upd_idx(&pool->write_idx, ovw_idx);
  }

  // update the buffer in use
  upd_buffer_in_use(pool->write_idx, true);

  // return the frame buffer
  return pool->buffers[pool->write_idx];
}

void buffer_pool_signal_filled(void) {
  ESP_LOGV(BUFTAG, ">> buffer_pool_signal_filled()");
  buffer_pool_t* pool = get_buffer_pool();
  if (!pool) {
    ESP_LOGW(BUFTAG, "Buffer pool has NULL pointer");
  }

  // update write index
  ESP_LOGV(BUFTAG, "Update [P] to %d", pool->write_idx);
  upd_idx(&pool->proc_idx, pool->write_idx);

  // release all buffers that are not proc_idx
  for (int ibuf=0; ibuf < POOL_SIZE; ibuf++) {
    // if is in use, but not processed -> set it free!
    if (pool->buffer_in_use[ibuf] && ibuf != pool->proc_idx) {
      upd_buffer_in_use(ibuf, false);
      // ESP_LOGV(BUFTAG, "Setting buffer (%d) free", ibuf);
    }
  }
  ESP_LOGV(BUFTAG, "update [W] to %d", pool->write_idx);
  inc_idx(&pool->write_idx);
}

// consumer: get filled buffer
camera_fb_t* buffer_pool_proc_get_filled(void) {
  ESP_LOGV(BUFTAG, "buffer_pool_proc_get_filled()");
  buffer_pool_t* pool = get_buffer_pool();

  // check if buffer pool is valid
  if (!pool) {
    ESP_LOGW(BUFTAG, "The buffer pool is a NULL pointer");
  }

  // check if proc_idx is already valid (startup behaviour)
  if (pool->proc_idx < 0 || pool->proc_idx >= POOL_SIZE) {
    ESP_LOGE(BUFTAG, "The proc_idx (%d) is not valid!", pool->proc_idx);
    return NULL;
  }

  if (!pool->buffer_in_use[pool->proc_idx]) {
    ESP_LOGE(BUFTAG, "processing buffer (%d) is somehow not marked as 'in use'", pool->proc_idx);
    return NULL;
  }

  // set the buffer_in_use at proc_idx to TRUE
  upd_buffer_in_use(pool->proc_idx, true);
  // ESP_LOGV(BUFTAG, "Processing id takes buffer %d", pool->proc_idx);
  return  pool->buffers[pool->proc_idx];
}

// consumer:signal buffer processed
void buffer_pool_signal_processed(void) {
  ESP_LOGV(BUFTAG, "buffer_pool_signal_processed()");

  buffer_pool_t* pool = get_buffer_pool();
  if (!pool) {
    ESP_LOGE(BUFTAG, "buffer pool pointer is NULL");
  }

  int iproc = pool->proc_idx;
  camera_fb_t* procbuf = pool->buffers[iproc];
  // processing done -> release buffer

  // take semaphore, wait if not available
  xSemaphoreTake(pool->display_mutex, portMAX_DELAY);

  // copy the frame to the buffer_pool[POOL_SIZE] (the display pool)
  camera_fb_t* fb_disp = pool->buffers[POOL_SIZE];  // the last is the display buffer
  memcpy(fb_disp->buf, procbuf->buf, procbuf->len);
  // now the meta data
  fb_disp->len = procbuf->len;
  fb_disp->width = procbuf->width;
  fb_disp->height = procbuf->height;
  fb_disp->format = procbuf->format;
  fb_disp->timestamp = procbuf->timestamp;
  ESP_LOGV(BUFTAG, "Copied the fb to display buffer position (%d) for display", POOL_SIZE);
  xSemaphoreGive(pool->display_mutex);

  // check if buffer is already released (should never happen)
  if (!pool->buffer_in_use[iproc]) {
    ESP_LOGE(BUFTAG, "The buffer for processing (%d) is already released. What gives?", iproc);
    return;
  }
}  // buffer_pool_signal_processed

