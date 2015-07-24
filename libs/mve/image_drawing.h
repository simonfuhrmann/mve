/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MVE_IMAGEDRAWING_HEADER
#define MVE_IMAGEDRAWING_HEADER

#include <algorithm>
#include <cmath>

#include "mve/defines.h"
#include "mve/image.h"

MVE_NAMESPACE_BEGIN
MVE_IMAGE_NAMESPACE_BEGIN

/**
 * Draws a line from (x0,y0) to (x1,y1) with given color on the image.
 * Length of the color array is expected to be the number of channels.
 * TODO: No boundary checks are performed yet.
 */
template <typename T>
void
draw_line (Image<T>& image, int x1, int y1, int x2, int y2, T const* color);

/**
 * Draws a circle with midpoint (x,y) and given 'radius' on the image.
 * Length of the color array is expected to be the number of channels.
 * TODO: No boundary checks are performed yet.
 */
template <typename T>
void
draw_circle (Image<T>& image, int x, int y, int radius, T const* color);

/**
 * Draws a rectangle from (x1,y1) to (x2,y2) on the image.
 * Length of the color array is expected to be the number of channels.
 */
template <typename T>
void
draw_rectangle (Image<T>& image, int x1, int y1, int x2, int y2,
    T const* color);

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

template <typename T>
void
draw_rectangle (Image<T>& image, int x1, int y1, int x2, int y2, T const* color)
{
    if (x1 > x2)
        std::swap(x1, x2);
    if (y1 > y2)
        std::swap(y1, y2);
    x1 = std::max(0, x1);
    x2 = std::max(0, x2);
    x1 = std::min(image.width() - 1, x1);
    x2 = std::min(image.width() - 1, x2);
    y1 = std::max(0, y1);
    y2 = std::max(0, y2);
    y1 = std::min(image.height() - 1, y1);
    y2 = std::min(image.height() - 1, y2);

    int const row_stride = image.width() * image.channels();
    T* ptr = image.begin();
    for (int y = y1; y <= y2; ++y)
        for (int x = x1; x <= x2; ++x)
            std::copy(color, color + image.channels(),
                ptr + x * image.channels() + y * row_stride);
}

MVE_IMAGE_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_IMAGEDRAWING_HEADER */
