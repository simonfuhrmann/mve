// Test cases for the SURF feature detector.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "sfm/surf.h"

class SurfTest : public sfm::Surf, public testing::Test
{
public:
    SurfTest()
        : sfm::Surf(sfm::Surf::Options())
    {
    }
};

/*
 * This is a collection of test images, e.g. gradients.
 */
namespace
{
    mve::ByteImage::Ptr
    create_constant_image(int size, unsigned char value)
    {
        mve::ByteImage::Ptr img = mve::ByteImage::create(size, size, 1);
        img->fill(value);
        return img;
    }

    mve::ByteImage::Ptr
    create_incrementing_image(int size)
    {
        mve::ByteImage::Ptr img = mve::ByteImage::create(size, size, 1);
        for (int i = 0; i < size * size; ++i)
            img->at(i) = i;
        return img;
    }

    mve::ByteImage::Ptr
    create_gradient_x_image(int size, bool negate)
    {
        mve::ByteImage::Ptr img = mve::ByteImage::create(size, size, 1);
        for (int i = 0; i < size * size; ++i)
            img->at(i) = negate ? 255 - (i % size) : (i % size);
        return img;
    }

    mve::ByteImage::Ptr
    create_gradient_y_image(int size, bool negate)
    {
        mve::ByteImage::Ptr img = mve::ByteImage::create(size, size, 1);
        for (int i = 0; i < size * size; ++i)
            img->at(i) = negate ? 255 - (i / size) : (i / size);
        return img;
    }

    mve::ByteImage::Ptr
    create_gradient_xy_image(int size, bool negate)
    {
        mve::ByteImage::Ptr img = mve::ByteImage::create(size, size, 1);
        for (int i = 0; i < size * size; ++i)
            img->at(i) = negate ? 255 - (i % size) - (i / size)
                : (i % size) + (i / size);
        return img;
    }

    mve::ByteImage::Ptr
    create_square_gradient_x_image(int size)
    {
        mve::ByteImage::Ptr img = mve::ByteImage::create(size, size, 1);
        for (int i = 0; i < size * size; ++i)
            img->at(i) = (i % size) * (i % size);
        return img;
    }

}  // namespace

TEST_F(SurfTest, TestSmallImages)
{
    for (int i = 0; i < 20; ++i)
    {
        mve::ByteImage::Ptr img = mve::ByteImage::create(i, i, 1);
        sfm::Surf surf = sfm::Surf(sfm::Surf::Options());
        surf.set_image(img);
        surf.process();
    }
}

TEST_F(SurfTest, TestFilterDxxDyy)
{
    // Smallest filter fs = 3, width = 3 * fs, height: 2 * fs - 1
    // Create a constant color image. dxx and dyy are 0.
    this->set_image(create_constant_image(10, 100));
    Surf::SatType dxx = this->filter_dxx(3, 5, 5);
    Surf::SatType dyy = this->filter_dyy(3, 5, 5);
    EXPECT_EQ(0, dxx);
    EXPECT_EQ(0, dyy);

    // Create a gradient. dx = 1, dy = width but dxx = 0 and dyy = 0.
    this->set_image(create_incrementing_image(10));
    dxx = this->filter_dxx(3, 5, 5);
    dyy = this->filter_dyy(3, 5, 5);
    EXPECT_EQ(0, dxx);
    EXPECT_EQ(0, dyy);

    this->set_image(create_square_gradient_x_image(10));
    dxx = this->filter_dxx(3, 5, 5);
    dyy = this->filter_dyy(3, 5, 5);
    EXPECT_GT(dxx, 0); // TODO Understand the scale of this value.
    EXPECT_EQ(0, dyy);
}

TEST_F(SurfTest, TestHaarWaveletsDXY)
{
    float dx, dy;

    // Test Haar Wavelets in dy.
    this->set_image(create_gradient_y_image(4, false));
    this->filter_dx_dy(2, 2, 1, &dx, &dy);
    EXPECT_EQ(1.0f, dy);
    EXPECT_EQ(0.0f, dx);

    // Test Haar Wavelets in dx.
    this->set_image(create_gradient_x_image(4, false));
    this->filter_dx_dy(2, 2, 1, &dx, &dy);
    EXPECT_EQ(0.0f, dy);
    EXPECT_EQ(1.0f, dx);

    // Test Haar Wavelets in -dx -dy.
    this->set_image(create_gradient_xy_image(4, true));
    this->filter_dx_dy(2, 2, 1, &dx, &dy);
    EXPECT_EQ(-1.0f, dy);
    EXPECT_EQ(-1.0f, dx);
}

