#include "esp_camera.h"

camera_fb_t* get_display_fb(void);
void frame_display_task(void* pvParameters);