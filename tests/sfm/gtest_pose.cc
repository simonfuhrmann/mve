// Test cases for pose estimation.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "math/matrix_tools.h"
#include "math/matrix_svd.h"
#include "sfm/camera_pose.h"
#include "sfm/fundamental.h"
#include "sfm/ransac.h"
#include "sfm/ransac_fundamental.h"
#include "sfm/triangulate.h"
#include "sfm/correspondence.h"

TEST(PoseTest, PointNormalization1)
{
    // Normalization of 3 points.
    math::Matrix<float, 3, 3> set;
    set(0,0) = 5.0f; set(1,0) = 5.0f; set(2,0) = 1.0f;
    set(0,1) = -5.0f; set(1,1) = -1.0f; set(2,1) = 1.0f;
    set(0,2) = 0.0f; set(1,2) = 0.0f; set(2,2) = 1.0f;
    math::Matrix<float, 3, 3> trans;
    sfm::compute_normalization(set, &trans);
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
    sfm::compute_normalization(set, &trans);
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

namespace
{
    void
    fill_golden_correspondences(sfm::Eight2DPoints& p1,
        sfm::Eight2DPoints& p2, sfm::FundamentalMatrix& F)
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

        F(0,0) = 0.000000014805557;
        F(0,1) = 0.000002197550186;
        F(0,2) = 0.001632934316777;
        F(1,0) = -0.000002283909471;
        F(1,1) = -0.000001354336179;
        F(1,2) = 0.008734421917905;
        F(2,0) = -0.001472308151103;
        F(2,1) = -0.008375559378962;
        F(2,2) = -0.160734037191207;
    }

}  // namespace

TEST(PoseTest, Test8Point)
{
    /* Obtain golden correspondences and correct solution from Matlab. */
    sfm::Eight2DPoints p1, p2;
    sfm::FundamentalMatrix F2;
    fill_golden_correspondences(p1, p2, F2);

    /* The normalized 8-point algorithm (Hartley, Zisserman, 11.2):
     * - Point set normalization (scaling, offset)
     * - Matrix computation
     * - Rank constraint enforcement
     * - De-normalization of matrix
     */
    math::Matrix<double, 3, 3> T1, T2;
    sfm::compute_normalization(p1, &T1);
    sfm::compute_normalization(p2, &T2);
    p1 = T1 * p1;
    p2 = T2 * p2;
    sfm::FundamentalMatrix F(0.0);
    sfm::fundamental_8_point(p1, p2, &F);
    sfm::enforce_fundamental_constraints(&F);
    F = T2.transposed() * F * T1;

    // Force Fundamental matrices to the same scale.
    F /= F(2,2);
    F2 /= F2(2,2);
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR((F[i] - F2[i]) / (F[i] + F2[i]), 0.0, 0.05);
}

TEST(PoseTest, TestLeastSquaresPose)
{
    /* Obtain golden correspondences and correct solution from Matlab. */
    sfm::Eight2DPoints p1, p2;
    sfm::FundamentalMatrix F2;
    fill_golden_correspondences(p1, p2, F2);

    math::Matrix<double, 3, 3> T1, T2;
    sfm::compute_normalization(p1, &T1);
    sfm::compute_normalization(p2, &T2);
    p1 = T1 * p1;
    p2 = T2 * p2;

    sfm::Correspondences2D2D points(8);
    for (int i = 0; i < 8; ++i)
    {
        points[i].p1[0] = p1(0, i);
        points[i].p1[1] = p1(1, i);
        points[i].p2[0] = p2(0, i);
        points[i].p2[1] = p2(1, i);
    }

    sfm::FundamentalMatrix F(0.0);
    sfm::fundamental_least_squares(points, &F);
    sfm::enforce_fundamental_constraints(&F);
    F = T2.transposed() * F * T1;

    // Force Fundamental matrices to the same scale.
    F /= F(2,2);
    F2 /= F2(2,2);
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR((F[i] - F2[i]) / (F[i] + F2[i]), 0.0, 0.1);
}

namespace
{

    void
    fill_ground_truth_pose (sfm::CameraPose* pose1, sfm::CameraPose* pose2)
    {
        // Calibration with focal lenght 1 and 800x600 image.
        // The first camera looks straight along the z axis.
        if (pose1 != nullptr)
        {
            pose1->set_k_matrix(800, 800 / 2, 600 / 2);
            math::matrix_set_identity(*pose1->R, 3);
            pose1->t.fill(0.0);
        }

        // The second camera is at (1,0,0) and rotated 45deg to the left.
        if (pose2 != nullptr)
        {
            pose2->set_k_matrix(800, 800 / 2, 600 / 2);
            pose2->R.fill(0.0);
            double const angle = MATH_PI / 4;
            pose2->R(0,0) = std::cos(angle); pose2->R(0,2) = std::sin(angle);
            pose2->R(1,1) = 1.0;
            pose2->R(2,0) = -std::sin(angle); pose2->R(2,2) = std::cos(angle);
            pose2->t.fill(0.0); pose2->t[0] = 1.0;
            pose2->t = pose2->R * -pose2->t;
        }
    }

