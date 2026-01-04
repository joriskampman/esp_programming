/*
This script grabs QVGA (320x240) frames from an ov5640 camera connected to the ESP32-S3 test board

It print outs - part of - the pixel values as a sanity check. Further processing will be added in
follow-up projects

author: Joris Kampman, Saxion SMART, winter 25/26
*/
// 1. configure the timer. using the cpu cycle counter
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() do { } while (0)
// 2. get the current timer value. esp_cpu_get_cycle_count() is perfect
#define portGET_RUN_TIME_COUNTER_VALUE() esp_cpu_get_cycle_count()

#include <stdio.h>
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "esp_camera.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "portmacro.h"
#include "math.h"
#include "esp_psram.h"
#include <stdbool.h>


// DEFINE PINOUT
// from: ../pics/pinout.avif
#define CAM_SIOD  4
#define CAM_SIOC  5
#define CAM_VSYNC 6
#define CAM_HREF  7
#define CAM_XCLK  15
#define CAM_Y9    16
#define CAM_Y8    17
#define CAM_Y7    18
#define CAM_Y4    8
#define CAM_Y3    9
#define CAM_Y5    10
#define CAM_Y2    11
#define CAM_Y6    12
#define CAM_PCLK  13

#define FRAMESIZE FRAMESIZE_QVGA
#define PIXFMT PIXFORMAT_GRAYSCALE

// name in the ESP_LOG<X> to identify where it comes from
static const char *TAG = "camera_stream";
static const bool enable_performance_logging = false;

// Global camera buffer
camera_fb_t *fb = NULL;

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

    .xclk_freq_hz = 24000000,
    .ledc_channel = LEDC_CHANNEL_0,
    .ledc_timer = LEDC_TIMER_0,
    .pixel_format = PIXFMT, // options: PIXFORMAT_[RGB565 | YUV422 | GRAYSCALE]
    .frame_size = FRAMESIZE,  // 320x240, fits on LED screen
    .jpeg_quality = 12,  // 1-63, lower is better quality, not used in grayscale
    .fb_count = 2,  //use 2 framebuffers, 1 for processing, 1 for filling at the same time
    .fb_location = CAMERA_FB_IN_PSRAM, // large framebuffers may only fit in PSRAM, alt: ..._DRAM
    .grab_mode = CAMERA_GRAB_LATEST, // alt: CAMERA_GRAB_WHEN_EMPTY (waits if none available)

  };

  // initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
    return err;
  }

  // configure OV3660 sensor settings
  sensor_t *p_sens = esp_camera_sensor_get();
  if(p_sens == NULL) {
    ESP_LOGE(TAG, "Failed to get get camera sensor.");
    return ESP_FAIL;
  }

  // OV3660 specific settings
  // reset to default values first
  p_sens->set_vflip(p_sens, 0);
  p_sens->set_hmirror(p_sens, 0);

  // set initial resolution
  if (p_sens->set_framesize(p_sens, FRAMESIZE) != 0) {
    ESP_LOGW(TAG, "failed to set framesize");
  }

  // disable special effects for clean output
  if (p_sens->set_special_effect(p_sens, 0) != 0) {
    ESP_LOGW(TAG, "Failed to set special effect");
  }

  // set auto-exposure
  if (p_sens->set_exposure_ctrl(p_sens, 1) != 0) {
    ESP_LOGW(TAG, "Failed to set exposure control");
  }

  // set auto white balance
  if (p_sens->set_whitebal(p_sens, 1) != 0) {
    ESP_LOGW(TAG, "Failed to set white balance");
  }

  // set auto gain control
  if (p_sens->set_gain_ctrl(p_sens, 1) != 0) {
    ESP_LOGW(TAG, "Failed to set gain control");
  }

  // all is OK
  ESP_LOGI(TAG, "Camera initialized successfully!");
  return ESP_OK;
}

void print_frame_data(const uint8_t *p_buf, size_t len) {
  // corner case: no buffer available
  if (p_buf == NULL || len == 0) {
    printf("ERROR: invalid buffer\n");
    return;
  }

  // todo: calculate some stats, for now print something
  printf(".. to implement: some statistics!");

  // print a set of pixel values
  printf("  sample pixels:\n");
  for (uint8_t ipix = 0; ipix < 10; ipix++) {
    printf("  %3.0f ", (float)p_buf[ipix]);
  }
  printf("\n");
}  // print_frame_data()

void camera_task(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(200);  // 2 Hz = 200 ms

  ESP_LOGI(TAG, "Camera task started");

  while (1) {
    // capture a frame
    camera_fb_t *fb = esp_camera_fb_get();

    if (enable_performance_logging) {
      static int frame_count = 0;
      if (++frame_count%10 == 0) {
        // 1. log heap info
        heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);

        // 2. LOG THIS TASKS' stack high water mark (in bytes)
        UBaseType_t stack_high_water = uxTaskGetStackHighWaterMark(NULL);
        ESP_LOGI(TAG, "Task stack high water mark: %.1fkB (max 4kB)", (float)stack_high_water/1000.0f);

        // 3. log psram-specific heap
        size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        ESP_LOGI(TAG, "Free PSRAM: %0.1fMB", (float)free_psram/1000000.0f);
      }
    }


    if (!fb) {
      ESP_LOGE(TAG, "Camera capture failed");
    } else {
      // print frame statistics
      print_frame_data(fb->buf, fb->len);

      // return the frame buffer
      esp_camera_fb_return(fb);
      fb = NULL;  // remove pointer

    }
    // wait for 5 Hz (200 ms)
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}  // camera_task()

void stats_task(void *pvParams) {
  const TickType_t xDelay = pdMS_TO_TICKS(30000);
  char statsBuffer[512]; // preallocate buffer

  while (1) {
    vTaskDelay(xDelay);

    printf("\n");

    // get the stats
    vTaskGetRunTimeStats(statsBuffer);

    // log the statsbuffer
    ESP_LOGI("CPU STATS", "\n%s", statsBuffer);

  }
}

void app_main(void) {
  // initialize NVS (non-volatile .. something?)
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
    ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ESP_ERROR_CHECK(nvs_flash_init());
    }

    // explicit initialization of PSRAM
    esp_err_t psram_result = esp_psram_init();
    if(psram_result != ESP_OK) {
      ESP_LOGE(TAG, "PSRAM init failed: 0x%x", psram_result);
      // continue anyway, camera will fall back to DRAM (will it though??)
  } else {
    ESP_LOGI(TAG, "PSRAM initialized successfully");
  }

  if (enable_performance_logging) {
    xTaskCreate(stats_task, "stats_task", 2048, NULL, 1, NULL);
  }

  // set console baud rate for better serial output
  esp_log_level_set("*", ESP_LOG_INFO);

  ESP_LOGI(TAG, "Starting camera application");

  // initialize camera
  ret = init_camera();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize camera");
    return;
  }

  // create camera task with sufficient stack size
  xTaskCreate(camera_task, "camera_task", 4096, NULL, 5, NULL);

  ESP_LOGI(TAG, "Camera stream task started at 5 Hz");

  // keep main task alive
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}  // app_main()