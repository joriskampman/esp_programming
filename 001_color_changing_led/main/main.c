#include <stdint.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include "esp_err.h"
#include "freertos/projdefs.h"
#include "led_strip.h"
#include "led_strip_types.h"
#include "portmacro.h"
// #include "portmacro.h"

// define your LED strip GPIO pins
// adjust based on specific led type (what is it??)
#define LED_STRIP_GPIO_NUM      48
#define LED_STRIP_LED_NUM       1
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

static led_strip_handle_t led_strip;

static void configure_led_strip(void) {
  // step 1: basic strip configuration
  led_strip_config_t strip_config = {
    .strip_gpio_num = LED_STRIP_GPIO_NUM,
    .max_leds = LED_STRIP_LED_NUM,
    .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    .led_model = LED_MODEL_WS2812,
    .flags = { .invert_out = false,},
  };

  // step 2: configuration for the RMT backend (handles timing)
  led_strip_rmt_config_t rmt_config = {
    .clk_src = RMT_CLK_SRC_DEFAULT,     // default clock source
    .resolution_hz = 10 * 1000 * 1000,  // 10 MHz resolution
    .mem_block_symbols = 64,
    .flags.with_dma = 0,
  };

  // step 3: create the LED strip handle with RMT backend
  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

  // step 4: clear the led to start fresh
  ESP_ERROR_CHECK(led_strip_clear(led_strip));
}

typedef struct {
  const char *name;
  uint8_t r;
  uint8_t g;
  uint8_t b;
} rgb_t;

const rgb_t COLOR_RED = {"red", 255, 0, 0};
const rgb_t COLOR_GREEN = {"green", 0, 255, 0};
const rgb_t COLOR_BLUE = {"blue", 0, 0, 255};
const rgb_t COLOR_WHITE = {"white", 255, 255, 255};
const rgb_t COLOR_BLACK = {"black", 0, 0, 0};
const rgb_t COLOR_MAGENTA = {"magenta", 255, 0, 255};
const rgb_t COLOR_CYAN = {"cyan", 0, 255, 255};
const rgb_t COLOR_YELLOW = {"yellow", 255, 255, 0};
const rgb_t COLOR_ORANGE = {"orange", 255, 128, 0};

void set_led_color(led_strip_handle_t led_strip, const rgb_t* col) {
  ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, col->r, col->g, col->b));
  ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

void calc_intermediate_color(rgb_t *color, const rgb_t* from, const rgb_t* to, float frac) {
  // calculate an intermediate color based on the from and to colors and the fraction underway
  // color = from + (to - from)*fraction

  if (frac < 0.0f) { frac = 0.0f; }
  if (frac > 1.0f) { frac = 1.0f; }

  float fr = (float)from->r;
  float fg = (float)from->g;
  float fb = (float)from->b;

  float tr = (float)to->r;
  float tg = (float)to->g;
  float tb = (float)to->b;

  color->r = (uint8_t)(fr + frac*(tr - fr));
  color->g = (uint8_t)(fg + frac*(tg - fg));
  color->b = (uint8_t)(fb + frac*(tb - fb));
} // calc_intermediate_frequency

void apply_brightness(rgb_t* color, float brightness) {
  // apply brightness (between 0.0 and 1.0) to a color
  // corner cases:
  if (brightness < 0.0f) { brightness = 0.0f; }
  if (brightness > 1.0f) { brightness = 1.0f; }

  color->r = (uint8_t)((float)color->r*brightness + 0.5f);
  color->g = (uint8_t)((float)color->g*brightness + 0.5f);
  color->b = (uint8_t)((float)color->b*brightness + 0.5f);
} // apply_brightness

typedef struct {
  float brightness;
  uint8_t interpolation_steps;
  float change_frequency_hz;
  uint8_t loop_count;
} animation_config_t;


typedef struct {
  uint8_t ifrom;
  uint8_t ito;
  int8_t dir;
} animation_state_t;

