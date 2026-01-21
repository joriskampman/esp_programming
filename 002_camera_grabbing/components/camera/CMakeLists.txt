idf_component_register(
  SRCS
    "src/camera.c"

  INCLUDE_DIRS
    "include"

  REQUIRES
    common
    freertos
    esp32-camera
    vision_processing
    system
    buffer_pool


  # PRIV_REQUIRES
)