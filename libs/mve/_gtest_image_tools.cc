// Test cases for the MVE image tools.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "mve/image.h"
#include "mve/image_tools.h"

mve::FloatImage::Ptr
create_test_float_image (int width, int height, int chans)
{
    mve::FloatImage::Ptr img = mve::FloatImage::create(width, height, chans);
    for (int i = 0; i < img->get_pixel_amount(); i += chans)
    {
        img->at(i, 0) = i / static_cast<float>(width * height * chans);
        img->at(i, 1) = 1.0f - img->at(i, 0);
        img->at(i, 2) = std::abs(1.0f - 2.0f * img->at(i, 0));
    }
    return img;
}

TEST(ImageToolsTest, ImageConversion)
{
    mve::FloatImage::Ptr img = mve::FloatImage::create(2, 2, 1);
    img->at(0) = 0.0f;
    img->at(1) = 1.0f;
    img->at(2) = 0.5f;
    img->at(3) = 2.0f;

    mve::ByteImage::Ptr img2 = mve::image::float_to_byte_image(img);
    ASSERT_EQ(0, img2->at(0));
    ASSERT_EQ(255, img2->at(1));
    ASSERT_EQ(128, img2->at(2));
    ASSERT_EQ(255, img2->at(3));

    // TODO
}

TEST(ImageToolsTest, ImageFindMinMax)
{
    mve::FloatImage::Ptr fimg = mve::FloatImage::create(4, 1, 1);
    fimg->at(0) = -1.0f;
    fimg->at(1) = 4.0f;
    fimg->at(2) = -2.0f;
    fimg->at(3) = 0.0f;
    float vmin, vmax;
    mve::image::find_min_max_value(fimg, &vmin, &vmax);
    EXPECT_EQ(-2.0f, vmin);
    EXPECT_EQ(4.0f, vmax);

    mve::ByteImage::Ptr bimg = mve::ByteImage::create(4, 1, 1);
    bimg->at(0) = 10;
    bimg->at(1) = 5;
    bimg->at(2) = 100;
    bimg->at(3) = 120;
    unsigned char bmin, bmax;
    mve::image::find_min_max_value(bimg, &bmin, &bmax);
    EXPECT_EQ(5, bmin);
    EXPECT_EQ(120, bmax);
}

TEST(ImageToolsTest, FloatImageNormalize)
{
    mve::FloatImage::Ptr fimg = mve::FloatImage::create(4, 1, 1);
    fimg->at(0) = 0.0f;
    fimg->at(1) = 1.0f;
    fimg->at(2) = 2.0f;
    fimg->at(3) = 2.0f;
    mve::image::float_image_normalize(fimg);
    EXPECT_EQ(0.0f, fimg->at(0));
    EXPECT_EQ(0.5f, fimg->at(1));
    EXPECT_EQ(1.0f, fimg->at(2));
    EXPECT_EQ(1.0f, fimg->at(3));

    fimg->at(0) = 1.0f;
    fimg->at(1) = 1.0f;
    fimg->at(2) = 1.0f;
    fimg->at(3) = 1.0f;
    mve::image::float_image_normalize(fimg);
    EXPECT_EQ(0.0f, fimg->at(0));
    EXPECT_EQ(0.0f, fimg->at(1));
    EXPECT_EQ(0.0f, fimg->at(2));
    EXPECT_EQ(0.0f, fimg->at(3));

    fimg->at(0) = -2.0f;
    fimg->at(1) = -2.0f;
    fimg->at(2) = -1.5f;
    fimg->at(3) = -1.0f;
    mve::image::float_image_normalize(fimg);
    EXPECT_EQ(0.0f, fimg->at(0));
    EXPECT_EQ(0.0f, fimg->at(1));
    EXPECT_EQ(0.5f, fimg->at(2));
    EXPECT_EQ(1.0f, fimg->at(3));
}

TEST(ImageToolsTest, RescaleImageSameSize)
{
    mve::FloatImage::Ptr img = mve::FloatImage::create(4, 4, 2);
    for (int i = 0; i < img->get_value_amount(); ++i)
        img->at(i) = static_cast<float>(i);

    mve::FloatImage::Ptr out = mve::image::rescale<float>(img,
        mve::image::RESCALE_GAUSSIAN, img->width(), img->height());

    EXPECT_EQ(out->width(), img->width());
    EXPECT_EQ(out->height(), img->height());
    EXPECT_EQ(out->channels(), img->channels());
    for (int i = 0; i < img->get_value_amount(); ++i)
        EXPECT_EQ(img->at(i), out->at(i)) << "at index " << i;
}

