// Test cases for pose estimation.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "math/matrix_tools.h"
#include "math/matrix_svd.h"
#include "sfm/pose.h"
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

TEST(PoseTest, CorrespondencesNormalization)
{
    sfm::Correspondences c(3);
    c[0].p1[0] = 1.0; c[0].p1[1] = 1.0;
    c[1].p1[0] = 3.0; c[1].p1[1] = -2.0;
    c[2].p1[0] = 2.0; c[2].p1[1] = 4.0;

    c[0].p2[0] = 1.0; c[0].p2[1] = -1.0;
    c[1].p2[0] = 3.0; c[1].p2[1] = -2.0;
    c[2].p2[0] = -2.0; c[2].p2[1] = -4.0;

    math::Matrix<double, 3, 3> transform1, transform2;
    sfm::compute_normalization(c, &transform1, &transform2);

    EXPECT_NEAR(transform1[0], 1.0 / 6.0, 1e-10);
    EXPECT_NEAR(transform1[1], 0.0, 1e-10);
    EXPECT_NEAR(transform1[2], -2.0 / 6.0, 1e-10);
    EXPECT_NEAR(transform1[3], 0.0, 1e-10);
    EXPECT_NEAR(transform1[4], 1.0 / 6.0, 1e-10);
    EXPECT_NEAR(transform1[5], -1.0 / 6.0, 1e-10);
    EXPECT_NEAR(transform1[6], 0.0, 1e-10);
    EXPECT_NEAR(transform1[7], 0.0, 1e-10);
    EXPECT_NEAR(transform1[8], 1.0, 1e-10);

    EXPECT_NEAR(transform2[0], 1.0 / 5.0, 1e-10);
    EXPECT_NEAR(transform2[1], 0.0, 1e-10);
    EXPECT_NEAR(transform2[2], -2.0/3.0 / 5.0, 1e-10);
    EXPECT_NEAR(transform2[3], 0.0, 1e-10);
    EXPECT_NEAR(transform2[4], 1.0 / 5.0, 1e-10);
    EXPECT_NEAR(transform2[5], 7.0/3.0 / 5.0, 1e-10);
    EXPECT_NEAR(transform2[6], 0.0, 1e-10);
    EXPECT_NEAR(transform2[7], 0.0, 1e-10);
    EXPECT_NEAR(transform2[8], 1.0, 1e-10);
}

