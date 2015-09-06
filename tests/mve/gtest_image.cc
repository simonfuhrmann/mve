// Test cases for the image class and related features.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "mve/image.h"
#include "mve/image_io.h"
#include "mve/image_tools.h"

TEST(ImageTest, ImageBaseInitialization)
{
    mve::ImageBase img;
    EXPECT_EQ(0, img.width());
    EXPECT_EQ(0, img.height());
    EXPECT_EQ(0, img.channels());
    EXPECT_FALSE(img.valid());

    EXPECT_FALSE(img.reinterpret(1, 1, 1));
    EXPECT_TRUE(img.reinterpret(0, 1, 1));
    EXPECT_TRUE(img.reinterpret(0, 0, 1));
    EXPECT_TRUE(img.reinterpret(0, 0, 0));

    EXPECT_EQ(0, img.get_byte_size());
    EXPECT_EQ(0, img.get_byte_pointer());
    EXPECT_EQ(mve::IMAGE_TYPE_UNKNOWN, img.get_type());
    EXPECT_STREQ("unknown", img.get_type_string());
}

TEST(ImageTest, ImageTypeStrings)
{
    {
        mve::TypedImageBase<unsigned char> img;
        EXPECT_EQ(mve::IMAGE_TYPE_UINT8, img.get_type());
        EXPECT_STREQ("uint8", img.get_type_string());
    }
    {
        mve::TypedImageBase<int> img;
        EXPECT_EQ(mve::IMAGE_TYPE_SINT32, img.get_type());
        EXPECT_STREQ("sint32", img.get_type_string());
    }
    {
        mve::TypedImageBase<float> img;
        EXPECT_EQ(mve::IMAGE_TYPE_FLOAT, img.get_type());
        EXPECT_STREQ("float", img.get_type_string());
    }
}

TEST(ImageTest, TypedImageBaseInitialization)
{
    mve::TypedImageBase<int> img;
    EXPECT_EQ(0, img.width());
    EXPECT_EQ(0, img.height());
    EXPECT_EQ(0, img.channels());
    EXPECT_FALSE(img.valid());

    img.allocate(1, 1, 0);
    EXPECT_EQ(1, img.width());
    EXPECT_EQ(1, img.height());
    EXPECT_EQ(0, img.channels());
    EXPECT_FALSE(img.valid());

    EXPECT_TRUE(img.reinterpret(0, 1, 0));

    img.clear();
    EXPECT_EQ(0, img.width());
    EXPECT_EQ(0, img.height());
    EXPECT_EQ(0, img.channels());
    EXPECT_FALSE(img.valid());

    // Funfact: &data[0] returns null if vector data is empty.
    EXPECT_EQ(0, img.get_data_pointer());
    EXPECT_EQ(0, img.get_byte_pointer());
    EXPECT_EQ(0, img.begin());
    EXPECT_EQ(0, img.end());

    EXPECT_EQ(0, img.get_pixel_amount());
    EXPECT_EQ(0, img.get_value_amount());
    EXPECT_EQ(0, img.get_byte_size());
}

TEST(ImageTest, AllocateResizeReinterpret)
{
    mve::TypedImageBase<int> img;
    EXPECT_FALSE(img.valid());
    img.allocate(4, 2, 1);
    EXPECT_TRUE(img.valid());

    EXPECT_EQ(4, img.width());
    EXPECT_EQ(2, img.height());
    EXPECT_EQ(1, img.channels());
    EXPECT_EQ(8, img.get_pixel_amount());
    EXPECT_EQ(8, img.get_value_amount());

    img.allocate(1, 2, 4);
    EXPECT_EQ(1, img.width());
    EXPECT_EQ(2, img.height());
    EXPECT_EQ(4, img.channels());
    EXPECT_EQ(2, img.get_pixel_amount());
    EXPECT_EQ(8, img.get_value_amount());

    img.resize(2, 2, 2);
    EXPECT_EQ(2, img.width());
    EXPECT_EQ(2, img.height());
    EXPECT_EQ(2, img.channels());
    EXPECT_EQ(4, img.get_pixel_amount());
    EXPECT_EQ(8, img.get_value_amount());

    EXPECT_FALSE(img.reinterpret(0, 1, 8));
    EXPECT_TRUE(img.reinterpret(8, 1, 1));
    EXPECT_TRUE(img.reinterpret(4, 2, 1));
    EXPECT_TRUE(img.reinterpret(4, 1, 2));
    EXPECT_TRUE(img.reinterpret(1, 1, 8));
    EXPECT_EQ(1, img.width());
    EXPECT_EQ(1, img.height());
    EXPECT_EQ(8, img.channels());
}

TEST(ImageTest, ImageDataFill)
{
    mve::TypedImageBase<int> img;
    img.allocate(2, 2, 2);

    // Test iterator run length.
    {
        int counter = 0;
        for (int const* i = img.begin(); i != img.end(); ++i)
            counter++;
        EXPECT_EQ(8, counter);
    }

    // Test fill and iterator.
    img.fill(23);
    for (int const* i = img.begin(); i != img.end(); ++i)
        EXPECT_EQ(23, *i);
}

TEST(ImageTest, ImageAccess)
{
    // TODO More tests??
    mve::IntImage img;
    img.allocate(2, 2, 2);
    img.fill(0);
    img.at(2) = 23; // (1,0,0);
    img.at(5) = 33; // (0,1,1);

    EXPECT_EQ(23, img.at(1, 0));  // TODO: Not a huge fan of this accessor! Delete?
    EXPECT_EQ(23, img.at(1, 0, 0));
    EXPECT_EQ(33, img.at(2, 1));
    EXPECT_EQ(33, img.at(0, 1, 1));
}

