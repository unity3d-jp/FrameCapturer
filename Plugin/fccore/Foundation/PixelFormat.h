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
fcAPI const void* fcConvertPixelFormat(void *dst, fcPixelFormat dstfmt, const void *src, fcPixelFormat srcfmt, size_t size);

// audio sample conversion
void fcF32ToU8Samples(uint8_t *dst, const float *src, size_t size);
void fcF32ToI16Samples(int16_t *dst, const float *src, size_t size);
void fcF32ToI24Samples(uint8_t *dst, const float *src, size_t size);
void fcF32ToI32Samples(int32_t *dst, const float *src, size_t size, float scale);
