// Test cases for pose estimation.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "math/matrixtools.h"
#include "sfm/matrixsvd.h"
#include "sfm/pose.h"

TEST(PoseTest, PointNormalization1)
{
    // Normalization of 3 points.
    math::Matrix<float, 3, 3> set;
    set(0,0) = 5.0f; set(1,0) = 5.0f; set(2,0) = 1.0f;
    set(0,1) = -5.0f; set(1,1) = -1.0f; set(2,1) = 1.0f;
    set(0,2) = 0.0f; set(1,2) = 0.0f; set(2,2) = 1.0f;
    math::Matrix<float, 3, 3> trans;
    sfm::pose_find_normalization(set, &trans);
    EXPECT_NEAR(trans[0], 0.1f, 1e-6f);
    EXPECT_NEAR(trans[1], 0.0f, 1e-6f);
    EXPECT_NEAR(trans[2], 0.0f, 1e-6f);
    EXPECT_NEAR(trans[3], 0.0f, 1e-6f);
    EXPECT_NEAR(trans[4], 0.1f, 1e-6f);
    EXPECT_NEAR(trans[5], -(5.0f + -1.0f + 0.0f)/3.0f/10.0f, 1e-6f);
    EXPECT_NEAR(trans[6], 0.0f, 1e-6f);
    EXPECT_NEAR(trans[7], 0.0f, 1e-6f);
    EXPECT_NEAR(trans[8], 1.0f, 1e-6f);
}

TEST(PoseTest, PointNormalization2)
{
    // Normalization of 2 points.
    math::Matrix<double, 3, 2> set;
    set(0,0) = -4.0; set(1,0) = 8.0; set(2,0) = 1.0f;
    set(0,1) = -5.0; set(1,1) = 10.0f; set(2,1) = 1.0f;
    math::Matrix<double, 3, 3> trans;
    sfm::pose_find_normalization(set, &trans);
    EXPECT_NEAR(trans[0], 0.5, 1e-6);
    EXPECT_NEAR(trans[1], 0.0, 1e-6);
    EXPECT_NEAR(trans[2], 4.5/2.0, 1e-6);
    EXPECT_NEAR(trans[3], 0.0, 1e-6);
    EXPECT_NEAR(trans[4], 0.5, 1e-6);
    EXPECT_NEAR(trans[5], -9.0/2.0, 1e-6);
    EXPECT_NEAR(trans[6], 0.0, 1e-6);
    EXPECT_NEAR(trans[7], 0.0, 1e-6);
    EXPECT_NEAR(trans[8], 1.0, 1e-6);
}

namespace {

    void
    fill_golden_correspondences(sfm::Eight2DPoints& p1,
        sfm::Eight2DPoints& p2)
    {
        p1(0, 0) = 45;  p1(1, 0) = 210; p1(2, 0) = 1;
        p1(0, 1) = 253; p1(1, 1) = 211; p1(2, 1) = 1;
        p1(0, 2) = 154; p1(1, 2) = 188; p1(2, 2) = 1;
        p1(0, 3) = 27;  p1(1, 3) = 37;  p1(2, 3) = 1;
        p1(0, 4) = 209; p1(1, 4) = 164; p1(2, 4) = 1;
        p1(0, 5) = 33;  p1(1, 5) = 77;  p1(2, 5) = 1;
        p1(0, 6) = 93;  p1(1, 6) = 58;  p1(2, 6) = 1;
        p1(0, 7) = 66;  p1(1, 7) = 75;  p1(2, 7) = 1;

        p2(0, 0) = 87;  p2(1, 0) = 216; p2(2, 0) = 1;
        p2(0, 1) = 285; p2(1, 1) = 216; p2(2, 1) = 1;
        p2(0, 2) = 188; p2(1, 2) = 194; p2(2, 2) = 1;
        p2(0, 3) = 51;  p2(1, 3) = 49;  p2(2, 3) = 1;
        p2(0, 4) = 234; p2(1, 4) = 171; p2(2, 4) = 1;
        p2(0, 5) = 56;  p2(1, 5) = 88;  p2(2, 5) = 1;
        p2(0, 6) = 114; p2(1, 6) = 69;  p2(2, 6) = 1;
        p2(0, 7) = 87;  p2(1, 7) = 86;  p2(2, 7) = 1;
    }

}  // namespace