TEST(ImageTest, ImageAddChannels)
{
    mve::IntImage img;
    img.allocate(2, 2, 1);

    // Simplest case.
    img.allocate(1, 1, 1);
    img.at(0) = 23;
    img.add_channels(1, 13);
    EXPECT_EQ(23, img.at(0));
    EXPECT_EQ(13, img.at(1));

    // Two pixel, actual move required.
    img.allocate(2, 1, 1);
    img.at(0) = 23;
    img.at(1) = 33;
    img.add_channels(1, 43);
    EXPECT_EQ(23, img.at(0));
    EXPECT_EQ(43, img.at(1));
    EXPECT_EQ(33, img.at(2));
    EXPECT_EQ(43, img.at(3));

    // Move with two new channels.
    img.allocate(2, 1, 1);
    img.at(0) = 23;
    img.at(1) = 33;
    img.add_channels(2, 43);
    EXPECT_EQ(23, img.at(0));
    EXPECT_EQ(43, img.at(1));
    EXPECT_EQ(43, img.at(2));
    EXPECT_EQ(33, img.at(3));
    EXPECT_EQ(43, img.at(4));
    EXPECT_EQ(43, img.at(5));
}

TEST(ImageTest, ImageCopyChannel)
{
    /* One pixel, simple test. */
    mve::IntImage img;
    img.allocate(1, 1, 2);
    img.at(0) = 23;
    img.at(1) = 33;
    img.copy_channel(0, 1);
    EXPECT_EQ(2, img.channels());
    EXPECT_EQ(23, img.at(0));
    EXPECT_EQ(23, img.at(1));

    /* One pixel, copy and add channel. */
    img.allocate(1, 1, 2);
    img.at(0) = 23;
    img.at(1) = 33;
    img.copy_channel(0, -1);
    EXPECT_EQ(3, img.channels());
    EXPECT_EQ(23, img.at(0));
    EXPECT_EQ(33, img.at(1));
    EXPECT_EQ(23, img.at(2));


    /* Test with two pixel */
    img.allocate(2, 1, 2);
    img.at(0) = 23;
    img.at(1) = 24;
    img.at(2) = 25;
    img.at(3) = 26;
    img.copy_channel(0, 1);
    EXPECT_EQ(23, img.at(0));
    EXPECT_EQ(23, img.at(1));
    EXPECT_EQ(25, img.at(2));
    EXPECT_EQ(25, img.at(3));
}

TEST(ImageTest, ImageSwapChannels)
{
    /* One pixel, simple test. */
    mve::IntImage img;
    img.allocate(1, 1, 2);
    img.at(0) = 23;
    img.at(1) = 33;
    img.swap_channels(0, 1);
    EXPECT_EQ(33, img.at(0));
    EXPECT_EQ(23, img.at(1));

    /* Test with two pixel */
    img.allocate(2, 1, 2);
    img.at(0) = 23;
    img.at(1) = 24;
    img.at(2) = 25;
    img.at(3) = 26;
    img.swap_channels(0, 1);
    EXPECT_EQ(24, img.at(0));
    EXPECT_EQ(23, img.at(1));
    EXPECT_EQ(26, img.at(2));
    EXPECT_EQ(25, img.at(3));
}

TEST(ImageTest, ImgaeDeleteChannel)
{
    /* One pixel. */
    mve::IntImage img;
    img.allocate(1, 1, 2);
    img.at(0) = 23;
    img.at(1) = 33;
    img.delete_channel(1);
    EXPECT_EQ(1, img.width());
    EXPECT_EQ(1, img.height());
    EXPECT_EQ(1, img.channels());
    EXPECT_EQ(23, img.at(0));

    img.allocate(1, 1, 2);
    img.at(0) = 23;
    img.at(1) = 33;
    img.delete_channel(0);
    EXPECT_EQ(1, img.width());
    EXPECT_EQ(1, img.height());
    EXPECT_EQ(1, img.channels());
    EXPECT_EQ(33, img.at(0));

    /* Two pixel, move required. */
    img.allocate(1, 2, 2);
    img.at(0) = 23;
    img.at(1) = 24;
    img.at(2) = 25;
    img.at(3) = 26;

    img.delete_channel(1);
    EXPECT_EQ(1, img.width());
    EXPECT_EQ(2, img.height());
    EXPECT_EQ(1, img.channels());
    EXPECT_EQ(23, img.at(0));
    EXPECT_EQ(25, img.at(1));
}

TEST(ImageTest, ImageLinearAccess)
{
    mve::FloatImage img;
    img.resize(2, 2, 2);
    for (int i = 0; i < 8; ++i)
        img.at(i) = static_cast<float>(i);

    // Linear access with single channel. Remove this on?
    EXPECT_EQ(0.0f, img.linear_at(0.0f, 0.0f, 0));
    EXPECT_EQ(1.0f, img.linear_at(0.0f, 0.0f, 1));
    EXPECT_EQ(2.0f, img.linear_at(1.0f, 0.0f, 0));
    EXPECT_EQ(3.0f, img.linear_at(0.5f, 0.5, 0));

    float px[2];
    img.linear_at(0.0f, 1.0f, px);
    EXPECT_EQ(4.0f, px[0]);
    EXPECT_EQ(5.0f, px[1]);
    img.linear_at(0.25f, 0.25f, px);
    EXPECT_EQ(1.5f, px[0]);
    EXPECT_EQ(2.5f, px[1]);
}

TEST(ImageTest, ImageFillColor)
{
    mve::FloatImage img(2, 2, 3);
    float color[] = { 1.0f, 2.0f, 3.0f };
    img.fill_color(color);
    for (int i = 0; i < img.get_value_amount(); ++i)
        EXPECT_EQ(color[i % 3], img.at(i));
}
