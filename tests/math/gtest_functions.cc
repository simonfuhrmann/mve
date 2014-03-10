/*
 * Test cases for math functions.
 * Written by Simon Fuhrmann and Michael Waechter.
 */

#include <gtest/gtest.h>

#include "math/vector.h"
#include "math/matrix.h"
#include "math/functions.h"
#include "math/matrix_tools.h"

TEST(MathFunctionsTest, GaussianTest)
{
    using namespace math;

    EXPECT_EQ(gaussian(0.0f, 1.0f), 1.0f);

    EXPECT_NEAR(gaussian( 1.0f, 1.0f), 0.606530659712633f, 1e-14f);
    EXPECT_NEAR(gaussian(-1.0f, 1.0f), 0.606530659712633f, 1e-14f);
    EXPECT_NEAR(gaussian( 2.0f, 1.0f), 0.135335283236613f, 1e-14f);
    EXPECT_NEAR(gaussian(-2.0f, 1.0f), 0.135335283236613f, 1e-14f);

    EXPECT_NEAR(gaussian( 1.0f, 2.0f), 0.882496902584595f, 1e-14f);
    EXPECT_NEAR(gaussian(-1.0f, 2.0f), 0.882496902584595f, 1e-14f);
    EXPECT_NEAR(gaussian( 2.0f, 2.0f), 0.606530659712633f, 1e-14f);
    EXPECT_NEAR(gaussian(-2.0f, 2.0f), 0.606530659712633f, 1e-14f);
}

TEST(MathFunctionsTest, GaussianNDTest)
{
    using namespace math;

    Vec3d zero(0.0, 0.0, 0.0);
    Matrix3d eye;
    matrix_set_identity(&eye);
    EXPECT_EQ(gaussian_nd(zero, eye), 1.0);

    double const arr[] = {
        0.126854, 0.016426, 0.015765,
        0.016426, 0.114678, 0.017557,
        0.015765, 0.017557, 0.194152};
    Matrix3d cov_inv(arr);
    Vec3d x(-4.8551, -4.3369, 3.9772);
    // correct result computed with octave
    EXPECT_NEAR(gaussian_nd(x, cov_inv), 0.0213277, 1e-7);
}

TEST(MathFunctionsTest, FastPowTest)
{
    EXPECT_EQ(1, math::fastpow(10, 0));
    EXPECT_EQ(10, math::fastpow(10, 1));
    EXPECT_EQ(100, math::fastpow(10, 2));
    EXPECT_EQ(1000, math::fastpow(10, 3));

    EXPECT_EQ(1, math::fastpow(2, 0));
    EXPECT_EQ(2, math::fastpow(2, 1));
    EXPECT_EQ(4, math::fastpow(2, 2));
    EXPECT_EQ(8, math::fastpow(2, 3));
    EXPECT_EQ(16, math::fastpow(2, 4));
    EXPECT_EQ(32, math::fastpow(2, 5));
    EXPECT_EQ(64, math::fastpow(2, 6));
    EXPECT_EQ(128, math::fastpow(2, 7));
    EXPECT_EQ(256, math::fastpow(2, 8));
    EXPECT_EQ(512, math::fastpow(2, 9));
    EXPECT_EQ(1024, math::fastpow(2, 10));
}