TEST(PoseTest, Test8Point)
{
    sfm::Eight2DPoints p1, p2;
    fill_golden_correspondences(p1, p2);

    /* The normalized 8-point algorithm (Hartley, Zisserman, 11.2):
     * - Point set normalization (scaling, offset)
     * - Matrix computation
     * - Rank constraint enforcement
     * - De-normalization of matrix
     */
    math::Matrix<double, 3, 3> T1, T2;
    sfm::pose_find_normalization(p1, &T1);
    sfm::pose_find_normalization(p2, &T2);
    p1 = T1 * p1;
    p2 = T2 * p2;
    sfm::FundamentalMatrix F(0.0);
    sfm::pose_8_point(p1, p2, &F);
    sfm::enforce_fundamental_constraints(&F);
    F = T2.transposed() * F * T1;

    /* Correct solution (computed with Matlab). */
    sfm::FundamentalMatrix F2;
    F2(0,0) = 0.000000014805557;  F2(0,1) = 0.000002197550186;  F2(0,2) = 0.001632934316777;
    F2(1,0) = -0.000002283909471; F2(1,1) = -0.000001354336179; F2(1,2) = 0.008734421917905;
    F2(2,0) = -0.001472308151103; F2(2,1) = -0.008375559378962; F2(2,2) = -0.160734037191207;

    // Force Fundamental matrices to the same scale.
    F /= F(2,2);
    F2 /= F2(2,2);

    //std::cout << "Fundamental:" << std::endl << F << std::endl;
    //std::cout << "Fundamental 2: " << std::endl << F2 << std::endl;
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR((F[i] - F2[i]) / (F[i] + F2[i]), 0.0, 0.05);
}

namespace {

    void
    fill_ground_truth_pose (sfm::CameraPose* pose1, sfm::CameraPose* pose2)
    {
        // Calibration with focal lenght 1 and 800x600 image.
        // The first camera looks straight along the z axis.
        pose1->K.fill(0.0);
        pose1->K(0,0) = 800.0; pose1->K(1,1) = 800.0;
        pose1->K(0,2) = 800.0 / 2.0; pose1->K(1,2) = 600.0 / 2.0;
        pose1->K(2,2) = 1.0;
        math::matrix_set_identity(*pose1->R, 3);
        pose1->t.fill(0.0);

        // The second camera is at (1,0,0) and rotated 45deg to the left.
        pose2->K.fill(0.0);
        pose2->K(0,0) = 800.0; pose2->K(1,1) = 800.0;
        pose2->K(0,2) = 800.0 / 2.0; pose2->K(1,2) = 600.0 / 2.0;
        pose2->K(2,2) = 1.0;
        pose2->R.fill(0.0);
        double const angle = MATH_PI / 4;
        pose2->R(0,0) = std::cos(angle); pose2->R(0,2) = std::sin(angle);
        pose2->R(1,1) = 1.0;
        pose2->R(2,0) = -std::sin(angle); pose2->R(2,2) = std::cos(angle);
        pose2->t.fill(0.0); pose2->t[0] = 1.0;
        pose2->t = pose2->R * -pose2->t;
    }

} // namespace

