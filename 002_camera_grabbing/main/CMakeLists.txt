idf_component_register(
  SRCS
    "app_main.c"

  INCLUDE_DIRS
    "."

  REQUIRES
    nvs_flash
    esp_psram
    common
    camera
    system
    esp32-camera
    buffer_pool
    display
    # vision_processing
    # webserver

  # note that there are no direct ESP-IDF components required.
  # They are via the components themselves
)
