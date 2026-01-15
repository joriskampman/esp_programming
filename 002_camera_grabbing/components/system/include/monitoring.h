#ifndef PERFORMANCE_H
#define PERFORMANCE_H

#include "freertos/idf_additions.h"
#include <stdint.h>
#include <stdio.h>

extern QueueHandle_t frame_event_queue;

// TYPE DEFINITIONS
typedef struct {
  uint32_t frame_count;
  uint64_t timestamp;
  size_t frame_size;
} frame_event_t;

typedef struct {
  uint32_t total_frames;
  uint32_t dropped_frames;
  uint32_t last_frame_count;
  uint32_t last_timestamp;
  float fps_actual;
  float fps_expected;
} frame_stats_t;


typedef struct {
  //
  float total_elapsed_time_ms;  // the total elapsed time sine start
  float delta_time_ms;  // the delta time after last update

  // RAM
  float internal_total_free_mb;
  float internal_largest_free_mb;
  // PSRAM
  float psram_largest_free_mb;
  float psram_total_free_mb;

  // deltas (changes between frames)
  float delta_internal_total_free_mb;
  float delta_internal_largest_free_mb;
  float delta_psram_total_free_mb;
  float delta_psram_largest_free_mb;

  QueueHandle_t frame_event_queue;

  struct {
    uint32_t total_frames;
    uint32_t dropped_frames;
    uint32_t queue_overflows;
    float fps_actual;
    float fps_expected;
  } camera_stats;

} mem_state_t;

// function declarations/prototypes
void update_mem_stats(mem_state_t* mem_state);
void print_memory_stats(mem_state_t* mem_state);
void mem_monitor_task(void* pvParameters);
void perf_mon_task(void* pvParameters);

#endif  // PERFORMANCE_H