TEST(PoseTest, SyntheticPoseTest1)
{
    // This test computes from a given pose the fundamental matrix,
    // then the essential matrix, then recovers the original pose.
    sfm::CameraPose pose1, pose2;
    fill_ground_truth_pose(&pose1, &pose2);

    // Compute fundamental for pose.
    sfm::FundamentalMatrix F;
    sfm::fundamental_from_pose(pose1, pose2, &F);
    // Compute essential from fundamental.
    sfm::EssentialMatrix E = pose2.K.transposed() * F * pose1.K;
    // Compute pose from essential.
    std::vector<sfm::CameraPose> poses;
    pose_from_essential(E, &poses);
    // Check if one of the poses is the solution.
    int num_equal_cameras = 0;
    for (std::size_t i = 0; i < poses.size(); ++i)
    {
        //std::cout << "Determinant: " << math::matrix_determinant(poses[i].R) << std::endl;
        bool equal = poses[i].R.is_similar(pose2.R, 1e-14)
            && poses[i].t.is_similar(pose2.t, 1e-14);
        num_equal_cameras += equal;
    }
    EXPECT_EQ(num_equal_cameras, 1);
}

TEST(PoseTest, SyntheticPoseTest2)
{
    // This test computes from a given pose eight corresponding pairs
    // of 2D projections in the images. These correspondences are used
    // to compute a fundamental matrix, then the essential matrix, then
    // recovers the original pose.
    sfm::CameraPose pose1, pose2;
    fill_ground_truth_pose(&pose1, &pose2);

    // Eight "random" 3D points.
    std::vector<math::Vec3d> points3d;
    points3d.push_back(math::Vec3d(-0.31, -0.42, 1.41));
    points3d.push_back(math::Vec3d( 0.04,  0.01, 0.82));
    points3d.push_back(math::Vec3d(-0.25, -0.24, 1.25));
    points3d.push_back(math::Vec3d( 0.47,  0.22, 0.66));
    points3d.push_back(math::Vec3d( 0.13,  0.03, 0.89));
    points3d.push_back(math::Vec3d(-0.13, -0.46, 1.15));
    points3d.push_back(math::Vec3d( 0.21, -0.23, 1.33));
    points3d.push_back(math::Vec3d(-0.42,  0.38, 0.62));

    // Re-project in images using ground truth pose.
    math::Matrix<double, 3, 8> points2d_v1, points2d_v2;
    for (int i = 0; i < 8; ++i)
    {
        math::Vec3d p1 = pose1.K * (pose1.R * points3d[i] + pose1.t);
        math::Vec3d p2 = pose2.K * (pose2.R * points3d[i] + pose2.t);
        p1 /= p1[2];
        p2 /= p2[2];
        for (int j = 0; j < 3; ++j)
        {
            points2d_v1(j, i) = p1[j];
            points2d_v2(j, i) = p2[j];
        }
    }

    // Compute fundamental using stabilized 8-point algorithm.
    math::Matrix<double, 3, 3> T1, T2;
    sfm::pose_find_normalization(points2d_v1, &T1);
    sfm::pose_find_normalization(points2d_v2, &T2);
    points2d_v1 = T1 * points2d_v1;
    points2d_v2 = T2 * points2d_v2;
    sfm::FundamentalMatrix F;
    sfm::pose_8_point(points2d_v1, points2d_v2, &F);
    sfm::enforce_fundamental_constraints(&F);
    F = T2.transposed() * F * T1;
    // Compute essential from fundamental.
    sfm::EssentialMatrix E = pose2.K.transposed() * F * pose1.K;
    // Compute pose from essential.
    std::vector<sfm::CameraPose> poses;
    pose_from_essential(E, &poses);
    // Check if one of the poses is the solution.
    int num_equal_cameras = 0;
    for (std::size_t i = 0; i < poses.size(); ++i)
    {
        bool equal = poses[i].R.is_similar(pose2.R, 1e-14)
            && poses[i].t.is_similar(pose2.t, 1e-14);
        num_equal_cameras += equal;
    }
    EXPECT_EQ(num_equal_cameras, 1);
}

TEST(PoseRansacTest, TestRansac1)
{
    // TODO
}
