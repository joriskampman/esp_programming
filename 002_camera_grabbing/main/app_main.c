/*
This script grabs QVGA (320x240) frames from an ovXXXX camera connected to the ESP32-S3 test board

It print outs - part of - the pixel values as a sanity check. Further processing will be added in
follow-up projects

author: Joris Kampman, Saxion SMART, winter 25/26
*/

#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"  // IWYU pragma: keep
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "portmacro.h"  // IWYU pragma: keep  (contains portCONFIGURE_TIMER_FOR_RUN_TIME_STATS etc)
#include "esp_psram.h"
#include <stdbool.h>
#include "monitoring.h"
#include "camera.h"
#include "common.h"

static const char* TAG = "main";

// creates a queueu (analog: post office with 10 mailboxes)
QueueHandle_t frame_event_queue = NULL;  // initialize to NULL (can't be an initializer)

void app_main(void) {
  common_init();

  // initialize NVS (non-volatile .. something?)
  esp_err_t ret = ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_flash_init());

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
    ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ESP_ERROR_CHECK(nvs_flash_init());
    }

    // explicit initialization of PSRAM
    esp_err_t psram_result = esp_psram_init();
    if(psram_result != ESP_OK) {
      ESP_LOGE(TAG, "PSRAM init failed: 0x%x", psram_result);
      ESP_LOGW(TAG, "Camera may use internal RAM only - frames may be smaller.");
      // continue anyway, camera will fall back to DRAM (will it though??)
  } else {
    ESP_LOGI(TAG, "PSRAM initialized successfully");
  }

  // create Queue, if fail --> return
  frame_event_queue = xQueueCreate(get_queue_depth(), sizeof(frame_event_t));
  if (frame_event_queue == NULL) {
    ESP_LOGE(TAG, "Failed to create frame event queue.");
    return;
  }

  // initialize perf_mon
  static perf_mon_t perf_mon = {0};
  perf_mon.frame_event_queue = frame_event_queue;
  perf_mon.frame_stats.fps_expected = 5.0f;  // TODO: parameterize

  // memory monitoring
  if (get_enable_perf_mon()) {
    ESP_LOGI(TAG, "Starting memory monitoring task");
    xTaskCreate(perf_mon_task, "perf_mon_task", 4096, &perf_mon, 1, NULL);  // task has lower prio
  }

  // set console baud rate for better serial output
  esp_log_level_set("main", ESP_LOG_INFO);
  esp_log_level_set("camera", ESP_LOG_INFO);

  ESP_LOGI(TAG, "Starting camera application");

  // initialize camera
  ESP_LOGI(TAG, "Initialize camera");
  if (init_camera() != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize camera");
    return;
  }

  // create camera task with sufficient stack size
  camera_tasks_params_t camera_params = {
    .frame_event_queue = frame_event_queue,
  };

  ESP_LOGI(TAG, "starting camera_task()");
  TaskHandle_t camera_task_handle;
  if (xTaskCreate(camera_task, "camera_task", 4096, &camera_params, 5, &camera_task_handle) != pdPASS) {
    ESP_LOGE(TAG, "Failed to create camera task");
    return;
  }
  ESP_LOGI(TAG, "Camera stream task started at 5 Hz");

  // keep main task alive
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}  // app_main()