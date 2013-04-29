// Test cases for the MVE image color conversions.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "mve/image.h"
#include "mve/imagecolor.h"

mve::FloatImage::Ptr
create_test_image (void)
{
    mve::FloatImage::Ptr img = mve::FloatImage::create(100, 100, 3);
    for (int i = 0; i < img->get_pixel_amount(); i += 3)
    {
        img->at(i, 0) = i / static_cast<float>(100 * 100 * 3);
        img->at(i, 1) = 1.0f - img->at(i, 0);
        img->at(i, 2) = std::abs(1.0f - 2.0f * img->at(i, 0));
    }
    return img;
}

TEST(ImageColorTest, RGBtoXYZ_BackAndForth)
{
    using mve::image::color_convert;
    mve::FloatImage::Ptr img = create_test_image();
    mve::FloatImage::Ptr out = img->duplicate();
    color_convert<float>(out, mve::image::color_srgb_to_xyz<float>);
    color_convert<float>(out, mve::image::color_xyz_to_srgb<float>);
    for (int i = 0; i < out->get_value_amount(); ++i)
        EXPECT_NEAR(img->at(i), out->at(i), 1e-3f);
}

TEST(ImageColorTest, XYYtoXYZ_BackAndForth)
{
    using mve::image::color_convert;
    mve::FloatImage::Ptr img = create_test_image();
    mve::FloatImage::Ptr out = img->duplicate();
    color_convert<float>(out, mve::image::color_xyy_to_xyz<float>);
    color_convert<float>(out, mve::image::color_xyz_to_xyy<float>);
    for (int i = 0; i < out->get_value_amount(); ++i)
        EXPECT_NEAR(img->at(i), out->at(i), 1e-3f);
}

TEST(ImageColorTest, RGBtoYCbCr_BackAndForth)
{
    using mve::image::color_convert;
    mve::FloatImage::Ptr img = create_test_image();
    mve::FloatImage::Ptr out = img->duplicate();
    color_convert<float>(out, mve::image::color_rgb_to_ycbcr<float>);
    color_convert<float>(out, mve::image::color_ycbcr_to_rgb<float>);
    for (int i = 0; i < out->get_value_amount(); ++i)
        EXPECT_NEAR(img->at(i), out->at(i), 1e-3f);
}
