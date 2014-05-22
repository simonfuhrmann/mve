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

TEST(MathFunctionsTest, RoundTest)
{
    EXPECT_EQ(1.0f, math::round(1.1f));
    EXPECT_EQ(2.0f, math::round(1.5f));
    EXPECT_EQ(2.0f, math::round(1.7f));
    EXPECT_EQ(-1.0f, math::round(-0.5f));
    EXPECT_EQ(-1.0f, math::round(-0.7f));
    EXPECT_EQ(-1.0f, math::round(-1.1f));
    EXPECT_EQ(-2.0f, math::round(-1.5f));
    EXPECT_EQ(0.0f, math::round(-0.4f));

    EXPECT_EQ(1.0, math::round(1.1));
    EXPECT_EQ(2.0, math::round(1.5));
    EXPECT_EQ(2.0, math::round(1.7));
    EXPECT_EQ(-1.0, math::round(-0.5));
    EXPECT_EQ(-1.0, math::round(-0.7));
    EXPECT_EQ(-1.0, math::round(-1.1));
    EXPECT_EQ(-2.0, math::round(-1.5));
    EXPECT_EQ(0.0, math::round(-0.4));
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