TEST_F(SurfTest, TestHaarWaveletsSmallKernel)
{
    float dx, dy;
    this->set_image(create_incrementing_image(4));
    this->filter_dx_dy(2, 2, 1, &dx, &dy);
    EXPECT_EQ(1.0f, dx);
    EXPECT_EQ(4.0f, dy);
}

TEST_F(SurfTest, TestHaarWaveletsLargerKernel)
{
    float dx, dy;
    this->set_image(create_incrementing_image(6));
    this->filter_dx_dy(3, 3, 2, &dx, &dy);
    EXPECT_EQ(1.0f, dx);
    EXPECT_EQ(6.0f, dy);
}

TEST_F(SurfTest, TestHaarWaveletsHugeKernel)
{
    float dx, dy;
    this->set_image(create_gradient_xy_image(100, false));
    this->filter_dx_dy(50, 50, 40, &dx, &dy);
    EXPECT_EQ(1.0f, dx);
    EXPECT_EQ(1.0f, dy);
}

TEST_F(SurfTest, TestDescriptorNoCrashSmallImages)
{
    for (int i = 0; i < 20; ++i)
    {
        sfm::Surf::Descriptor descr;
        descr.scale = 1.2;
        descr.x = i / 2;
        descr.y = i / 2;
        this->set_image(create_constant_image(i, 100));
        this->descriptor_orientation(&descr);
    }
}

TEST_F(SurfTest, TestDescriptorOrientation)
{
    // Mock Descriptor.
    sfm::Surf::Descriptor descr;
    descr.scale = 1.2;
    descr.x = 10;
    descr.y = 10;

    // Create image with gradient to the right.
    this->set_image(create_gradient_x_image(20, false));
    this->descriptor_orientation(&descr);
    EXPECT_NEAR(0.0f, descr.orientation, 1e-5f);

    // Create image with gradient to the left.
    this->set_image(create_gradient_x_image(20, true));
    this->descriptor_orientation(&descr);
    EXPECT_NEAR(MATH_PI, descr.orientation, 1e-5f);

    // Create image with gradient to the bottom.
    this->set_image(create_gradient_y_image(20, false));
    this->descriptor_orientation(&descr);
    EXPECT_NEAR(MATH_PI / 2.0, descr.orientation, 1e-5f);

    // Create image with gradient to the top.
    this->set_image(create_gradient_y_image(20, true));
    this->descriptor_orientation(&descr);
    EXPECT_NEAR(-MATH_PI / 2.0, descr.orientation, 1e-5f);

    // Create image with gradient to the top-right.
    this->set_image(create_gradient_xy_image(20, false));
    this->descriptor_orientation(&descr);
    EXPECT_NEAR(MATH_PI / 4.0, descr.orientation, 1e-5f);

    // Create image with gradient to the top-right.
    this->set_image(create_gradient_xy_image(20, true));
    this->descriptor_orientation(&descr);
    EXPECT_NEAR(-3.0f * MATH_PI / 4.0, descr.orientation, 1e-5f);

    // Potential error case: constant image.
    this->set_image(create_constant_image(20, 0));
    this->descriptor_orientation(&descr);
    EXPECT_NEAR(0.0f, descr.orientation, 1e-5f);

    this->set_image(create_constant_image(20, 255));
    this->descriptor_orientation(&descr);
    EXPECT_NEAR(0.0f, descr.orientation, 1e-5f);
}

// Test descriptor on various patterns (paper)

