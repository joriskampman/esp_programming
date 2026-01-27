#include "freertos/FreeRTOS.h"  // IWYU pragma: keep
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "camera.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "portmacro.h"
#include "vision_processing.h"
#include "common.h"
#include "monitoring.h"
#include <stdint.h>
#include "buffer_pool.h"

static const char* CAM_TAG = "camera";

void get_frame_dims(framesize_t size, size_t *width, size_t *height) {
  // lookup table for standard resolutions
  static const int dims[][2] = {
    [FRAMESIZE_96X96]     = {96, 96},
    [FRAMESIZE_QQVGA]     = {160, 120},
    [FRAMESIZE_QCIF]      = {176, 144},
    [FRAMESIZE_HQVGA]     = {240, 176},
    [FRAMESIZE_240X240]   = {240, 240},
    [FRAMESIZE_QVGA]      = {320, 240},
    [FRAMESIZE_CIF]       = {400, 296},
    [FRAMESIZE_HVGA]      = {480, 320},
    [FRAMESIZE_VGA]       = {640, 480},
    [FRAMESIZE_SVGA]      = {800, 600},
    [FRAMESIZE_XGA]       = {1024, 768},
    [FRAMESIZE_HD]        = {1280, 720},
    [FRAMESIZE_SXGA]      = {1280, 1024},
    [FRAMESIZE_UXGA]      = {1600, 1200},
  };

  if (size >= 0 && size < sizeof(dims)/(2*sizeof(int))) {
    *width = dims[size][0];
    *height = dims[size][1];
  } else {
    ESP_LOGE(CAM_TAG, "The framesize enumerator %s is not a valid key", size);
    return;
  }
}  // get_frame_dims


float get_xclk_freq_hz_f() {
  return DATA_RATE_HZ;
}

// getter for frame buffer count (queue related)
float get_fb_count() {
  return FB_COUNT;
}

// function to initialize camera --> init_camera()
esp_err_t init_camera(void) {
  camera_config_t config = {
    .pin_pwdn = -1,
    .pin_reset = -1,
    .pin_xclk = CAM_XCLK,
    .pin_sccb_sda = CAM_SIOD,
    .pin_sccb_scl = CAM_SIOC,

    .pin_d7 = CAM_Y9,
    .pin_d6 = CAM_Y8,
    .pin_d5 = CAM_Y7,
    .pin_d4 = CAM_Y6,
    .pin_d3 = CAM_Y5,
    .pin_d2 = CAM_Y4,
    .pin_d1 = CAM_Y3,
    .pin_d0 = CAM_Y2,

    .pin_vsync = CAM_VSYNC,
    .pin_href = CAM_HREF,
    .pin_pclk = CAM_PCLK,

    .xclk_freq_hz = get_xclk_freq_hz_f(),
    .ledc_channel = LEDC_CHANNEL_0,
    .ledc_timer = LEDC_TIMER_0,
    .pixel_format = PIXFMT, // options: PIXFORMAT_[RGB565 | YUV422 | GRAYSCALE]
    .frame_size = FRAMESIZE,  // 320x240, fits on LED screen
    .jpeg_quality = 12,  // 1-63, lower is better quality, not used in grayscale
    .fb_count = get_fb_count(),  //use 2 framebuffers, 1 for processing, 1 for filling at the same time
    .fb_location = CAMERA_FB_IN_PSRAM, // large framebuffers may only fit in PSRAM, alt: ..._DRAM
    .grab_mode = CAMERA_GRAB_LATEST, // alt: CAMERA_GRAB_WHEN_EMPTY (waits if none available)

  };

  // initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    ESP_LOGE(CAM_TAG, "Camera init failed with error 0x%x", err);
    return err;
  }

  // configure OV3660 sensor settings
  sensor_t *p_sens = esp_camera_sensor_get();
  if(p_sens == NULL) {
    ESP_LOGE(CAM_TAG, "Failed to get get camera sensor.");
    return ESP_FAIL;
  }

  // OV3660 specific settings
  // reset to default values firstnu
  p_sens->set_vflip(p_sens, 0);
  p_sens->set_hmirror(p_sens, 0);

  // set initial resolution
  if (p_sens->set_framesize(p_sens, FRAMESIZE) != 0) {
    ESP_LOGW(CAM_TAG, "failed to set framesize");
  }

  // disable special effects for clean output
  if (p_sens->set_special_effect(p_sens, 0) != 0) {
    ESP_LOGW(CAM_TAG, "Failed to set special effect");
  }

  // set auto-exposure
  if (p_sens->set_exposure_ctrl(p_sens, 1) != 0) {
    ESP_LOGW(CAM_TAG, "Failed to set exposure control");
  }

  // set auto white balance
  if (p_sens->set_whitebal(p_sens, 1) != 0) {
    ESP_LOGW(CAM_TAG, "Failed to set white balance");
  }

  // set auto gain control
  if (p_sens->set_gain_ctrl(p_sens, 1) != 0) {
    ESP_LOGW(CAM_TAG, "Failed to set gain control");
  }

  // all is OK
  ESP_LOGI(CAM_TAG, "Camera initialized successfully!");
  return ESP_OK;
}


void camera_task(void *pvParameters) {
  // get the camera task parameters
  camera_tasks_params_t* cam_pars = (camera_tasks_params_t*)pvParameters;

  printf(ANSI_SPECIAL "fb_pool pointer %p\n" ANSI_RESET, &cam_pars->fb_pool);
  static uint32_t frame_counter = 0;

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t REFRESH_INTERVAL_TICK = pdMS_TO_TICKS(b2m_f/get_frame_rate_hz_f());  // 2 Hz = 200 ms

  ESP_LOGI(CAM_TAG, "Camera task started");

  while (1) {
    // capture a frame
    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb) {
      ESP_LOGE(CAM_TAG, "Camera capture failed");
    } else {
      // create frame event for monitoring
      uint64_t timestamp_us = (uint64_t)fb->timestamp.tv_sec*1000000UL
      + (uint64_t)fb->timestamp.tv_usec;

      // frame_event_t event = {
      //   .frame_count = frame_counter++,
      //   .timestamp = timestamp_us,
      //   .frame_size = fb->len
      // };

      // place the frame in the buffer pool if there is a free_sem available
      camera_fb_t* fb_empty = buffer_pool_get_write_buffer();

      // copy the fb to the fb_empty
      memcpy(fb_empty->buf, fb->buf, fb->len);
      // now the meta data
      fb_empty->len = fb->len;
      fb_empty->width = fb->width;
      fb_empty->height = fb->height;
      fb_empty->format = fb->format;
      fb_empty->timestamp = fb->timestamp;

      // update the write idx and giving the free_sem bacck
      buffer_pool_signal_filled();

      // print_frame_data(fb->buf, fb->len);

      // send the frame actually to the queue
      // if (queue != NULL) {
      //   if (xQueueSend(queue, &event, 0) != pdTRUE) {
      //     ESP_LOGW(CAM_TAG, "Queue full, dropping frame %lu", frame_counter-1);
      //   }
      // }

      // return the frame buffer
      esp_camera_fb_return(fb);
      fb = NULL;  // remove pointer

    }
    // wait for 5 Hz (200 ms)
    vTaskDelayUntil(&xLastWakeTime, REFRESH_INTERVAL_TICK);
  }
}  // camera_task()

