/*
 * Pose estimation from matches between two views in a RANSAC framework.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_POSE_RANSAC_HEADER
#define SFM_POSE_RANSAC_HEADER

#include "math/matrix.h"

#include "defines.h"
#include "correspondence.h"
#include "fundamental.h"

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
         * The number of RANSAC iterations. Defaults to 100.
         * TODO: Document reasonable default values and RANSAC theory.
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
    PoseRansac2D2D (Options const& options);
    void estimate (Correspondences const& matches, Result* result);

private:
    void estimate_8_point (Correspondences const& matches,
        FundamentalMatrix* fundamental);
    void find_inliers (Correspondences const& matches,
        FundamentalMatrix const& fundamental, std::vector<int>* result);
    double sampson_distance (FundamentalMatrix const& fundamental,
        Correspondence const& match);
private:
    Options opts;
};

/* ---------------------------------------------------------------- */

class PoseRansac2D3D
{
public:
    struct Options
    {
        Options (void);

        /**
         * The number of RANSAC iterations. Defaults to 100.
         * TODO: Document reasonable default values and RANSAC theory.
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
    PoseRansac2D3D (Options const& options);
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

SFM_NAMESPACE_END

#endif /* SFM_POSE_RANSAC_HEADER */
