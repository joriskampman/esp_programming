idf_component_register(
  SRCS
    "src/vision_processing.c"

  INCLUDE_DIRS
    "include"

  REQUIRES
    common

  # note camera will be built first due to this dependency here
)