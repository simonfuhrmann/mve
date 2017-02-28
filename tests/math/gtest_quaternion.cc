/*
 * Test cases for the quaternion class.
 * Written by Nils Moehrle.
 */

#include <gtest/gtest.h>

#include "math/quaternion.h"

TEST(QuaternionTest, Rotate)
{
    using namespace math;
    Vec3f x(1.0f, 0.0f, 0.0f);
    Vec3f y(0.0f, 1.0f, 0.0f);
    Vec3f z(0.0f, 0.0f, 1.0f);

    /* 90 deg ccw rotation around z. */
    Quat4f q(z, M_PI / 2.0f);

    float error = (y - q.rotate(x)).norm();
    EXPECT_TRUE(error < 1e-7f);
}
