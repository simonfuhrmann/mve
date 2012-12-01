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
