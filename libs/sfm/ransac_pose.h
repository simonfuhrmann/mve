/*
 * Pose estimation from matches between two views in a RANSAC framework.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_RANSAC_POSE_HEADER
#define SFM_RANSAC_POSE_HEADER

#include "math/matrix.h"
#include "sfm/defines.h"
#include "sfm/correspondence.h"
#include "sfm/fundamental.h"

SFM_NAMESPACE_BEGIN

/**
 * RANSAC pose estimation from noisy 2D-3D image correspondences.
 *
 * The pose of a new view is to be determined from a set of point to
 * image correspondences contaminated with outliers. The algorithm
 * randomly selects N correspondences (where N depends on the pose
 * algorithm) to estimate the pose. Running for a number of iterations,
 * the pose supporting the most matches is returned as result.
 */
class RansacPose
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

        /**
         * Produce status messages on the console.
         */
        bool verbose_output;
    };

    struct Result
    {
        /**
         * The resulting P-matrix which led to the inliers.
         * This is NOT the re-computed matrix from the inliers.
         */
        math::Matrix<double, 3, 4> p_matrix;

        /**
         * The indices of inliers in the correspondences which led to the
         * homography matrix.
         */
        std::vector<int> inliers;
    };

public:
    explicit RansacPose (Options const& options);
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

#endif /* SFM_RANSAC_POSE_HEADER */
