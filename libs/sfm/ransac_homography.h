/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

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
         * Threshold used to determine inliers. Defaults to 0.005.
         * This threshold assumes that the input points are normalized.
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
    void estimate (Correspondences2D2D const& matches, Result* result);

private:
    void compute_homography (Correspondences2D2D const& matches,
        HomographyMatrix* homography);
    void evaluate_homography (Correspondences2D2D const& matches,
        HomographyMatrix const& homography, std::vector<int>* inliers);

private:
    Options opts;
};

/* ------------------------ Implementation ------------------------ */

inline
RansacHomography::Options::Options (void)
    : max_iterations(1000)
    , threshold(0.005)
    , verbose_output(false)
{
}

SFM_NAMESPACE_END

#endif /* SFM_RANSAC_HOMOGRAPHY_HEADER */
