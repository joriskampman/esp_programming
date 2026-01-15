#include <stdio.h>
#include "freertos/FreeRTOS.h"  // IWYU pragma: keep
#include "esp_heap_caps.h"
#include "freertos/task.h"
#include "portmacro.h"
#include "monitoring.h"
#include "constants.h"

static const float MEM_STAT_DISP_INTV_MS = 10.0f*b2m_f;
static const bool ENABLE_RUNTIME_LOG = true;

void print_memory_stats(mem_state_t* mem_state) {
  // create 1 big string that is the log

  // print the memory stats
  char log_buffer[512];  // single log buffer of 512 characters
  snprintf(log_buffer, sizeof(log_buffer),
    "\n--------       Monitoring memory and it's changes       ----------------\n" \
    "internal RAM\n" \
    "  total free: %0.1fMB (%+.0fkB change)" \
    "  largest free block: %0.1fMB (%+.0fkB change)" \
    "\n" \
    "PSRAM\n" \
    "  total free: %0.1fMB (%+.0fkB change)" \
    "  largest free block: %0.1fMB (%+.0fkB change)\n",
    mem_state->internal_total_free_mb,
    mem_state->delta_internal_total_free_mb*1000.f,
    mem_state->internal_largest_free_mb,
    mem_state->delta_internal_largest_free_mb*1000.f,
    mem_state->psram_total_free_mb,
    mem_state->delta_psram_total_free_mb*1000.0f,
    mem_state->psram_largest_free_mb,
    mem_state->delta_psram_largest_free_mb*1000.0f);

  // ESP_LOGD(MEM_TAG, "Monitoring memory and stack ...");
  printf(ANSI_DEBUG "%s" ANSI_RESET, log_buffer);

} // print_memory_stats

void update_mem_stats(mem_state_t* mem_state) {
  /*
  update the memory status numbers
  */
  float itfm = mem_state->internal_total_free_mb;
  float ilfm = mem_state->internal_largest_free_mb;
  float ptfm = mem_state->psram_total_free_mb;
  float plfm = mem_state->psram_largest_free_mb;

  // delay the task -> give back to scheduler
  mem_state->internal_total_free_mb =
  (float)heap_caps_get_free_size(MALLOC_CAP_INTERNAL)/1000000.0f;
  mem_state->internal_largest_free_mb =
  (float)heap_caps_get_free_size(MALLOC_CAP_INTERNAL)/1000000.0f;

  mem_state->psram_total_free_mb =
  (float)heap_caps_get_free_size(MALLOC_CAP_SPIRAM)/1000000.0f;
  mem_state->psram_largest_free_mb =
  (float)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)/1000000.0f;

  // fill in the deltas
  mem_state->delta_internal_total_free_mb = mem_state->internal_total_free_mb - itfm;
  mem_state->delta_internal_largest_free_mb = mem_state->internal_largest_free_mb - ilfm;
  mem_state->delta_psram_total_free_mb = mem_state->psram_total_free_mb - ptfm;
  mem_state->delta_psram_largest_free_mb = mem_state->psram_largest_free_mb - plfm;
}  // update_mem_stats

void mem_monitor_task(void* pvParameters) {
  // cast void pointer to mem_state_t
  mem_state_t* mem_state = (mem_state_t*)pvParameters;

  const TickType_t xDelay = pdMS_TO_TICKS(MEM_STAT_DISP_INTV_MS);  // every 10 seconds a memory check

  while (1) {
    // get the previous values in helper variables
    vTaskDelay(xDelay);

    update_mem_stats(mem_state);

    // print stats
    print_memory_stats(mem_state);
  } // while (1)
} // mem_monitor_task

void update_frame_stats(frame_stats_t* stats, uint32_t current_count, uint64_t timestamp) {
  if (stats->last_frame_count == 0) {
    // first frame
    stats->last_frame_count = current_count;
    stats->last_timestamp = timestamp;
    return;
  }

  // check for drops (frame count should increment by 1)
  uint32_t expected = stats->last_frame_count + 1;
  if (current_count > expected) {
    stats->dropped_frames += (current_count - expected);
    printf(ANSI_DEBUG "Frame drop detected! Expected %u, got %u\n" ANSI_RESET,
           expected, current_count);
  }  // more than expected -> frames are dropped

  // update stats
  stats->total_frames = current_count;
  stats->last_frame_count = current_count;

  // calculate actual fps
  if (timestamp > stats->last_timestamp) {
    float delta_s = (timestamp - stats->last_timestamp)*u2b_f;
    if (delta_s > 0) {
      stats->fps_actual = (1.0f/delta_s);
    }
  }
  stats->last_timestamp = timestamp;
}  // update_frame_stats()

void print_frame_stats(const frame_stats_t* stats) {
  printf(ANSI_DEBUG "Frame stats: Total=%u, Dropped=%u (%.1f%%), "
        "fps=%.1f/%.1f (actual/expected)\n" ANSI_RESET,
        stats->total_frames, stats->dropped_frames,
        (stats->total_frames > 0) ?
        (100.0f*stats->dropped_frames/stats->total_frames) : 0.0f,
        stats->fps_actual, stats->fps_expected);
}

void perf_mon_task(void *pvParameters) {
  // monitor task or all parameters to monitor during running
  mem_state_t* mem_state = (mem_state_t*) pvParameters;
  const TickType_t xDelay = pdMS_TO_TICKS(MEM_STAT_DISP_INTV_MS);

  // initialize frame stats
  mem_state->camera_stats.fps_expected = 5.0f;
  static char statsBuffer[512]; // preallocate buffer
  UBaseType_t stack_high_water;

  while (1) {
    // delay

    vTaskDelay(xDelay);

    // update memory state in-place
    update_mem_stats(mem_state);
    print_memory_stats(mem_state);
    print_frame_stats(&mem_state->camera_stats);

    // high water mark
    stack_high_water = uxTaskGetStackHighWaterMark(NULL);
    printf(ANSI_DEBUG "Task stack high water mark = %d of 4096 bytes\n" ANSI_RESET, stack_high_water);

    if (ENABLE_RUNTIME_LOG) {
      vTaskGetRunTimeStats(statsBuffer);

      printf(ANSI_SPECIAL "CPU statistics:\n%s\n" ANSI_RESET, statsBuffer);
    }
  }
}  // perf_mon_task
