// Test cases for the MVE image tools.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "mve/image.h"
#include "mve/imagefile.h"
#include "mve/imagetools.h"

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