TEST(PoseTest, Correspondences2D3DNormalization)
{
    sfm::Correspondences2D3D c(2);
    c[0].p2d[0] = 1.0f; c[0].p2d[1] = 1.0f;
    c[1].p2d[0] = 3.0f; c[1].p2d[1] = -1.0f;

    c[0].p3d[0] = -5.0; c[0].p3d[1] = -2.0; c[0].p3d[2] = -2.0;
    c[1].p3d[0] = -1.0; c[1].p3d[1] = 8.0;  c[1].p3d[2] = 1.0;

    math::Matrix<double, 3, 3> T2D;
    math::Matrix<double, 4, 4> T3D;
    sfm::compute_normalization(c, &T2D, &T3D);

    double t2d_scale = 0.5;
    EXPECT_NEAR(T2D[0], t2d_scale, 1e-10);
    EXPECT_NEAR(T2D[1], 0.0, 1e-10);
    EXPECT_NEAR(T2D[2], -2.0 * t2d_scale, 1e-10);
    EXPECT_NEAR(T2D[3], 0.0, 1e-10);
    EXPECT_NEAR(T2D[4], t2d_scale, 1e-10);
    EXPECT_NEAR(T2D[5], -0.0 * t2d_scale, 1e-10);
    EXPECT_NEAR(T2D[6], 0.0, 1e-10);
    EXPECT_NEAR(T2D[7], 0.0, 1e-10);
    EXPECT_NEAR(T2D[8], 1.0, 1e-10);

    double t3d_scale = 0.1;
    EXPECT_NEAR(T3D[0], t3d_scale, 1e-10);
    EXPECT_NEAR(T3D[1], 0.0, 1e-10);
    EXPECT_NEAR(T3D[2], 0.0, 1e-10);
    EXPECT_NEAR(T3D[3], 3.0 * t3d_scale, 1e-10);
    EXPECT_NEAR(T3D[4], 0.0, 1e-10);
    EXPECT_NEAR(T3D[5], t3d_scale, 1e-10);
    EXPECT_NEAR(T3D[6], 0.0, 1e-10);
    EXPECT_NEAR(T3D[7], -3.0 * t3d_scale, 1e-10);
    EXPECT_NEAR(T3D[8], 0.0, 1e-10);
    EXPECT_NEAR(T3D[9], 0.0, 1e-10);
    EXPECT_NEAR(T3D[10], t3d_scale, 1e-10);
    EXPECT_NEAR(T3D[11], 0.5 * t3d_scale, 1e-10);
    EXPECT_NEAR(T3D[12], 0.0, 1e-10);
    EXPECT_NEAR(T3D[13], 0.0, 1e-10);
    EXPECT_NEAR(T3D[14], 0.0, 1e-10);
    EXPECT_NEAR(T3D[15], 1.0, 1e-10);
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

    sfm::Correspondences points(8);
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
        if (pose1 != NULL)
        {
            pose1->set_k_matrix(800, 800 / 2, 600 / 2);
            math::matrix_set_identity(*pose1->R, 3);
            pose1->t.fill(0.0);
        }

        // The second camera is at (1,0,0) and rotated 45deg to the left.
        if (pose2 != NULL)
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
        bool equal = poses[i].R.is_similar(pose2.R, 1e-14)
            && poses[i].t.is_similar(pose2.t, 1e-14);
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

    sfm::Correspondence match;
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

TEST(PoseTest, PoseFrom2D3DCorrespondences)
{
    sfm::CameraPose pose;
    fill_ground_truth_pose(NULL, &pose);
    math::Matrix<double, 3, 4> groundtruth_p_matrix;
    pose.fill_p_matrix(&groundtruth_p_matrix);

    // Get "random" points.
    std::vector<math::Vec3d> points;
    fill_eight_random_points(&points);

    // Project points to 2d coords and create correspondence.
    sfm::Correspondences2D3D corresp;
    for (int i = 0; i < 6; ++i)
    {
        math::Vec3f x = pose.K * (pose.R * points[i] + pose.t);
        x /= x[2];
        sfm::Correspondence2D3D c;
        std::copy(*points[i], *points[i] + 3, c.p3d);
        std::copy(*x, *x + 2, c.p2d);
        corresp.push_back(c);
    }

    // Apply normalization to the correspondences.
    // NOTE: It seems that normalizing the input points does not have a
    // notable effect on the accuracy of the resulting solution.
    math::Matrix<double, 3, 3> T2D;
    math::Matrix<double, 4, 4> T3D;
    sfm::compute_normalization(corresp, &T2D, &T3D);
    sfm::apply_normalization(T2D, T3D, &corresp);

    // Compute pose from correspondences.
    math::Matrix<double, 3, 4> estimated_p_matrix;
    sfm::pose_from_2d_3d_correspondences(corresp, &estimated_p_matrix);

    // Remove normalization from resulting P-matrix.
    estimated_p_matrix = math::matrix_inverse(T2D) * estimated_p_matrix * T3D;

    // Obtain camera pose decomposition from P-matrix.
    sfm::CameraPose new_pose;
    sfm::pose_from_p_matrix(estimated_p_matrix, &new_pose);

    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR(new_pose.K[i], pose.K[i], 0.015f) << "for i=" << i;
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR(new_pose.R[i], pose.R[i], 0.001f) << "for i=" << i;
    for (int i = 0; i < 3; ++i)
        EXPECT_NEAR(new_pose.t[i], pose.t[i], 0.001f) << "for i=" << i;
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
