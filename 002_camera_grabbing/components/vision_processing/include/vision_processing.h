/*
vision_processing.h

contains the camera processing or vision processing for the camera frames
*/
#ifndef VISION_PROCESSING_H
#define VISION_PROCESSING_H
// #pragma once

#include <stdio.h>

// print the frame data --> removed when actual processing is done
void print_frame_data(const uint8_t *buf, size_t len);
void vision_processing_task(void*);

#endif