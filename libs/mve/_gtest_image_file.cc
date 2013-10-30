// Test cases for the MVE image file reader/writer.
// Written by Simon Fuhrmann.

#include <fstream>
#include <cstdio>
#include <string>
#include <gtest/gtest.h>

#include "util/file_system.h"
#include "mve/image.h"
#include "mve/image_io.h"

struct TempFile : public std::string
{
    TempFile (std::string const& postfix)
        : std::string(std::tmpnam(NULL))
    {
        this->append(postfix);
    }

    ~TempFile (void)
    {
        util::fs::unlink(this->c_str());
    }
};

bool
compare_jpeg (mve::ByteImage::ConstPtr img1, mve::ByteImage::ConstPtr img2)
{
    if (img1->width() != img2->width()
        || img1->height() != img2->height()
        || img1->channels() != img2->channels())
        return false;
    // Accumulate error over all image values.
    int error = 0;
    for (int i = 0; i < img1->get_value_amount(); ++i)
        error += std::abs((int)img1->at(i) - (int)img2->at(i));
    // Two times the number of values seems a reasonable JPEG loss.
    if (error > img1->get_value_amount() * 2)
        return false;
    return true;
}

template <typename T>
bool
compare_exact (typename mve::Image<T>::ConstPtr img1,
    typename mve::Image<T>::ConstPtr img2)
{
    if (img1->width() != img2->width()
        || img1->height() != img2->height()
        || img1->channels() != img2->channels())
        return false;
    for (int i = 0; i < img1->get_value_amount(); ++i)
        if (img1->at(i) != img2->at(i))
            return false;
    return true;
}

mve::ByteImage::Ptr
make_byte_image (int width, int height, int channels)
{
    mve::ByteImage::Ptr img = mve::ByteImage::create(width, height, channels);
    for (int i = 0; i < img->get_value_amount(); ++i)
        img->at(i) = i % 256;
    uint8_t special_values[] = { 0, 127, 128, 255 };
    if (img->get_value_amount() >= 4)
        std::copy(special_values, special_values + 4, img->begin());
    return img;
}

mve::FloatImage::Ptr
make_float_image (int width, int height, int channels)
{
    mve::FloatImage::Ptr img = mve::FloatImage::create(width, height, channels);
    for (int i = 0; i < img->get_value_amount(); ++i)
        img->at(i) = static_cast<float>(i) / 32.0f;
    return img;
}

mve::RawImage::Ptr
make_raw_image (int width, int height, int channels)
{
    mve::RawImage::Ptr img = mve::RawImage::create(width, height, channels);
    for (int i = 0; i < img->get_value_amount(); ++i)
        img->at(i) = i % (1<<14);
    uint16_t special_values[] = { 0, 32767, 32768, 65535 };
    if (img->get_value_amount() >= 4)
        std::copy(special_values, special_values + 4, img->begin());
    return img;
}

TEST(ImageFileTest, JPEGSaveLoad)
{
    TempFile filename("jpegtest");
    mve::ByteImage::Ptr img1, img2;

    img1 = make_byte_image(255, 256, 1);
    mve::image::save_jpg_file(img1, filename, 90);
    img2 = mve::image::load_jpg_file(filename);
    EXPECT_TRUE(compare_jpeg(img1, img2));

    img1 = make_byte_image(256, 255, 3);
    mve::image::save_jpg_file(img1, filename, 90);
    img2 = mve::image::load_jpg_file(filename);
    EXPECT_TRUE(compare_jpeg(img1, img2));
}

TEST(ImageFileTest, PNGSaveLoad)
{
    TempFile filename("pngtest");
    mve::ByteImage::Ptr img1, img2;

    img1 = make_byte_image(256, 255, 1);
    mve::image::save_png_file(img1, filename);
    img2 = mve::image::load_png_file(filename);
    EXPECT_TRUE(compare_exact<uint8_t>(img1, img2));

    img1 = make_byte_image(256, 255, 2);
    mve::image::save_png_file(img1, filename);
    img2 = mve::image::load_png_file(filename);
    EXPECT_TRUE(compare_exact<uint8_t>(img1, img2));

    img1 = make_byte_image(255, 256, 3);
    mve::image::save_png_file(img1, filename);
    img2 = mve::image::load_png_file(filename);
    EXPECT_TRUE(compare_exact<uint8_t>(img1, img2));

    img1 = make_byte_image(255, 256, 4);
    mve::image::save_png_file(img1, filename);
    img2 = mve::image::load_png_file(filename);
    EXPECT_TRUE(compare_exact<uint8_t>(img1, img2));
}

