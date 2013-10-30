/*
 * Pose estimation from matches between two views in a RANSAC framework.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_POSE_RANSAC_HEADER
#define SFM_POSE_RANSAC_HEADER

#include "math/matrix.h"
#include "sfm/defines.h"
#include "sfm/correspondence.h"
#include "sfm/fundamental.h"

SFM_NAMESPACE_BEGIN

/**
 * RANSAC pose estimation from noisy 2D-2D image correspondences.
 *
 * The fundamental matrix for two views is to be determined from a set
 * of image correspondences contaminated with outliers. The algorithm
 * randomly selects N image correspondences (where N depends on the pose
 * algorithm) to estimate a fundamental matrix. Running for a number of
 * iterations, the fundamental matrix supporting the most matches is
 * returned as result.
 */
class PoseRansac2D2D
{
public:
    struct Options
    {
        Options (void);

        /**
         * The number of RANSAC iterations. Defaults to 1000.
         * Function compute_ransac_iterations() can be used to estimate the
         * required number of iterations for a certain RANSAC success rate.
         */
        int max_iterations;

        /**
         * Threshold used to determine inliers. Defaults to 0.001.
         * This threshold depends on whether the input points are normalized.
         * In the normalized case, this threshold is relative to the image
         * dimension, e.g. 1e-3. In the un-normalized case, this threshold
         * is in pixels, e.g. 2.0.
         * TODO: Test the threshold in the un-normalized case (seems small).
         */
        double threshold;

        /**
         * Whether the input points are already normalized. Defaults to true.
         * If this is set to false, in every RANSAC iteration the coordinates
         * of the randomly selected matches will be normalized for 8-point.
         */
        bool already_normalized;
    };

    struct Result
    {
        /**
         * The resulting fundamental matrix which led to the inliers.
         * This is NOT the re-computed matrix from the inliers.
         */
        FundamentalMatrix fundamental;
        std::vector<int> inliers;
    };

public:
    explicit PoseRansac2D2D (Options const& options);
    void estimate (Correspondences const& matches, Result* result);

private:
    void estimate_8_point (Correspondences const& matches,
        FundamentalMatrix* fundamental);
    void find_inliers (Correspondences const& matches,
        FundamentalMatrix const& fundamental, std::vector<int>* result);

private:
    Options opts;
};

/* ---------------------------------------------------------------- */

/**
 * RANSAC pose estimation from noisy 2D-3D image correspondences.
 *
 * The pose of a new view is to be determined from a set of point to
 * image correspondences contaminated with outliers. The algorithm
 * randomly selects N correspondences (where N depends on the pose
 * algorithm) to estimate the pose. Running for a number of iterations,
 * the pose supporting the most matches is returned as result.
 */
class PoseRansac2D3D
{
public:
    struct Options
    {
        Options (void);

        /**
         * The number of RANSAC iterations. Defaults to 100.
         * Function compute_ransac_iterations() can be used to estimate the
         * required number of iterations for a certain RANSAC success rate.
         */
        int max_iterations;

        /**
         * Threshold used to determine inliers. Defaults to 0.001.
         */
        double threshold;
    };

    struct Result
    {
        /**
         * The resulting P-matrix which led to the inliers.
         * This is NOT the re-computed matrix from the inliers.
         */
        math::Matrix<double, 3, 4> p_matrix;
        std::vector<int> inliers;
    };

public:
    explicit PoseRansac2D3D (Options const& options);
    void estimate (Correspondences2D3D const& corresp, Result* result);

private:
    void estimate_6_point (Correspondences2D3D const& corresp,
        math::Matrix<double, 3, 4>* p_matrix);
    void find_inliers (Correspondences2D3D const& corresp,
        math::Matrix<double, 3, 4> const& pose,
        std::vector<int>* result);

private:
    Options opts;
};

/* ---------------------------------------------------------------- */

/**
 * The function returns the required number of iterations for a desired
 * RANSAC success rate. If w is the probability of choosing one good sample
 * (the inlier ratio), then w^n is the probability that all n samples are
 * inliers. Then k is the number of iterations required to draw only inliers
 * with a certain probability of success, p:
 *
 *          log(1 - p)
 *     k = ------------
 *         log(1 - w^n)
 *
 * Example: For w = 50%, p = 99%, n = 8: k = log(0.001) / log(0.99609) = 1176.
 * Thus, it requires 1176 iterations for RANSAC to succeed with a 99% chance.
 */
int
compute_ransac_iterations (double inlier_ratio,
    int num_samples,
    double desired_success_rate = 0.99);

SFM_NAMESPACE_END

#endif /* SFM_POSE_RANSAC_HEADER */
