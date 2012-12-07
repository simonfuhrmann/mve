// Test cases for the image class and related features.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "sfm/surf.h"

class SurfTest : public sfm::Surf, public testing::Test
{
};

TEST_F(SurfTest, TestSmallImages)
{
    for (int i = 0; i < 20; ++i)
    {
        mve::ByteImage::Ptr img = mve::ByteImage::create(i, i, 1);
        sfm::Surf surf;
        surf.set_image(img);
        surf.process();
    }
}

TEST_F(SurfTest, TestFilterDxxDyy)
{
    // Smallest filter fs = 3, width = 3 * fs, height: 2 * fs - 1

    // Create a constant color image. dxx and dyy are 0.
    mve::ByteImage::Ptr img = mve::ByteImage::create(10, 10, 1);
    for (int i = 0; i < 100; ++i)
        img->at(i) = 100;
    this->set_image(img);
    this->create_integral_image();
    Surf::SatType dxx = this->filter_dxx(3, 5, 5);
    Surf::SatType dyy = this->filter_dyy(3, 5, 5);
    EXPECT_EQ(0, dxx);
    EXPECT_EQ(0, dyy);

    // Create a gradient. dx = 1, dy = width but dxx = 0 and dyy = 0.
    for (int i = 0; i < 100; ++i)
        img->at(i) = i;
    this->set_image(img);
    this->create_integral_image();
    dxx = this->filter_dxx(3, 5, 5);
    dyy = this->filter_dyy(3, 5, 5);
    EXPECT_EQ(0, dxx);
    EXPECT_EQ(0, dyy);

    for (int i = 0; i < 100; ++i)
        img->at(i) = (i % 10) * (i % 10);
    this->set_image(img);
    this->create_integral_image();
    dxx = this->filter_dxx(3, 5, 5);
    dyy = this->filter_dyy(3, 5, 5);
    EXPECT_GT(dxx, 0); // TODO Understand the scale of this value.
    EXPECT_EQ(0, dyy);
}

TEST_F(SurfTest, TestHaarWaveletsDXY)
{
    mve::ByteImage::Ptr img = mve::ByteImage::create(4, 4, 1);
    float dx, dy;

    // Test Haar Wavelets in dy.
    for (int y = 0; y < img->height(); ++y)
        for (int x = 0; x < img->width(); ++x)
            img->at(x, y, 0) = y;
    this->set_image(img);
    this->create_integral_image();
    this->filter_dx_dy(2, 2, 1, &dx, &dy);
    EXPECT_EQ(1.0f, dy);
    EXPECT_EQ(0.0f, dx);

    // Test Haar Wavelets in dx.
    for (int y = 0; y < img->height(); ++y)
        for (int x = 0; x < img->width(); ++x)
            img->at(x, y, 0) = x;
    this->set_image(img);
    this->create_integral_image();
    this->filter_dx_dy(2, 2, 1, &dx, &dy);
    EXPECT_EQ(0.0f, dy);
    EXPECT_EQ(1.0f, dx);

    // Test Haar Wavelets in -dx -dy.
    for (int y = 0; y < img->height(); ++y)
        for (int x = 0; x < img->width(); ++x)
            img->at(x, y, 0) = 200 - x - y;
    this->set_image(img);
    this->create_integral_image();
    this->filter_dx_dy(2, 2, 1, &dx, &dy);
    EXPECT_EQ(-1.0f, dy);
    EXPECT_EQ(-1.0f, dx);
}

TEST_F(SurfTest, TestHaarWaveletsSmallKernel)
{
    mve::ByteImage::Ptr img = mve::ByteImage::create(4, 4, 1);
    float dx, dy;

    for (int i = 0; i < img->get_value_amount(); ++i)
        img->at(i) = 6 * i;
    this->set_image(img);
    this->create_integral_image();
    this->filter_dx_dy(2, 2, 1, &dx, &dy);
    EXPECT_EQ(6.0f, dx);
    EXPECT_EQ(24.0f, dy);
}

