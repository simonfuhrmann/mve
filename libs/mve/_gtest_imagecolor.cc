// Test cases for the MVE image color conversions.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "mve/image.h"
#include "mve/imagecolor.h"

mve::FloatImage::Ptr
create_float_test_image (void)
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

mve::ByteImage::Ptr
create_byte_test_image (void)
{
    mve::ByteImage::Ptr img = mve::ByteImage::create(16, 16, 3);
    for (int i = 0; i < img->get_pixel_amount(); i += 3)
    {
        img->at(i, 0) = i;
        img->at(i, 1) = 255 - i;
        img->at(i, 2) = std::abs(255 - 2 * i);
    }
    return img;
}

TEST(ImageColorTest, RGBtoXYZ_Float_BackAndForth)
{
    using mve::image::color_convert;
    mve::FloatImage::Ptr img = create_float_test_image();
    mve::FloatImage::Ptr out = img->duplicate();
    color_convert<float>(out, mve::image::color_srgb_to_xyz<float>);
    color_convert<float>(out, mve::image::color_xyz_to_srgb<float>);
    for (int i = 0; i < out->get_value_amount(); ++i)
        EXPECT_NEAR(img->at(i), out->at(i), 1e-3f);
}

TEST(ImageColorTest, XYYtoXYZ_Float_BackAndForth)
{
    using mve::image::color_convert;
    mve::FloatImage::Ptr img = create_float_test_image();
    mve::FloatImage::Ptr out = img->duplicate();
    color_convert<float>(out, mve::image::color_xyy_to_xyz<float>);
    color_convert<float>(out, mve::image::color_xyz_to_xyy<float>);
    for (int i = 0; i < out->get_value_amount(); ++i)
        EXPECT_NEAR(img->at(i), out->at(i), 1e-3f);
}

TEST(ImageColorTest, RGBtoYCbCr_Float_BackAndForth)
{
    using mve::image::color_convert;
    mve::FloatImage::Ptr img = create_float_test_image();
    mve::FloatImage::Ptr out = img->duplicate();
    color_convert<float>(out, mve::image::color_rgb_to_ycbcr<float>);
    color_convert<float>(out, mve::image::color_ycbcr_to_rgb<float>);
    for (int i = 0; i < out->get_value_amount(); ++i)
        EXPECT_NEAR(img->at(i), out->at(i), 1e-3f);
}

TEST(ImageColorTest, RGBtoYCbCr_Byte_BackAndForth)
{
    using mve::image::color_convert;
    mve::ByteImage::Ptr img = create_byte_test_image();
    mve::ByteImage::Ptr out = img->duplicate();
    color_convert<uint8_t>(out, mve::image::color_rgb_to_ycbcr<uint8_t>);
    color_convert<uint8_t>(out, mve::image::color_ycbcr_to_rgb<uint8_t>);
    for (int i = 0; i < out->get_value_amount(); ++i)
        EXPECT_NEAR(img->at(i), out->at(i), 1);
}
