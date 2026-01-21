/*
common functions, getters and setters of parameters
*/
#include "common.h"
#include <stdint.h>

// defined variables
static float display_rate_hz_f;
static float frame_rate_hz_f;
static float mon_interval_ms;

static float queue_depth_buffer_factor;
// static uint32_t queue_depth;

static uint32_t queue_depth;
static bool enable_perf_mon;  // turns off all others too!
static bool enable_runtime_mon;
static bool enable_mem_check;
static bool enable_frame_dropping_check;

void common_init(void) {
  display_rate_hz_f = 0.5f;
  frame_rate_hz_f = 5.0f;
  mon_interval_ms = 10.0f*b2m_f;

  queue_depth_buffer_factor = 1.2f;
  queue_depth = (uint32_t)(queue_depth_buffer_factor*(mon_interval_ms*m2b_f*frame_rate_hz_f));

  enable_perf_mon = true;  // turns off all others too!
  enable_runtime_mon = false;
  enable_mem_check = false;
  enable_frame_dropping_check = true;
}

// getters and setters
float get_display_rate_hz_f() {
  return display_rate_hz_f;
}

void set_display_rate_hz_f(float drate) {
  display_rate_hz_f = drate;
}

float get_frame_rate_hz_f() {
  return frame_rate_hz_f;
}

void set_frame_rate_hz_f(float frate) {
  frame_rate_hz_f = frate;
}

bool get_enable_runtime_mon() {
  return enable_runtime_mon;
}

void set_enable_runtime_mon(bool flag) {
  enable_runtime_mon = flag;
}

bool get_enable_mem_check() {
  return enable_mem_check;
}

void set_enable_mem_check(bool flag) {
  enable_mem_check = flag;
}

bool get_enable_frame_dropping_check() {
  return enable_frame_dropping_check;
}

void set_enable_frame_dropping_check(bool flag) {
  enable_frame_dropping_check = flag;
}

bool get_enable_perf_mon() {
  return enable_perf_mon;
}

void set_enable_perf_mon(bool flag) {
  enable_perf_mon = flag;
}

uint32_t get_queue_depth() {
  return queue_depth;
}

void set_queue_depth(uint32_t depth) {
  queue_depth = depth;
}

float get_mon_interval_ms() {
  return mon_interval_ms;
}

void set_mon_interval_ms(float interval_ms) {
  mon_interval_ms = interval_ms;
}