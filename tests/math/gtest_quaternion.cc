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

TEST(QuaternionTest, ToAndFromRotationMatrix)
{
    using namespace math;
    Vector<float, 9> rot;
    Vec3f x(1.0f, 0.0f, 0.0f);
    Vec3f y(0.0f, 1.0f, 0.0f);
    Vec3f z(0.0f, 0.0f, 1.0f);
    Quat4f q, q_test;

    /* 90 deg ccw rotation around x. */
    q.set(x, M_PI / 2.0f);
    q.to_rotation_matrix(*rot);
    q_test.set_from_rotation_matrix(*rot);

    EXPECT_NEAR(q[0], q_test[0], 1e-7);
    EXPECT_NEAR(q[1], q_test[1], 1e-7);
    EXPECT_NEAR(q[2], q_test[2], 1e-7);
    EXPECT_NEAR(q[3], q_test[3], 1e-7);

    /* 180 deg ccw rotation around y. */
    q.set(y, M_PI);
    q.to_rotation_matrix(*rot);
    q_test.set_from_rotation_matrix(*rot);

    EXPECT_NEAR(q[0], q_test[0], 1e-7);
    EXPECT_NEAR(q[1], q_test[1], 1e-7);
    EXPECT_NEAR(q[2], q_test[2], 1e-7);
    EXPECT_NEAR(q[3], q_test[3], 1e-7);

    /* 45 deg ccw rotation around z. */
    q.set(z, M_PI / 4.0f);
    q.to_rotation_matrix(*rot);
    q_test.set_from_rotation_matrix(*rot);

    EXPECT_NEAR(q[0], q_test[0], 1e-7);
    EXPECT_NEAR(q[1], q_test[1], 1e-7);
    EXPECT_NEAR(q[2], q_test[2], 1e-7);
    EXPECT_NEAR(q[3], q_test[3], 1e-7);
}
