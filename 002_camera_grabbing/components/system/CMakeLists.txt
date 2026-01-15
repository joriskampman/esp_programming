idf_component_register(
  SRCS
    "src/monitoring.c"

  INCLUDE_DIRS
    "include"

  REQUIRES
    common
    freertos
    heap        # for memory stats
    esp_timer   # for timing functions

  # PRIV_REQUIRES
)