TEST(ImageToolsTest, ImageRotateAngle)
{
    unsigned char const color_black_1[] = { 0 };

    mve::ByteImage::Ptr i1 = mve::ByteImage::create(1, 1, 1);
    i1->fill(127);
    i1 = mve::image::rotate(i1, MATH_PI / 4.0f, color_black_1);
    EXPECT_EQ(127, i1->at(0));

    mve::ByteImage::Ptr i2 = mve::ByteImage::create(2, 2, 1);
    i2->fill(127);
    i2 = mve::image::rotate(i2, MATH_PI / 4.0f, color_black_1);
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(127, i2->at(i));

    mve::ByteImage::Ptr i3 = mve::ByteImage::create(3, 3, 1);
    i3->fill(127);
    i3 = mve::image::rotate(i3, MATH_PI / 4.0f, color_black_1);
    for (int i = 0; i < 9; ++i)
        EXPECT_EQ(127, i3->at(i));

    mve::ByteImage::Ptr i4 = mve::ByteImage::create(4, 4, 1);
    i4->fill(127);
    i4 = mve::image::rotate(i4, MATH_PI / 4.0f, color_black_1);
    for (int i = 0; i < 16; ++i)
        if (i == 0 || i == 3 || i == 12 || i == 15)
            EXPECT_EQ(color_black_1[0], i4->at(i));
        else
            EXPECT_EQ(127, i4->at(i));
}

TEST(ImageToolsTest, ImageCropInside)
{
    mve::ByteImage::Ptr img = mve::ByteImage::create(4, 4, 2);
    for (int i = 0; i < img->get_value_amount(); ++i)
        img->at(i) = i;
    img = mve::image::crop<uint8_t>(img, 2, 2, 1, 1, NULL);
    EXPECT_EQ(2, img->width());
    EXPECT_EQ(2, img->height());
    EXPECT_EQ(2, img->channels());
    EXPECT_EQ(10, img->at(0));
    EXPECT_EQ(11, img->at(1));
    EXPECT_EQ(12, img->at(2));
    EXPECT_EQ(13, img->at(3));
    EXPECT_EQ(18, img->at(4));
    EXPECT_EQ(19, img->at(5));
    EXPECT_EQ(20, img->at(6));
    EXPECT_EQ(21, img->at(7));
}

TEST(ImageToolsTest, ImageCropOutside1)
{
    mve::ByteImage::Ptr img = mve::ByteImage::create(4, 4, 2);
    for (int i = 0; i < img->get_value_amount(); ++i)
        img->at(i) = i;
    uint8_t color[] = { 63, 127 };
    img = mve::image::crop<uint8_t>(img, 2, 2, -2, -2, color);
    for (int i = 0; i < img->get_value_amount(); ++i)
        if (i % 2 == 0)
            EXPECT_EQ(63, img->at(i));
        else
            EXPECT_EQ(127, img->at(i));
}

TEST(ImageToolsTest, ImageCropOutside2)
{
    mve::ByteImage::Ptr img = mve::ByteImage::create(4, 4, 2);
    for (int i = 0; i < img->get_value_amount(); ++i)
        img->at(i) = i;
    uint8_t color[] = { 63, 127 };
    img = mve::image::crop<uint8_t>(img, 2, 2, 4, 4, color);
    for (int i = 0; i < img->get_value_amount(); ++i)
        if (i % 2 == 0)
            EXPECT_EQ(63, img->at(i));
        else
            EXPECT_EQ(127, img->at(i));
}

TEST(ImageToolsTest, CropImagePortrait)
{
    mve::ByteImage::Ptr img = mve::ByteImage::create(1, 2, 1);
    unsigned char white = 255;
    unsigned char black = 0;
    img->at(0, 0, 0) = black;
    img->at(0, 1, 0) = white;
    mve::ByteImage::Ptr cropped = mve::image::crop(img, 1, 1, 0, 1, &black);
    EXPECT_EQ(white, cropped->at(0));
}

