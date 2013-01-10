// Test cases for the MVE image file reader/writer.
// Written by Simon Fuhrmann.

#include <cstdio>
#include <gtest/gtest.h>

#include "mve/image.h"
#include "mve/imagefile.h"

bool
compare_jpeg (mve::ByteImage::ConstPtr img1, mve::ByteImage::ConstPtr img2)
{
    if (img1->width() != img2->width() || img1->height() != img2->height()
        || img1->channels() != img2->channels())
        return false;
    return true;
}

bool
compare_exact (mve::ByteImage::ConstPtr img1, mve::ByteImage::ConstPtr img2)
{
    if (img1->width() != img2->width() || img1->height() != img2->height()
        || img1->channels() != img2->channels())
        return false;
    for (int i = 0; i < img1->get_value_amount(); ++i)
        if (img1->at(i) != img2->at(i))
            return false;
    return true;
}

mve::ByteImage::Ptr
make_image (int width, int height, int channels)
{
    mve::ByteImage::Ptr img = mve::ByteImage::create(width, height, channels);
    for (int i = 0; i < img->get_value_amount(); ++i)
        img->at(i) = i % 256;
    return img;
}

TEST(ImageFileTest, JpegSaveLoad)
{
    std::string filename = std::tmpnam(NULL);

    mve::ByteImage::Ptr img1 = make_image(255, 256, 1);
    mve::image::save_jpg_file(img1, filename, 90);
    mve::ByteImage::Ptr img2 = mve::image::load_jpg_file(filename);
    EXPECT_TRUE(compare_jpeg(img1, img2));

    img1 = make_image(256, 255, 3);
    mve::image::save_jpg_file(img1, filename, 90);
    img2 = mve::image::load_jpg_file(filename);
    EXPECT_TRUE(compare_jpeg(img1, img2));
}

TEST(ImageFileTest, PngSaveLoad)
{
    std::string filename = std::tmpnam(NULL);

    mve::ByteImage::Ptr img1 = make_image(256, 255, 1);
    mve::image::save_png_file(img1, filename);
    mve::ByteImage::Ptr img2 = mve::image::load_png_file(filename);
    EXPECT_TRUE(compare_exact(img1, img2));

    img1 = make_image(256, 255, 2);
    mve::image::save_png_file(img1, filename);
    img2 = mve::image::load_png_file(filename);
    EXPECT_TRUE(compare_exact(img1, img2));

    img1 = make_image(255, 256, 3);
    mve::image::save_png_file(img1, filename);
    img2 = mve::image::load_png_file(filename);
    EXPECT_TRUE(compare_exact(img1, img2));

    img1 = make_image(255, 256, 4);
    mve::image::save_png_file(img1, filename);
    img2 = mve::image::load_png_file(filename);
    EXPECT_TRUE(compare_exact(img1, img2));
}

// TODO: TIFF, TIFF16, PPM, PPM16, PFM
