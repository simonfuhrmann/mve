// Test cases for the MVE image color conversions.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "math/vector.h"
#include "mve/image.h"
#include "mve/image_color.h"

mve::FloatImage::Ptr
create_float_test_image (void)
{
    mve::FloatImage::Ptr img = mve::FloatImage::create(100, 100, 3);
    for (int i = 0; i < img->get_pixel_amount(); ++i)
    {
        img->at(i, 0) = i / static_cast<float>(100 * 100 - 1);
        img->at(i, 1) = 1.0f - img->at(i, 0);
        img->at(i, 2) = std::abs(1.0f - 2.0f * img->at(i, 0));
    }
    return img;
}

mve::ByteImage::Ptr
create_byte_test_image (void)
{
    mve::ByteImage::Ptr img = mve::ByteImage::create(16, 16, 3);
    for (int i = 0; i < img->get_pixel_amount(); ++i)
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
        EXPECT_NEAR(img->at(i), out->at(i), 1e-3f) << "At value " << i;
}

TEST(ImageColorTest, XYYtoXYZ_Float_BackAndForth)
{
    using mve::image::color_convert;
    mve::FloatImage::Ptr img = create_float_test_image();
    mve::FloatImage::Ptr out = img->duplicate();
    color_convert<float>(out, mve::image::color_xyy_to_xyz<float>);
    color_convert<float>(out, mve::image::color_xyz_to_xyy<float>);
    for (int i = 0; i < out->get_pixel_amount(); ++i)
    {
        math::Vec3f p1(&img->at(i, 0));
        math::Vec3f p2(&out->at(i, 0));
        if (p1[1] == 0.0f)  // Special handling for special color.
            EXPECT_EQ(p2, math::Vec3f(0.0f, 0.0f, 0.0f));
        else
            EXPECT_TRUE(p1.is_similar(p2, 1e-6f)) << "At pixel " << i;
    }
}

TEST(ImageColorTest, RGBtoYCbCr_Float_BackAndForth)
{
    using mve::image::color_convert;
    mve::FloatImage::Ptr img = create_float_test_image();
    mve::FloatImage::Ptr out = img->duplicate();
    color_convert<float>(out, mve::image::color_rgb_to_ycbcr<float>);
    color_convert<float>(out, mve::image::color_ycbcr_to_rgb<float>);
    for (int i = 0; i < out->get_value_amount(); ++i)
        EXPECT_NEAR(img->at(i), out->at(i), 1e-5f) << "At value " << i;
}

TEST(ImageColorTest, RGBtoYCbCr_Byte_BackAndForth)
{
    using mve::image::color_convert;
    mve::ByteImage::Ptr img = create_byte_test_image();
    mve::ByteImage::Ptr out = img->duplicate();
    color_convert<uint8_t>(out, mve::image::color_rgb_to_ycbcr<uint8_t>);
    color_convert<uint8_t>(out, mve::image::color_ycbcr_to_rgb<uint8_t>);
    for (int i = 0; i < out->get_value_amount(); ++i)
        EXPECT_NEAR(img->at(i), out->at(i), 1) << "At value " << i;
}

TEST(ImageColorTest, RGBtoXYZ_Byte_BackAndForth)
{
    using mve::image::color_convert;
    mve::ByteImage::ConstPtr img = create_byte_test_image();
    mve::ByteImage::Ptr out = img->duplicate();
    color_convert<uint8_t>(out, mve::image::color_srgb_to_xyz<uint8_t>);
    mve::ByteImage::Ptr xyz = out->duplicate();
    color_convert<uint8_t>(out, mve::image::color_xyz_to_srgb<uint8_t>);
    for (int i = 0; i < out->get_pixel_amount(); ++i)
    {
        // Ignore overflowed pixels in z channel.
        if (xyz->at(i, 2) == 255)
            continue;
        math::Vec3i pimg(&img->at(i, 0));
        math::Vec3i pout(&out->at(i, 0));
        EXPECT_TRUE(pimg.is_similar(pout, 2))
            << pimg << " vs " << pout << " for px " << i;
    }
}

TEST(ImageColorTest, XYYtoXYZ_Byte_BackAndForth)
{
    using mve::image::color_convert;
    mve::ByteImage::ConstPtr img = create_byte_test_image();
    mve::ByteImage::Ptr out = img->duplicate();
    color_convert<uint8_t>(out, mve::image::color_xyy_to_xyz<uint8_t>);
    mve::ByteImage::Ptr xyz = out->duplicate();
    color_convert<uint8_t>(out, mve::image::color_xyz_to_xyy<uint8_t>);
    for (int i = 0; i < out->get_pixel_amount(); ++i)
    {
        math::Vec3i pimg(&img->at(i, 0));
        math::Vec3i pout(&out->at(i, 0));
        // Ignore overflowed pixels in x channel.
        if (xyz->at(i, 0) == 255)
            continue;
        // Special handling for special color.
        if (img->at(i, 1) == 0)
            EXPECT_EQ(pout, math::Vec3i(0, 0, 0));
        else
            EXPECT_TRUE(pimg.is_similar(pout, 5))
                << pimg << " vs " << pout << " for px " << i;
    }
}
