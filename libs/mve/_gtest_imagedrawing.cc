// Test cases for the image drawing features.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "mve/image.h"
#include "mve/imagedrawing.h"

unsigned char color_g_white[1] = { 255 };
unsigned char color_rgb_red[3] = { 255, 0, 0 };

TEST(ImageDrawingTest, DrawLineSimpleTests)
{
    mve::ByteImage img(3, 3, 1);
    // Draw a dot in the center of the image.
    img.fill(0);
    mve::image::draw_line(img, 1, 1, 1, 1, color_g_white);
    for (int i = 0; i < 3 * 3; ++i)
    {
        if (i == 4)
            EXPECT_EQ(255, img.at(i));
        else
            EXPECT_EQ(0, img.at(i));
    }

    // Draw a horizontal line.
    img.fill(0);
    mve::image::draw_line(img, 0, 1, 2, 1, color_g_white);
    for (int i = 0; i < 3 * 3; ++i)
    {
        if (i >= 3 && i <= 5)
            EXPECT_EQ(255, img.at(i));
        else
            EXPECT_EQ(0, img.at(i));
    }

}

TEST(ImageDrawingTest, DrawLineRGB)
{
    mve::ByteImage img(5, 5, 3);
    img.fill(0);
    mve::image::draw_line(img, 0, 0, 4, 4, color_rgb_red);
    for (int i = 0; i < 5 * 5 * 3; ++i)
    {
        if (i % 3 == 0 && (i/3) / 5 == (i/3) % 5)
            EXPECT_EQ(255, img.at(i));
        else
            EXPECT_EQ(0, img.at(i));
    }

    img.fill(0);
    mve::image::draw_line(img, 4, 4, 0, 0, color_rgb_red);
    for (int i = 0; i < 5 * 5 * 3; ++i)
    {
        if (i % 3 == 0 && (i/3) / 5 == (i/3) % 5)
            EXPECT_EQ(255, img.at(i));
        else
            EXPECT_EQ(0, img.at(i));
    }
}

// TODO: Write test with "golden patterns" from reference implementations.