    void
    fill_eight_random_points (std::vector<math::Vec3d>* points)
    {
        points->push_back(math::Vec3d(-0.31, -0.42, 1.41));
        points->push_back(math::Vec3d( 0.04,  0.01, 0.82));
        points->push_back(math::Vec3d(-0.25, -0.24, 1.25));
        points->push_back(math::Vec3d( 0.47,  0.22, 0.66));
        points->push_back(math::Vec3d( 0.13,  0.03, 0.89));
        points->push_back(math::Vec3d(-0.13, -0.46, 1.15));
        points->push_back(math::Vec3d( 0.21, -0.23, 1.33));
        points->push_back(math::Vec3d(-0.42,  0.38, 0.62));
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
    fill_eight_random_points(&points3d);

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

    // Compute fundamental using normalized 8-point algorithm.
    math::Matrix<double, 3, 3> T1, T2;
    sfm::compute_normalization(points2d_v1, &T1);
    sfm::compute_normalization(points2d_v2, &T2);
    points2d_v1 = T1 * points2d_v1;
    points2d_v2 = T2 * points2d_v2;
    sfm::FundamentalMatrix F;
    sfm::fundamental_8_point(points2d_v1, points2d_v2, &F);
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
        bool equal = poses[i].R.is_similar(pose2.R, 1e-13)
            && poses[i].t.is_similar(pose2.t, 1e-13);
        num_equal_cameras += equal;
    }
    EXPECT_EQ(num_equal_cameras, 1);
}

TEST(PostTest, TriangulateTest1)
{
    // Fill the ground truth pose.
    sfm::CameraPose pose1, pose2;
    fill_ground_truth_pose(&pose1, &pose2);

    math::Vec3d x_gt(0.0, 0.0, 1.0);
    math::Vec3d x1 = pose1.K * (pose1.R * x_gt + pose1.t);
    math::Vec3d x2 = pose2.K * (pose2.R * x_gt + pose2.t);

    sfm::Correspondence2D2D match;
    match.p1[0] = x1[0] / x1[2];
    match.p1[1] = x1[1] / x1[2];
    match.p2[0] = x2[0] / x2[2];
    match.p2[1] = x2[1] / x2[2];

    math::Vec3d x = sfm::triangulate_match(match, pose1, pose2);
    EXPECT_NEAR(x[0], x_gt[0], 1e-14);
    EXPECT_NEAR(x[1], x_gt[1], 1e-14);
    EXPECT_NEAR(x[2], x_gt[2], 1e-14);
    EXPECT_TRUE(sfm::is_consistent_pose(match, pose1, pose2));
}

TEST(PoseTest, ComputeRANSACIterations)
{
    double inlier_ratio = 0.5;
    double success_rate = 0.99;
    int num_samples = 8;
    EXPECT_EQ(1177, sfm::compute_ransac_iterations(
        inlier_ratio, num_samples, success_rate));
}

#if 0 // This test case is disabled because it is not deterministic.
TEST(PoseRansacFundamental, TestRansac1)
{
    // This test computes from a given pose eight corresponding pairs
    // of 2D projections in the images. These correspondences are used
    // to compute a fundamental matrix, then the essential matrix, then
    // recovers the original pose.
    sfm::CameraPose pose1, pose2;
    fill_ground_truth_pose(&pose1, &pose2);

    // Some "random" 3D points.
    std::vector<math::Vec3d> points3d;
    points3d.push_back(math::Vec3d(-0.31, -0.42, 1.41));
    points3d.push_back(math::Vec3d( 0.04,  0.01, 0.82));
    points3d.push_back(math::Vec3d(-0.25, -0.24, 1.25));
    points3d.push_back(math::Vec3d( 0.47,  0.22, 0.66));
    points3d.push_back(math::Vec3d( 0.13,  0.03, 0.89));
    points3d.push_back(math::Vec3d(-0.13, -0.46, 1.15));
    points3d.push_back(math::Vec3d( 0.21, -0.23, 1.33));
    points3d.push_back(math::Vec3d(-0.42,  0.38, 0.62));
    points3d.push_back(math::Vec3d(-0.22, -0.38, 0.52));
    //points3d.push_back(math::Vec3d( 0.15,  0.12, 1.12));

    // Re-project in images using ground truth pose.
    sfm::Correspondences matches;
    for (int i = 0; i < points3d.size(); ++i)
    {
        math::Vec3d p1 = pose1.K * (pose1.R * points3d[i] + pose1.t);
        math::Vec3d p2 = pose2.K * (pose2.R * points3d[i] + pose2.t);
        p1 /= p1[2];
        p2 /= p2[2];

        sfm::Correspondence match;
        match.p1[0] = p1[0] / p1[2];
        match.p1[1] = p1[1] / p1[2];
        match.p2[0] = p2[0] / p2[2];
        match.p2[1] = p2[1] / p2[2];
        matches.push_back(match);
    }
    matches[points3d.size()-1].p2[0] -= 15.0;
    matches[points3d.size()-1].p2[1] += 33.0;
    //matches[points3d.size()-2].p1[0] += 25.0;
    //matches[points3d.size()-2].p1[1] -= 13.0;

    sfm::RansacFundamental::Options opts;
    opts.max_iterations = 50;
    opts.threshold = 1.0;
    opts.already_normalized = false;
    sfm::RansacFundamental ransac(opts);
    sfm::RansacFundamental::Result result;
    std::srand(std::time(0));
    ransac.estimate(matches, &result);

    EXPECT_EQ(8, result.inliers.size());
}
#endif