TEST_F(SurfTest, TestDescriptorUpright)
{
    // Make sure the upright descriptor produces the same results as the
    // rotation invariant descriptor with upright orientation (0 deg).
    sfm::Surf::Descriptor descr;
    descr.scale = 1.2;
    descr.x = 25;
    descr.y = 25;
    descr.orientation = 0.0f;

    this->set_image(create_gradient_xy_image(50, true));
    this->descriptor_computation(&descr, false);
    for (int i = 0; i < 64; ++i)
        if (i % 4 == 0)
            EXPECT_LT(descr.data[i], -1e-5f);  // sum dx
        else if (i % 4 == 1)
            EXPECT_LT(descr.data[i], -1e-5f);  // sum dy
        else if (i % 4 == 2)
            EXPECT_GT(descr.data[i], 1e-5f); // sum |dx|
        else if (i % 4 == 2)
            EXPECT_GT(descr.data[i], 1e-5f); // sum |dy|

    this->set_image(create_gradient_xy_image(50, true));
    this->descriptor_computation(&descr, true);
    for (int i = 0; i < 64; ++i)
        if (i % 4 == 0)
            EXPECT_LT(descr.data[i], -1e-5f);  // sum dx
        else if (i % 4 == 1)
            EXPECT_LT(descr.data[i], -1e-5f);  // sum dy
        else if (i % 4 == 2)
            EXPECT_GT(descr.data[i], 1e-5f); // sum |dx|
        else if (i % 4 == 2)
            EXPECT_GT(descr.data[i], 1e-5f); // sum |dy|
}

TEST_F(SurfTest, TestDescriptorComputation)
{
    // Mock Descriptor.
    sfm::Surf::Descriptor descr;
    descr.scale = 1.2;
    descr.x = 25;
    descr.y = 25;

    // Test on the upright descriptor first. For the invariant version
    // it remains to test proper rotation of coordiantes and responses.

    // Special Case: Constant image produces zero descripftor.
    // Normalization of vector can cause trouble.
    this->set_image(create_constant_image(50, 100));
    this->descriptor_computation(&descr, true);
    for (int i = 0; i < 64; ++i)
        EXPECT_NEAR(0.0f, descr.data[i], 1e-5f);

    this->set_image(create_gradient_x_image(50, false));
    this->descriptor_computation(&descr, true);
    for (int i = 0; i < 64; ++i)
        if (i % 2 == 0)
            EXPECT_GT(descr.data[i], 1e-5f);
        else
            EXPECT_NEAR(0.0f, descr.data[i], 1e-5f);

    this->set_image(create_gradient_y_image(50, true));
    this->descriptor_computation(&descr, true);
    for (int i = 0; i < 64; ++i)
        if (i % 4 == 0)
            EXPECT_NEAR(0.0f, descr.data[i], 1e-5f);  // sum dx
        else if (i % 4 == 1)
            EXPECT_LT(descr.data[i], -1e-5f);  // sum dy
        else if (i % 4 == 2)
            EXPECT_NEAR(0.0f, descr.data[i], 1e-5f); // sum |dx|
        else if (i % 4 == 2)
            EXPECT_GT(descr.data[i], 1e-5f); // sum |dy|

    this->set_image(create_gradient_xy_image(50, true));
    this->descriptor_computation(&descr, true);
    for (int i = 0; i < 64; ++i)
        if (i % 4 == 0)
            EXPECT_LT(descr.data[i], -1e-5f);  // sum dx
        else if (i % 4 == 1)
            EXPECT_LT(descr.data[i], -1e-5f);  // sum dy
        else if (i % 4 == 2)
            EXPECT_GT(descr.data[i], 1e-5f); // sum |dx|
        else if (i % 4 == 2)
            EXPECT_GT(descr.data[i], 1e-5f); // sum |dy|
}

TEST_F(SurfTest, TestRotationInvariance)
{
    // Mock Descriptor.
    sfm::Surf::Descriptor descr;
    descr.scale = 1.2;
    descr.x = 25;
    descr.y = 25;
    descr.orientation = MATH_PI / 2.0;

    this->set_image(create_gradient_y_image(50, false));
    this->descriptor_computation(&descr, false);

    for (int i = 0; i < 64; ++i)
        if (i % 2 == 0)
            EXPECT_GT(descr.data[i], 1e-5f);
        else
            EXPECT_NEAR(0.0f, descr.data[i], 1e-5f);
}
