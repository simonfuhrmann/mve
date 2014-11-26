/*
 * Pose estimation from matches between two views in a RANSAC framework.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_RANSAC_FUNDAMENTAL_HEADER
#define SFM_RANSAC_FUNDAMENTAL_HEADER

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
class RansacFundamental
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

        /**
         * Produce status messages on the console.
         */
        bool verbose_output;
    };

    struct Result
    {
        /**
         * The resulting fundamental matrix which led to the inliers.
         * This is NOT the re-computed matrix from the inliers.
         */
        FundamentalMatrix fundamental;

        /**
         * The indices of inliers in the correspondences which led to the
         * homography matrix.
         */
        std::vector<int> inliers;
    };

public:
    explicit RansacFundamental (Options const& options);
    void estimate (Correspondences const& matches, Result* result);

private:
    void estimate_8_point (Correspondences const& matches,
        FundamentalMatrix* fundamental);
    void find_inliers (Correspondences const& matches,
        FundamentalMatrix const& fundamental, std::vector<int>* result);

private:
    Options opts;
};

/* ------------------------ Implementation ------------------------ */

inline
RansacFundamental::Options::Options (void)
    : max_iterations(1000)
    , threshold(1e-3)
    , already_normalized(true)
    , verbose_output(false)
{
}

SFM_NAMESPACE_END

#endif /* SFM_RANSAC_FUNDAMENTAL_HEADER */
