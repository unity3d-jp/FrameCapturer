#pragma once

struct fcI420Data
{
    const void *y;
    const void *u;
    const void *v;
};

struct fcI420Image
{
    Buffer y;
    Buffer u;
    Buffer v;

    void resize(int width, int height);
    fcI420Data data();
};

void fcRGBA2I420(fcI420Image& dst, const void *rgba_pixels, int width, int height);