void determine_next_state(animation_state_t* state, size_t palette_size) {
    if (state->ifrom == (palette_size - 1) && state->dir == 1) {
      state->dir = -1;  // reverse direction
      state->ito = state->ifrom - 1;
      printf("change direction from forwards to backwards\n");
    } else if (state->ito == 0 && state->dir == -1) {
      state->dir = +1;
      state->ifrom = 0;
      state->ito = 1;
      printf("change direction from backwards to forwards\n");
    } else {
      state->ito = (uint8_t)(state->ifrom + state->dir);
    }

} // determine_next_state

void blinker(uint8_t nof_blinks, float delay_ms, led_strip_handle_t led_strip, const rgb_t* color) {
  // blink twice to signal start and finish
  for (uint8_t iblink = 0; iblink < nof_blinks; iblink++) {
    set_led_color(led_strip, color);
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    ESP_ERROR_CHECK(led_strip_clear(led_strip));
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
  }
}

// add a colors 'palette' for easier looping
const rgb_t* PALETTE[] = {&COLOR_RED,
                          &COLOR_ORANGE,
                          &COLOR_YELLOW,
                          &COLOR_GREEN,
                          &COLOR_CYAN,
                          &COLOR_BLUE,
                          &COLOR_MAGENTA,
                          &COLOR_RED};
#define PALETTE_SIZE ARRAY_SIZE(PALETTE)

const animation_config_t DEFAULT_CONFIG = {
  .brightness = 0.1f,  // scale the values with
  .loop_count = 4,
  .interpolation_steps = 20,
  .change_frequency_hz = 10.0
};


// ---------------- main program -------------------------------------------------
void app_main(void)
{
  // initialize LED strip
  configure_led_strip();

  printf("ESP32-S3 WS2812 Blink test started on GPIO %d\n", LED_STRIP_GPIO_NUM);

  blinker(2, 250.0f, led_strip, &COLOR_WHITE);
  TickType_t delay_ticks = pdMS_TO_TICKS(1000.0f/DEFAULT_CONFIG.change_frequency_hz);

  // cycles through these colors
  animation_state_t state = {
    .ifrom = 0,
    .ito = 1,
    .dir = 1,
  };

  // print a list of all colors
  printf("The lights takes the following colors in order:\n");
  for (uint8_t i = 0; i < PALETTE_SIZE; i++) {
    printf(" (%d) %s\n", i, PALETTE[i]->name);
  }

  printf("Delay in ticks is %" PRIu32 "\n\n", delay_ticks);

  // some colors that update
  rgb_t col;
  float frac;

  for (uint8_t iloop = 0; iloop < DEFAULT_CONFIG.loop_count; iloop++) {
    printf("\nloop %d of %d\n\n", iloop + 1, DEFAULT_CONFIG.loop_count);
    for (uint8_t ipass = 0; ipass < PALETTE_SIZE-1; ipass++) {

      // determine next colors/state
      determine_next_state(&state, PALETTE_SIZE);

      // initialize to the first color index
      set_led_color(led_strip, PALETTE[state.ifrom]);

      printf("looping from (%d) %s to (%d) %s\n",
        state.ifrom,
        PALETTE[state.ifrom]->name,
        state.ito,
        PALETTE[state.ito]->name);

      // loop the intermediate colors
      for (uint8_t istep = 0; istep < DEFAULT_CONFIG.interpolation_steps; istep++) {
        frac = (float)(istep + 1)/((float)DEFAULT_CONFIG.interpolation_steps); // note the +1

        calc_intermediate_color(&col, PALETTE[state.ifrom], PALETTE[state.ito], frac);
        apply_brightness(&col, DEFAULT_CONFIG.brightness);

        // set the color
        set_led_color(led_strip, &col);

        // calculate the delay
        vTaskDelay(delay_ticks);
      }
      // change ifrom to next index
      state.ifrom = (uint8_t)(state.ifrom + state.dir);
    }
  }

  printf("%d loops performed. Function stopped!\n\n", DEFAULT_CONFIG.loop_count);
  blinker(2, 250.0f, led_strip, &COLOR_WHITE);

} // main
