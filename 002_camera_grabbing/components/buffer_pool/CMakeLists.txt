# this must be registered as an esp-idf component
idf_component_register(
  # source files, can be empty for header only
  SRCS "src/buffer_pool.c"

  INCLUDE_DIRS "include"

  # # dependencies on other esp-idf components
  REQUIRES
    camera
    freertos
    esp32-camera

  # # private dependencies
  # PRIV_REQUIRES
)