/*
This is the TEMPLATE project. It serves as a starting point and sanity check.
Replace with new project code after the sanity check is passed and the device is working OK.

author: Joris Kampman, Saxion SMART, winter 25/26
*/
#include <stdio.h>
#include "esp_log.h"
#include "freertos/projdefs.h"
#include "led_strip.h"
#include "led_strip_rmt.h"
#include "portmacro.h"

#define WS2812B_PIN 48
#define RMT_RESOLUTION_HZ 10000000  // 10 MHz
#define LED_COUNT 1
#define NOF_BLINKS 2

static const char *TAG = "TEMPLATE";

void app_main(void) {
  // print startup message
  ESP_LOGI(TAG, "=================================================================");
  ESP_LOGI(TAG, "Hello world, I am a new template!");
  ESP_LOGI(TAG, "ESP32-S3 Template project.");
  ESP_LOGI(TAG, "Sanity check: LED will blink red, white and blue");
  ESP_LOGI(TAG, "=================================================================");
  ESP_LOGI(TAG, "");

  // configure RMT peripheral
  led_strip_config_t strip_config = {
    .strip_gpio_num = WS2812B_PIN,
    .max_leds = LED_COUNT,
    .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    .led_model = LED_MODEL_WS2812,
  };

  led_strip_rmt_config_t rmt_config = {
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = RMT_RESOLUTION_HZ,
    .mem_block_symbols = 64,  // might be overkill, but it's OK for now
  };

  // led strip handle
  led_strip_handle_t led_strip;
  led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);

  // clear all leds
  led_strip_clear(led_strip);

  float delay_ms = 800.0f;
  TickType_t delay_ticks = pdMS_TO_TICKS(delay_ms);
  float fraction = 0.75f;

  uint8_t iloop = 1;
  while (delay_ticks > 0) {
    printf("loop %d delay %.1f [ms] (= %" PRIu32 " ticks)\n", iloop, delay_ms, delay_ticks);
    led_strip_set_pixel(led_strip, 0, 255, 0, 0);
    led_strip_refresh(led_strip);
    vTaskDelay(delay_ticks);
    led_strip_clear(led_strip);
    vTaskDelay(delay_ticks);
    led_strip_set_pixel(led_strip, 0, 83, 83, 83);
    led_strip_refresh(led_strip);
    vTaskDelay(delay_ticks);
    led_strip_clear(led_strip);
    vTaskDelay(delay_ticks);
    led_strip_set_pixel(led_strip, 0, 0, 0, 255);
    led_strip_refresh(led_strip);
    vTaskDelay(delay_ticks);
    led_strip_clear(led_strip);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // calculate new delay in ms/ticks
    iloop++;
    delay_ms *= fraction;
    delay_ticks = pdMS_TO_TICKS(delay_ms);
  }  // loop led loop

  ESP_LOGI(TAG, "sanity check complete. Device appears sane!");
}  // app_main()