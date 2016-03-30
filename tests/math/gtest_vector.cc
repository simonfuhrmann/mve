/*
 * Test cases for the vector class.
 * Written by Simon Fuhrmann.
 */

#include <gtest/gtest.h>

#include "math/vector.h"

TEST(VectorTest, MiscOperations)
{
    using namespace math;
    EXPECT_EQ(Vec2f(1.0f), Vec2f(1.0f, 1.0f));
    EXPECT_EQ(Vec2f(1.0f, 2.0f) + Vec2f(2.0f, 3.0f), Vec2f(3.0f, 5.0f));
    EXPECT_EQ(Vec2f(1.0f, 2.0f) - Vec2f(2.0f, 3.0f), Vec2f(-1.0f, -1.0f));
    EXPECT_EQ(Vec2f(10.0f, 0.0f).normalize(), Vec2f(1.0f, 0.0f));
    EXPECT_EQ(Vec2f(3.0f, 4.0f).norm(), 5.0f);
    EXPECT_EQ(Vec2f(2.0f, 2.0f).square_norm(), 8.0f);
    EXPECT_EQ(Vec2f::dim, 2);
    EXPECT_EQ(Vec3f::dim, 3);
    EXPECT_EQ(Vec4f::dim, 4);
    EXPECT_EQ(Vec2f(-1.0f, 2.0f).abs_value(), Vec2f(1.0f, 2.0f));
    EXPECT_EQ(Vec2f(-1.0f, -2.0f).abs_value(), Vec2f(1.0f, 2.0f));
    EXPECT_EQ(Vec2f(1.0f, -2.0f).abs_value(), Vec2f(1.0f, 2.0f));
    EXPECT_EQ(Vec2f(1.0f, 4.0f).dot(Vec2f(2.0f, 10.0f)), 42.0f);
    EXPECT_EQ(Vec2f(1.0f, 4.0f).dot(Vec2f(2.0f, 10.0f)), 42.0f);
    EXPECT_EQ(Vec2f(-1.0f, 4.0f).dot(Vec2f(2.0f, 10.0f)), 38.0f);
    EXPECT_EQ(Vec2f(1.0f, -4.0f).dot(Vec2f(2.0f, 10.0f)), -38.0f);
    EXPECT_EQ(Vec2f(-1.0f, -4.0f).dot(Vec2f(2.0f, 10.0f)), -42.0f);
    EXPECT_EQ(Vec3f(1.0f, 0.0f, 0.0f).cross(Vec3f(0.0f, 1.0f, 0.0f)),
        Vec3f(0.0f, 0.0f, 1.0f));
    EXPECT_EQ(Vec2f(1.0f, 2.0f).cw_mult(Vec2f(5.0f, 6.0f)), Vec2f(5.0f, 12.0f));
    EXPECT_EQ(Vec2f(3.0f, 2.0f).cw_mult(Vec2f(2.0f, 3.0f)), Vec2f(6.0f, 6.0f));

    EXPECT_TRUE(Vec2f(1.0f, 2.0f).is_similar(Vec2f(1.0f, 2.1f), 0.1f));
    EXPECT_TRUE(Vec2f(1.0f, 2.0f).is_similar(Vec2f(1.0f, 2.1f), 0.2f));
    EXPECT_TRUE(!Vec2f(1.0f, 2.0f).is_similar(Vec2f(1.0f, 2.1f), 0.05f));
    EXPECT_TRUE(Vec2f(0.0f, 0.0f).is_similar(Vec2f(0.0f, 0.0f), 0.0f));
    EXPECT_TRUE(Vec2f(1.0f, 0.0f).is_similar(Vec2f(1.0f, 0.0f), 0.0f));

    EXPECT_EQ(Vec2f(1.0f, 100.0f).maximum(), 100.0f);
    EXPECT_EQ(Vec2f(1.0f, 100.0f).minimum(), 1.0f);
    EXPECT_EQ(Vec2f(-1000.0f, 100.0f).minimum(), -1000.0f);
    EXPECT_EQ(Vec2f(-1000.0f, 100.0f).maximum(), 100.0f);
    EXPECT_EQ(Vec3f(2.0f, 2.0f, 2.0f).product(), 8.0f);
    EXPECT_EQ(Vec3f(2.0f, 2.0f, 2.0f).sum(), 6.0f);
    EXPECT_EQ(Vec3f(2.0f, -1.0f, -1.0f).sum(), 0.0f);
    EXPECT_EQ(Vec3f(2.0f, -1.0f, -1.0f).abs_sum(), 4.0f);

    EXPECT_EQ(Vec3f(1.0f, 2.0f, 3.0f).sort_asc(), Vec3f(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(Vec3f(3.0f, 2.0f, 1.0f).sort_asc(), Vec3f(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(Vec3f(1.0f, 3.0f, 2.0f).sort_asc(), Vec3f(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(Vec3f(1.0f, 2.0f, 3.0f).sort_desc(), Vec3f(3.0f, 2.0f, 1.0f));
    EXPECT_EQ(Vec3f(3.0f, 2.0f, 1.0f).sort_desc(), Vec3f(3.0f, 2.0f, 1.0f));
    EXPECT_EQ(Vec3f(1.0f, 3.0f, 2.0f).sort_desc(), Vec3f(3.0f, 2.0f, 1.0f));
}
