#pragma once

enum fcPixelFormat;
int fcGetPixelSize(fcPixelFormat format);

void fcImageFlipY(void *image_, int width, int height, fcPixelFormat fmt);

class half;
void fcScaleArray(uint8_t *data, size_t size, float scale);
void fcScaleArray(uint16_t *data, size_t size, float scale);
void fcScaleArray(int32_t *data, size_t size, float scale);
void fcScaleArray(half *data, size_t size, float scale);
void fcScaleArray(float *data, size_t size, float scale);
const void* fcConvertPixelFormat(void *dst, fcPixelFormat dstfmt, const void *src, fcPixelFormat srcfmt, size_t size);
