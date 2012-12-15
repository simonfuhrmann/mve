/*
 * Image drawing utilities.
 * Written by Simon Fuhrmann.
 */

#ifndef MVE_IMAGEDRAWING_HEADER
#define MVE_IMAGEDRAWING_HEADER

#include <algorithm>
#include <cmath>

#include "defines.h"
#include "mve/image.h"

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

/**
 * Draws a line from (x0,y0) to (x1,y1) with given color on the image.
 * Length of the color array is expected to be the number of channels.
 * No boundary checks are performed.
 */
template <typename T>
void
draw_line (Image<T>& image, int x1, int y1, int x2, int y2, T const* color);

template <typename T>
void
draw_circle (Image<T>& image, int x, int y, int radius, T const* color);

MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END

/* ------------------------- Implementation ----------------------- */

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

template <typename T>
void
draw_line (Image<T>& image, int x0, int y0, int x1, int y1, T const* color)
{
    /* http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm */
    int const chans = image.channels();
    int const row_stride = image.width() * chans;

    int const dx = std::abs(x1 - x0);
    int const dy = std::abs(y1 - y0) ;
    int const sx = x0 < x1 ? 1 : -1;
    int const sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    T* ptr = &image.at(x0, y0, 0);
    while (true)
    {
        std::copy(color, color + chans, ptr);
        if (x0 == x1 && y0 == y1)
            break;
        int const e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
            ptr += sx * chans;
        }
        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
            ptr += sy * row_stride;
        }
    }
}


template <typename T>
void
draw_circle (Image<T>& image, int x, int y, int radius, T const* color)
{
    /* http://en.wikipedia.org/wiki/Bresenham_circle */
    int const chans = image.channels();
    int const row_stride = image.width() * chans;
    T* const ptr = &image.at(x, y, 0);
    std::copy(color, color + chans, ptr - radius * row_stride);
    std::copy(color, color + chans, ptr - radius * chans);
    std::copy(color, color + chans, ptr + radius * chans);
    std::copy(color, color + chans, ptr + radius * row_stride);

    int f = 1 - radius;
    int ddf_x = 1;
    int ddf_y = -2 * radius;
    int xi = 0;
    int yi = radius;
    while (xi < yi)
    {
        if (f >= 0)
        {
            yi--;
            ddf_y += 2;
            f += ddf_y;
        }
        xi++;
        ddf_x += 2;
        f += ddf_x;

        std::copy(color, color + chans, ptr + xi * chans + yi * row_stride);
        std::copy(color, color + chans, ptr - xi * chans + yi * row_stride);
        std::copy(color, color + chans, ptr + xi * chans - yi * row_stride);
        std::copy(color, color + chans, ptr - xi * chans - yi * row_stride);
        std::copy(color, color + chans, ptr + xi * row_stride + yi * chans);
        std::copy(color, color + chans, ptr - xi * row_stride + yi * chans);
        std::copy(color, color + chans, ptr + xi * row_stride - yi * chans);
        std::copy(color, color + chans, ptr - xi * row_stride - yi * chans);
    }
}


MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_IMAGEDRAWING_HEADER */
