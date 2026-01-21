#include "buffer_pool.h"

camera_fb_t* get_display_fb(void);
void update_display_fb(buffer_pool_t*);

void frame_display_task(void* pvParameters);