#ifndef VISION_PROCESSING_H
#define VISION_PROCESSING_H
#pragma once

#include <stdio.h>
#include "common.h"
#include "buffer_pool.h"

// print the frame data --> removed when actual processing is done
void print_frame_data(const uint8_t *buf, size_t len);

#endif