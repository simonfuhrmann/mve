// Test cases for perspective 3 point algorithms.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include <vector>

#include "math/vector.h"
#include "math/matrix.h"
#include "sfm/pose_p3p.h"

namespace
{
    void
    fill_colinear_points_and_directions (std::vector<math::Vec3d>* p,
        std::vector<math::Vec3d>* d)
    {
        math::Vec3d p1(-1.0, -1.0, 2.0);
        math::Vec3d p2(0.0, 0.0, 2.0);
        math::Vec3d p3(1.0, 1.0, 2.0);
        p->clear();
        p->push_back(p1);
        p->push_back(p2);
        p->push_back(p3);
        d->clear();
        d->push_back(p1.normalized());
        d->push_back(p2.normalized());
        d->push_back(p3.normalized());
    }

    void
    fill_test_points_and_directions (std::vector<math::Vec3d>* p,
        std::vector<math::Vec3d>* d)
    {
        math::Vec3d p1(-1.0, 1.0, 2.0);
        math::Vec3d p2(0.0, 0.0, 2.0);
        math::Vec3d p3(1.0, 1.0, 2.0);
        p->clear();
        p->push_back(p1);
        p->push_back(p2);
        p->push_back(p3);
        d->clear();
        d->push_back(p1.normalized());
        d->push_back(p2.normalized());
        d->push_back(p3.normalized());
    }

    void
    fill_groundtruth_data (std::vector<math::Vec3d>* p,
        std::vector<math::Vec3d>* d, math::Matrix<double, 3, 4>* s)
    {
        math::Matrix<double, 4, 4> pose(0.0);
        double const angle = MATH_PI / 4.0f;
        pose[0] = std::cos(angle);
        pose[2] = std::sin(angle);
        pose[5] = 1.0;
        pose[8] = -std::sin(angle);
        pose[10] = std::cos(angle);
        pose[15] = 1.0;

        math::Vec3d p1(2.0, 1.0, 2.0);
        math::Vec3d p2(3.0, -1.0, 2.0);
        math::Vec3d p3(2.0, -3.0, 3.0);
        p->clear();
        p->push_back(p1);
        p->push_back(p2);
        p->push_back(p3);
        d->clear();
        d->push_back(pose.mult(p1, 1.0).normalized());
        d->push_back(pose.mult(p2, 1.0).normalized());
        d->push_back(pose.mult(p3, 1.0).normalized());
        *s = math::Matrix<double, 3, 4>(pose.begin());
    }
}

TEST(PoseP3PTest, NumSolutions)
{
    std::vector<math::Vec3d> points, directions;
    std::vector<math::Matrix<double, 3, 4> > solutions;
    fill_test_points_and_directions(&points, &directions);

    sfm::pose_p3p_kneip(points[0], points[1], points[2],
        directions[0], directions[1], directions[2], &solutions);
    EXPECT_EQ(4, solutions.size());

    fill_colinear_points_and_directions(&points, &directions);
    sfm::pose_p3p_kneip(points[0], points[1], points[2],
        directions[0], directions[1], directions[2], &solutions);
    EXPECT_EQ(0, solutions.size());
}

TEST(PoseP3PTest, GroundTruth1)
{
    std::vector<math::Vec3d> points, directions;
    math::Matrix<double, 3, 4> solution;
    fill_groundtruth_data(&points, &directions, &solution);

    std::vector<math::Matrix<double, 3, 4> > solutions;
    sfm::pose_p3p_kneip(points[0], points[1], points[2],
        directions[0], directions[1], directions[2], &solutions);
    bool found_good_solution = false;
    for (std::size_t i = 0; i < solutions.size(); ++i)
    {
        if (solution.is_similar(solutions[i], 1e-10))
            found_good_solution = true;
    }
    EXPECT_TRUE(found_good_solution);
}