TEST(ImageToolsTest, CropImageOverlap1)
{
    mve::ByteImage::Ptr img = mve::ByteImage::create(4, 4, 2);
    for (int i = 0; i < img->get_value_amount(); ++i)
        img->at(i) = i;
    uint8_t color[] = { 63, 127 };
    img = mve::image::crop<uint8_t>(img, 2, 2, -1, -1, color);
    EXPECT_EQ(2, img->width());
    EXPECT_EQ(2, img->height());
    EXPECT_EQ(2, img->channels());
    EXPECT_EQ(63, img->at(0));
    EXPECT_EQ(127, img->at(1));
    EXPECT_EQ(63, img->at(2));
    EXPECT_EQ(127, img->at(3));
    EXPECT_EQ(63, img->at(4));
    EXPECT_EQ(127, img->at(5));
    EXPECT_EQ(0, img->at(6));
    EXPECT_EQ(1, img->at(7));
}

TEST(ImageToolsTest, CropImageOverlap2)
{
    mve::ByteImage::Ptr img = mve::ByteImage::create(4, 4, 2);
    for (int i = 0; i < img->get_value_amount(); ++i)
        img->at(i) = i;
    uint8_t color[] = { 63, 127 };
    img = mve::image::crop<uint8_t>(img, 2, 2, 3, 3, color);
    EXPECT_EQ(2, img->width());
    EXPECT_EQ(2, img->height());
    EXPECT_EQ(2, img->channels());
    EXPECT_EQ(30, img->at(0));
    EXPECT_EQ(31, img->at(1));
    EXPECT_EQ(63, img->at(2));
    EXPECT_EQ(127, img->at(3));
    EXPECT_EQ(63, img->at(4));
    EXPECT_EQ(127, img->at(5));
    EXPECT_EQ(63, img->at(6));
    EXPECT_EQ(127, img->at(7));
}

TEST(ImageToolsTest, ImageHalfSizeEvenSize)
{
    mve::FloatImage::Ptr img = mve::FloatImage::create(4, 2, 2);
    for (int i = 0; i < img->get_value_amount(); ++i)
        img->at(i) = i;

    mve::FloatImage::Ptr out = mve::image::rescale_half_size<float>(img);
    EXPECT_EQ(2, out->width());
    EXPECT_EQ(1, out->height());
    EXPECT_EQ(2, out->channels());
    EXPECT_EQ((0.0f + 2.0f + 8.0f + 10.0f) / 4.0f, out->at(0, 0, 0));
    EXPECT_EQ((1.0f + 3.0f + 9.0f + 11.0f) / 4.0f, out->at(0, 0, 1));
    EXPECT_EQ((4.0f + 6.0f + 12.0f + 14.0f) / 4.0f, out->at(1, 0, 0));
    EXPECT_EQ((5.0f + 7.0f + 13.0f + 15.0f) / 4.0f, out->at(1, 0, 1));
}

TEST(ImageToolsTest, ImageHalfSizeOddSize)
{
    mve::FloatImage::Ptr img = mve::FloatImage::create(3, 2, 2);
    for (int i = 0; i < img->get_value_amount(); ++i)
        img->at(i) = i;

    mve::FloatImage::Ptr out = mve::image::rescale_half_size<float>(img);
    EXPECT_EQ(2, out->width());
    EXPECT_EQ(1, out->height());
    EXPECT_EQ(2, out->channels());
    EXPECT_EQ((0.0f + 2.0f + 6.0f + 8.0f) / 4.0f, out->at(0, 0, 0));
    EXPECT_EQ((1.0f + 3.0f + 7.0f + 9.0f) / 4.0f, out->at(0, 0, 1));
    EXPECT_EQ((4.0f + 10.0f) / 2.0f, out->at(1, 0, 0));
    EXPECT_EQ((5.0f + 11.0f) / 2.0f, out->at(1, 0, 1));
}

