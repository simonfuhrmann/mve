/*
 * Test cases for the generic type accumulator.
 * Written by Simon Fuhrmann.
 */

#include <gtest/gtest.h>

#include "math/accum.h"

TEST(AccumTest, TestFloatAccum)
{
    math::Accum<float> accum(0.0f);
    accum.add(1.0f, 1.0f);
    accum.add(2.0f, 1.0f);
    accum.add(3.0f, 1.0f);
    EXPECT_EQ(2.0f, accum.normalized());
}

TEST(AccumTest, TestFloatWeights)
{
    math::Accum<float> accum(0.0f);
    accum.add(2.0f, 0.5f);
    accum.add(10.0f, 0.5f);
    accum.add(6.0f, 0.5f);
    EXPECT_EQ((1.0f + 5.0f + 3.0f) / 1.5f, accum.normalized());
}

TEST(AccumTest, TestUCharRounding)
{
    math::Accum<unsigned char> accum(0);
    accum.add(11, 0.5f);
    accum.add(11, 0.5f);
    EXPECT_EQ(11, accum.normalized());
}

TEST(AccumTest, TestUCharNoOverflow)
{
    math::Accum<unsigned char> accum(0);
    accum.add(100, 1.0f);
    accum.add(150, 1.0f);
    accum.add(50, 1.0f);
    EXPECT_EQ(100, accum.normalized());
}

#if 0
TEST(AccumTest, TestIntRounding)
{
    // Supposed to fail for now!
    math::Accum<int> accum(0);
    accum.add(11, 0.5f);
    accum.add(11, 0.5f);
    EXPECT_EQ(11, accum.normalized());
}
#endif