TEST_F(SurfTest, TestHaarWaveletsLargerKernel)
{
    mve::ByteImage::Ptr img = mve::ByteImage::create(6, 6, 1);
    float dx, dy;

    for (int i = 0; i < img->get_value_amount(); ++i)
        img->at(i) = i;
    this->set_image(img);
    this->create_integral_image();
    this->filter_dx_dy(3, 3, 2, &dx, &dy);
    EXPECT_EQ(1.0f, dx);
    EXPECT_EQ(6.0f, dy);
}

TEST_F(SurfTest, TestHaarWaveletsHugeKernel)
{
    mve::ByteImage::Ptr img = mve::ByteImage::create(100, 100, 1);
    float dx, dy;

    for (int y = 0; y < img->height(); ++y)
        for (int x = 0; x < img->width(); ++x)
            img->at(x, y, 0) = x + y;

    this->set_image(img);
    this->create_integral_image();
    this->filter_dx_dy(50, 50, 40, &dx, &dy);
    EXPECT_EQ(1.0f, dx);
    EXPECT_EQ(1.0f, dy);
}

TEST_F(SurfTest, TestDescriptorNoCrashSmallImages)
{
    for (int i = 0; i < 20; ++i)
    {
        sfm::SurfDescriptor descr;
        descr.scale = 1.2;
        descr.x = i / 2;
        descr.y = i / 2;
        mve::ByteImage::Ptr img = mve::ByteImage::create(i, i, 1);
        this->set_image(img);
        this->create_integral_image();
        this->descriptor_orientation(&descr);
    }
}

TEST_F(SurfTest, TestDescriptorOrientation)
{
    // Mock Descriptor.
    sfm::SurfDescriptor descr;
    descr.scale = 1.2;
    descr.x = 10;
    descr.y = 10;
    mve::ByteImage::Ptr img = mve::ByteImage::create(20, 20, 1);

    // Create image with gradient to the right.
    for (int i = 0; i < 20 * 20; ++i)
        img->at(i) = i % 20;
    this->set_image(img);
    this->create_integral_image();
    this->descriptor_orientation(&descr);
    EXPECT_NEAR(0.0f, descr.orientation, 1e-5f);

    // Create image with gradient to the left.
    for (int i = 0; i < 20 * 20; ++i)
        img->at(i) = 100 - i % 20;
    this->set_image(img);
    this->create_integral_image();
    this->descriptor_orientation(&descr);
    EXPECT_NEAR(MATH_PI, descr.orientation, 1e-5f);

    // Create image with gradient to the bottom.
    for (int i = 0; i < 20 * 20; ++i)
        img->at(i) = i / 20;
    this->set_image(img);
    this->create_integral_image();
    this->descriptor_orientation(&descr);
    EXPECT_NEAR(MATH_PI / 2.0, descr.orientation, 1e-5f);

    // Create image with gradient to the top.
    for (int i = 0; i < 20 * 20; ++i)
        img->at(i) = 100 - i / 20;
    this->set_image(img);
    this->create_integral_image();
    this->descriptor_orientation(&descr);
    EXPECT_NEAR(-MATH_PI / 2.0, descr.orientation, 1e-5f);

    // Create image with gradient to the top-right.
    for (int i = 0; i < 20 * 20; ++i)
        img->at(i) = 100 - i / 20 + i % 20;
    this->set_image(img);
    this->create_integral_image();
    this->descriptor_orientation(&descr);
    EXPECT_NEAR(-MATH_PI / 4.0, descr.orientation, 1e-5f);

    // Potential error case: constant image.
    img->fill(0);
    this->set_image(img);
    this->create_integral_image();
    this->descriptor_orientation(&descr);
    EXPECT_NEAR(0.0f, descr.orientation, 1e-5f);
}

