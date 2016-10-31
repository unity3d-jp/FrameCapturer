#pragma once

#include "Buffer.h"

struct I420Data
{
    void *y;
    void *u;
    void *v;
};

class I420Image
{
public:
    void resize(int width, int height);
    size_t size() const;
    const I420Data& data() const;

private:
    Buffer m_buffer;
    I420Data m_data;
};

void RGBA2I420(I420Image& dst, const void *rgba_pixels, int width, int height);
void RGBA2I420(const I420Data& dst, const void *rgba_pixels, int width, int height);
