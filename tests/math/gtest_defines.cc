/*
 * Test cases for math defines.
 * Written by Simon Fuhrmann.
 */

#include <cmath>
#include <gtest/gtest.h>

#include "math/defines.h"

TEST(MathDefinesTest, ConstantsTest)
{
    EXPECT_NEAR(MATH_PI / 2.0, MATH_PI_2, 1e-30);
    EXPECT_NEAR(MATH_PI / 4.0, MATH_PI_4, 1e-30);
    EXPECT_NEAR(1.0 / MATH_PI, MATH_1_PI, 1e-30);
    EXPECT_NEAR(2.0 / MATH_PI, MATH_2_PI, 1e-30);
    // TODO: More constants
}

TEST(MathDefinesTest, PowerTest)
{
    EXPECT_EQ(1, MATH_POW2(1));
    EXPECT_EQ(1, MATH_POW3(1));
    EXPECT_EQ(1, MATH_POW4(1));

    EXPECT_EQ(4, MATH_POW2(2));
    EXPECT_EQ(8, MATH_POW3(2));
    EXPECT_EQ(16, MATH_POW4(2));

    EXPECT_EQ(4.0f, MATH_POW2(2.0f));
    EXPECT_EQ(8.0f, MATH_POW3(2.0f));
    EXPECT_EQ(16.0f, MATH_POW4(2.0f));
}

TEST(MathDefinesTest, InfTest)
{
    EXPECT_TRUE(std::isinf(MATH_POS_INF));
    EXPECT_TRUE(std::isinf(MATH_NEG_INF));
    EXPECT_TRUE(MATH_POS_INF > 0.0);
    EXPECT_TRUE(MATH_NEG_INF < 0.0);
    EXPECT_FALSE(MATH_POS_INF < 0.0);
    EXPECT_FALSE(MATH_NEG_INF > 0.0);
}

TEST(MathDefinesTest, EpsilonComparisonTest)
{
    EXPECT_TRUE(MATH_EPSILON_EQ(0.0, 1.0, 1.0));
    EXPECT_TRUE(MATH_EPSILON_EQ(0.0, -1.0, 1.0));
    EXPECT_TRUE(MATH_EPSILON_EQ(0.0, 0.0, 0.0));
    EXPECT_TRUE(MATH_EPSILON_LESS(0.0, 1.0, 0.9999));
    EXPECT_FALSE(MATH_EPSILON_EQ(0.0, 1.0, 0.9999));
    EXPECT_FALSE(MATH_EPSILON_EQ(0.0, -1.0, 0.9999));
    EXPECT_FALSE(MATH_EPSILON_LESS(0.0, 1.0, 1.0));
    EXPECT_FALSE(MATH_EPSILON_LESS(0.0, 1.0, 1.0001));
}
