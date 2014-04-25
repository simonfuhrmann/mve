/*
 * Test cases for geometric routines.
 * Written by Simon Fuhrmann.
 */

#include <gtest/gtest.h>

#include "math/vector.h"
#include "math/matrix.h"
#include "math/octree_tools.h"

TEST(GeomTests, RayRayIntersectTest)
{
    math::Vec3d p1, d1, p2,d2;
    p1[0] = 0; p1[1] = 0; p1[2] = 0;
    d1[0] = 0; d1[1] = 0; d1[2] = 1;
    p2[0] = 0; p2[1] = 0; p2[2] = 0;
    d2[0] = 0; d2[1] = 1; d2[2] = 0;

    math::Vec2d t = math::geom::ray_ray_intersect<double>(p1,d1,p2,d2);
    EXPECT_EQ(math::Vec2d(0.0), t);

    p2[0] = 1.0;
    t = math::geom::ray_ray_intersect<double>(p1,d1,p2,d2);
    EXPECT_EQ(math::Vec2d(0.0), t);

    p2[0] = 1; p2[1] = 1; p2[2] = 1;
    t = math::geom::ray_ray_intersect<double>(p1,d1,p2,d2);
    EXPECT_EQ((t[0]*d1+p1), math::Vec3f(0,0,1));
    EXPECT_EQ((t[1]*d2+p2), math::Vec3f(1,0,1));
}
