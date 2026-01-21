# this must be registered as an esp-idf component
idf_component_register(
  # source files, can be empty for header only
  SRCS "src/display.c"

  INCLUDE_DIRS "include"

  # # dependencies on other esp-idf components
  REQUIRES
    freertos
    esp32-camera
    buffer_pool
    camera

  # # private dependencies
  # PRIV_REQUIRES
)