TEST(ImageFileTest, PPMSaveLoad)
{
    TempFile filename("ppmtest");
    mve::ByteImage::Ptr img1, img2;

    img1 = make_byte_image(256, 255, 1);
    mve::image::save_ppm_file(img1, filename);
    img2 = mve::image::load_ppm_file(filename);
    EXPECT_TRUE(compare_exact<uint8_t>(img1, img2));

    img1 = make_byte_image(256, 255, 3);
    mve::image::save_ppm_file(img1, filename);
    img2 = mve::image::load_ppm_file(filename);
    EXPECT_TRUE(compare_exact<uint8_t>(img1, img2));
}

TEST(ImageFileTest, TIFFSaveLoad)
{
    TempFile filename("tifftest");
    mve::ByteImage::Ptr img1, img2;

    img1 = make_byte_image(256, 255, 1);
    mve::image::save_tiff_file(img1, filename);
    img2 = mve::image::load_tiff_file(filename);
    EXPECT_TRUE(compare_exact<uint8_t>(img1, img2));

    img1 = make_byte_image(255, 256, 2);
    mve::image::save_tiff_file(img1, filename);
    img2 = mve::image::load_tiff_file(filename);
    EXPECT_TRUE(compare_exact<uint8_t>(img1, img2));

    img1 = make_byte_image(256, 257, 3);
    mve::image::save_tiff_file(img1, filename);
    img2 = mve::image::load_tiff_file(filename);
    EXPECT_TRUE(compare_exact<uint8_t>(img1, img2));

    img1 = make_byte_image(128, 63, 4);
    mve::image::save_tiff_file(img1, filename);
    img2 = mve::image::load_tiff_file(filename);
    EXPECT_TRUE(compare_exact<uint8_t>(img1, img2));

    img1 = make_byte_image(64, 31, 5);
    mve::image::save_tiff_file(img1, filename);
    img2 = mve::image::load_tiff_file(filename);
    EXPECT_TRUE(compare_exact<uint8_t>(img1, img2));
}

TEST(ImageFileTest, PFMSaveLoad)
{
    TempFile filename("pfmtest");
    mve::FloatImage::Ptr img1, img2;

    img1 = make_float_image(256, 255, 1);
    mve::image::save_pfm_file(img1, filename);
    img2 = mve::image::load_pfm_file(filename);
    EXPECT_TRUE(compare_exact<float>(img1, img2));

    img1 = make_float_image(155, 324, 3);
    mve::image::save_pfm_file(img1, filename);
    img2 = mve::image::load_pfm_file(filename);
    EXPECT_TRUE(compare_exact<float>(img1, img2));
}

TEST(ImageFileTest, PFMLoadScale)
{
    TempFile filename("pfmtestscale");
    std::ofstream out(filename.c_str());
    float value = 10.0f;
    out << "Pf\n1 1 -2.0\n";
    out.write(reinterpret_cast<char const*>(&value), sizeof(float));
    out.close();

    mve::FloatImage::Ptr img = mve::image::load_pfm_file(filename);
    EXPECT_EQ(img->width(), 1);
    EXPECT_EQ(img->height(), 1);
    EXPECT_EQ(img->channels(), 1);
    EXPECT_EQ(img->at(0), 20.0f);
}

TEST(ImageFileTest, PPM16SaveLoad)
{
    TempFile filename("ppm16test");
    mve::RawImage::Ptr img1, img2;

    img1 = make_raw_image(256, 255, 1);
    mve::image::save_ppm_16_file(img1, filename);
    img2 = mve::image::load_ppm_16_file(filename);
    EXPECT_TRUE(compare_exact<uint16_t>(img1, img2));

    img1 = make_raw_image(155, 324, 3);
    mve::image::save_ppm_16_file(img1, filename);
    img2 = mve::image::load_ppm_16_file(filename);
    EXPECT_TRUE(compare_exact<uint16_t>(img1, img2));
}

TEST(ImageFileTest, TIFF16SaveLoad)
{
    TempFile filename("tiff16test");
    mve::RawImage::Ptr img1, img2;

    img1 = make_raw_image(123, 255, 1);
    mve::image::save_tiff_16_file(img1, filename);
    img2 = mve::image::load_tiff_16_file(filename);
    EXPECT_TRUE(compare_exact<uint16_t>(img1, img2));

    img1 = make_raw_image(155, 324, 3);
    mve::image::save_tiff_16_file(img1, filename);
    img2 = mve::image::load_tiff_16_file(filename);
    EXPECT_TRUE(compare_exact<uint16_t>(img1, img2));
}
