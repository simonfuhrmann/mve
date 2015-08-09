/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <stdexcept>

#include "mve/image_tools.h"
#include "mve/image_drawing.h"
#include "sfm/visualizer.h"

SFM_NAMESPACE_BEGIN

namespace
{
    unsigned char color_table[12][3] =
    {
        { 255, 0,   0 }, { 0, 255,   0 }, { 0, 0, 255   },
        { 255, 255, 0 }, { 255, 0, 255 }, { 0, 255, 255 },
        { 127, 255, 0 }, { 255, 127, 0 }, { 127, 0, 255 },
        { 255, 0, 127 }, { 0, 127, 255 }, { 0, 255, 127 }
    };

    void
    draw_box (mve::ByteImage& image, float x, float y,
        float size, float orientation, uint8_t const* color)
    {
        float const sin_ori = std::sin(orientation);
        float const cos_ori = std::cos(orientation);

        float const x0 = (cos_ori * -size - sin_ori * -size);
        float const y0 = (sin_ori * -size + cos_ori * -size);
        float const x1 = (cos_ori * +size - sin_ori * -size);
        float const y1 = (sin_ori * +size + cos_ori * -size);
        float const x2 = (cos_ori * +size - sin_ori * +size);
        float const y2 = (sin_ori * +size + cos_ori * +size);
        float const x3 = (cos_ori * -size - sin_ori * +size);
        float const y3 = (sin_ori * -size + cos_ori * +size);

        mve::image::draw_line(image, static_cast<int>(x + x0 + 0.5f),
            static_cast<int>(y + y0 + 0.5f), static_cast<int>(x + x1 + 0.5f),
            static_cast<int>(y + y1 + 0.5f), color);
        mve::image::draw_line(image, static_cast<int>(x + x1 + 0.5f),
            static_cast<int>(y + y1 + 0.5f), static_cast<int>(x + x2 + 0.5f),
            static_cast<int>(y + y2 + 0.5f), color);
        mve::image::draw_line(image, static_cast<int>(x + x2 + 0.5f),
            static_cast<int>(y + y2 + 0.5f), static_cast<int>(x + x3 + 0.5f),
            static_cast<int>(y + y3 + 0.5f), color);
        mve::image::draw_line(image, static_cast<int>(x + x3 + 0.5f),
            static_cast<int>(y + y3 + 0.5f), static_cast<int>(x + x0 + 0.5f),
            static_cast<int>(y + y0 + 0.5f), color);
    }

}  // namespace

/* ---------------------------------------------------------------- */

void
Visualizer::draw_keypoint (mve::ByteImage& image,
    Visualizer::Keypoint const& keypoint, Visualizer::KeypointStyle style,
    uint8_t const* color)
{
    int const x = static_cast<int>(keypoint.x + 0.5);
    int const y = static_cast<int>(keypoint.y + 0.5);
    int const width = image.width();
    int const height = image.height();
    int const channels = image.channels();

    if (x < 0 || x >= width || y < 0 || y >= height)
        return;

    int required_space = 0;
    bool draw_orientation = false;
    switch (style)
    {
        default:
        case SMALL_DOT_STATIC:
            required_space = 0;
            draw_orientation = false;
            break;
        case SMALL_CIRCLE_STATIC:
            required_space = 3;
            draw_orientation = false;
            break;
        case RADIUS_BOX_ORIENTATION:
            required_space = static_cast<int>(std::sqrt
                (2.0f * keypoint.radius * keypoint.radius)) + 1;
            draw_orientation = true;
            break;
        case RADIUS_CIRCLE_ORIENTATION:
            required_space = static_cast<int>(keypoint.radius);
            draw_orientation = true;
            break;
    }

    if (x < required_space || x >= width - required_space
        || y < required_space || y >= height - required_space)
    {
        style = SMALL_DOT_STATIC;
        required_space = 0;
        draw_orientation = false;
    }

    switch (style)
    {
        default:
        case SMALL_DOT_STATIC:
            std::copy(color, color + channels, &image.at(x, y, 0));
            break;
        case SMALL_CIRCLE_STATIC:
            mve::image::draw_circle(image, x, y, 3, color);
            break;
        case RADIUS_BOX_ORIENTATION:
            draw_box(image, keypoint.x, keypoint.y,
                keypoint.radius, keypoint.orientation, color);
            break;
        case RADIUS_CIRCLE_ORIENTATION:
            mve::image::draw_circle(image, x, y, required_space, color);
            break;
    }

    if (draw_orientation)
    {
        float const sin_ori = std::sin(keypoint.orientation);
        float const cos_ori = std::cos(keypoint.orientation);
        float const x1 = (cos_ori * keypoint.radius);
        float const y1 = (sin_ori * keypoint.radius);
        mve::image::draw_line(image, static_cast<int>(keypoint.x + 0.5f),
            static_cast<int>(keypoint.y + 0.5f),
            static_cast<int>(keypoint.x + x1 + 0.5f),
            static_cast<int>(keypoint.y + y1 + 0.5f), color);
    }
}

/* ---------------------------------------------------------------- */

mve::ByteImage::Ptr
Visualizer::draw_keypoints(mve::ByteImage::ConstPtr image,
    std::vector<Visualizer::Keypoint> const& matches,
    Visualizer::KeypointStyle style)
{
    mve::ByteImage::Ptr ret;
    if (image->channels() == 3)
    {
        ret = mve::image::desaturate<unsigned char>(image, mve::image::DESATURATE_AVERAGE);
        ret = mve::image::expand_grayscale<unsigned char>(ret);
    }
    else if (image->channels() == 1)
    {
        ret = mve::image::expand_grayscale<unsigned char>(image);
    }

    uint8_t* color = color_table[3];
    for (std::size_t i = 0; i < matches.size(); ++i)
    {
        Visualizer::draw_keypoint(*ret, matches[i], style, color);
    }

    return ret;
}

/* ---------------------------------------------------------------- */

mve::ByteImage::Ptr
Visualizer::draw_matches (mve::ByteImage::ConstPtr image1,
    mve::ByteImage::ConstPtr image2,
    Correspondences2D2D const& matches)
{
    if (image1->channels() != 3 || image2->channels() != 3)
        throw std::invalid_argument("Only 3-channel images allowed");

    int const img1_width = image1->width();
    int const img1_height = image1->height();
    int const img2_width = image2->width();
    int const img2_height = image2->height();
    int const out_width = img1_width + img2_width;
    int const out_height = std::max(img1_height, img2_height);

    mve::ByteImage::Ptr ret = mve::ByteImage::create(out_width, out_height, 3);
    ret->fill(0);

    /* Copy images into output image. */
    unsigned char* out_ptr = ret->begin();
    unsigned char const* img1_ptr = image1->begin();
    unsigned char const* img2_ptr = image2->begin();
    for (int y = 0; y < out_height; ++y)
    {
        if (y < img1_height)
        {
            std::copy(img1_ptr, img1_ptr + img1_width * 3, out_ptr);
            img1_ptr += img1_width * 3;
        }
        out_ptr += img1_width * 3;
        if (y < img2_height)
        {
            std::copy(img2_ptr, img2_ptr + img2_width * 3, out_ptr);
            img2_ptr += img2_width * 3;
        }
        out_ptr += img2_width * 3;
    }

    /* Draw matches. */
    for (std::size_t i = 0; i < matches.size(); ++i)
    {
        mve::image::draw_line(*ret, matches[i].p1[0], matches[i].p1[1],
            matches[i].p2[0] + img1_width, matches[i].p2[1],
            color_table[i % 12]);
    }

    return ret;
}

SFM_NAMESPACE_END