TEST(ImageToolsTest, IntegralImage)
{
    mve::ByteImage::Ptr img = mve::ByteImage::create(4, 4, 2);
    for (int i = 0; i < 4 * 4 * 2; ++i)
        img->at(i) = i;

    mve::IntImage::Ptr sat = mve::image::integral_image<uint8_t, int>(img);
    EXPECT_EQ(sat->width(), img->width());
    EXPECT_EQ(sat->height(), img->height());
    EXPECT_EQ(sat->channels(), img->channels());

    EXPECT_EQ(0, sat->at(0, 0, 0));  // channel 0, row 0
    EXPECT_EQ(2, sat->at(1, 0, 0));
    EXPECT_EQ(6, sat->at(2, 0, 0));
    EXPECT_EQ(12, sat->at(3, 0, 0));
    EXPECT_EQ(8, sat->at(0, 1, 0));  // channel 0, row 1
    EXPECT_EQ(20, sat->at(1, 1, 0));
    EXPECT_EQ(36, sat->at(2, 1, 0));
    EXPECT_EQ(56, sat->at(3, 1, 0));
    EXPECT_EQ(24, sat->at(0, 2, 0));  // channel 0, row 2
    EXPECT_EQ(54, sat->at(1, 2, 0));
    EXPECT_EQ(90, sat->at(2, 2, 0));
    EXPECT_EQ(132, sat->at(3, 2, 0));
    EXPECT_EQ(48, sat->at(0, 3, 0));  // channel 0, row 3
    EXPECT_EQ(104, sat->at(1, 3, 0));
    EXPECT_EQ(168, sat->at(2, 3, 0));
    EXPECT_EQ(240, sat->at(3, 3, 0));

    EXPECT_EQ(1, sat->at(0, 0, 1));  // channel 1, row 0
    EXPECT_EQ(4, sat->at(1, 0, 1));
    EXPECT_EQ(9, sat->at(2, 0, 1));
    EXPECT_EQ(16, sat->at(3, 0, 1));
    EXPECT_EQ(10, sat->at(0, 1, 1));  // channel 1, row 1
    EXPECT_EQ(24, sat->at(1, 1, 1));
    EXPECT_EQ(42, sat->at(2, 1, 1));
    EXPECT_EQ(64, sat->at(3, 1, 1));
    EXPECT_EQ(27, sat->at(0, 2, 1));  // channel 1, row 2
    EXPECT_EQ(60, sat->at(1, 2, 1));
    EXPECT_EQ(99, sat->at(2, 2, 1));
    EXPECT_EQ(144, sat->at(3, 2, 1));
    EXPECT_EQ(52, sat->at(0, 3, 1));  // channel 1, row 3
    EXPECT_EQ(112, sat->at(1, 3, 1));
    EXPECT_EQ(180, sat->at(2, 3, 1));
    EXPECT_EQ(256, sat->at(3, 3, 1));
}

TEST(ImageToolsTest, GammaCorrect_Float_GoldenValues)
{
    mve::FloatImage::Ptr img = mve::FloatImage::create(1, 1, 3);
    img->at(0, 0, 0) = 1.0f;
    img->at(0, 0, 1) = 4.4f;
    img->at(0, 0, 2) = 0.3f;

    {
        mve::FloatImage::Ptr out = img->duplicate();
        mve::image::gamma_correct<float>(out, 1.0f / 2.2f);
        EXPECT_NEAR(out->at(0, 0, 0), std::pow(1.0, 1.0 / 2.2), 1e-10f);
        EXPECT_NEAR(out->at(0, 0, 1), std::pow(4.4, 1.0 / 2.2), 1e-7f);
        EXPECT_NEAR(out->at(0, 0, 2), std::pow(0.3, 1.0 / 2.2), 1e-7f);
    }

    {
        mve::FloatImage::Ptr out = img->duplicate();
        mve::image::gamma_correct<float>(out, 2.2f);
        EXPECT_NEAR(out->at(0, 0, 0), std::pow(1.0, 2.2), 1e-10f);
        EXPECT_NEAR(out->at(0, 0, 1), std::pow(4.4, 2.2), 1e-5f);
        EXPECT_NEAR(out->at(0, 0, 2), std::pow(0.3, 2.2), 1e-7f);
    }
}

TEST(ImageToolsTest, GammaCorrect_Float_BackAndForth)
{
    mve::FloatImage::Ptr img = create_test_float_image(100, 100, 3);
    mve::FloatImage::Ptr out = img->duplicate();

    mve::image::gamma_correct<float>(out, 1.0/2.2);
    mve::image::gamma_correct<float>(out, 2.2);

    for (int i = 0; i < out->get_value_amount(); ++i)
        EXPECT_NEAR(img->at(i), out->at(i), 1e-6f);
}

TEST(ImageToolsTest, GammaCorrectSRGB_Float_BackAndForth)
{
    mve::FloatImage::Ptr img = create_test_float_image(100, 100, 3);
    mve::FloatImage::Ptr out = img->duplicate();

    mve::image::gamma_correct_srgb<float>(out);
    mve::image::gamma_correct_inv_srgb<float>(out);

    for (int i = 0; i < out->get_value_amount(); ++i)
        EXPECT_NEAR(img->at(i), out->at(i), 1e-6f);
}
