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
