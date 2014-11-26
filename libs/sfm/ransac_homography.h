#ifndef SFM_RANSAC_HOMOGRAPHY_HEADER
#define SFM_RANSAC_HOMOGRAPHY_HEADER

#include <vector>

#include "sfm/homography.h"

SFM_NAMESPACE_BEGIN

/**
 * RANSAC homography estimation from noisy 2D-2D image correspondences.
 *
 * The homography matrix for two views is to be determined from a set of image
 * correspondences contaminated with outliers. The algorithm randomly selects 4
 * image correspondences to estimate a homography matrix. Running for a number
 * of iterations, the homography matrix supporting the most matches returned.
 */
class RansacHomography
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
         * Threshold used to determine inliers. Defaults to 0.01.
         * This threshold depends on whether the input points are normalized.
         * In the normalized case, this threshold is relative to the image
         * dimension, e.g. 1e-3. In the un-normalized case, this threshold
         * is in pixels, e.g. 2.0.
         */
        double threshold;

        /**
         * Whether the input points are already normalized. Defaults to true.
         * If this is set to false, in every RANSAC iteration the coordinates
         * of the randomly selected matches will be normalized for homography
         * DLT algorithm.
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
         * The resulting homography matrix which led to the inliers.
         * This is NOT the re-computed matrix from the inliers.
         */
        HomographyMatrix homography;

        /**
         * The indices of inliers in the correspondences which led to the
         * homography matrix.
         */
        std::vector<int> inliers;
    };

public:
    explicit RansacHomography (Options const& options);
    void estimate (Correspondences const& matches, Result* result);

private:
    void compute_homography (Correspondences const& matches,
        HomographyMatrix* homography);
    void evaluate_homography (Correspondences const& matches,
        HomographyMatrix const& homography, std::vector<int>* inliers);

private:
    Options opts;
};

/* ------------------------ Implementation ------------------------ */

inline
RansacHomography::Options::Options (void)
    : max_iterations(1000)
    , threshold(0.01)
    , already_normalized(true)
    , verbose_output(false)
{
}

SFM_NAMESPACE_END

#endif /* SFM_RANSAC_HOMOGRAPHY_HEADER */
