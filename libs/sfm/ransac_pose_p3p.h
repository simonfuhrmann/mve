/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_RANSAC_POSE_P3P_HEADER
#define SFM_RANSAC_POSE_P3P_HEADER

#include <vector>

#include "math/matrix.h"
#include "math/vector.h"
#include "sfm/correspondence.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN

/**
 * RANSAC pose estimation from 2D-3D correspondences and known camera
 * calibration using the perspective 3-point (P3P) algorithm.
 *
 * The rotation and translation of a camera is determined from a set of
 * 2D image to 3D point correspondences contaminated with outliers. The
 * algorithm iteratively selects 3 random correspondences and returns the
 * result which led to the most inliers.
 *
 * The input 2D image coordinates, the input K-matrix and the threshold
 * in the options must be consistent. For example, if the 2D image coordinates
 * are NOT normalized but in pixel coordinates, the K-matrix must be in pixel
 * coordinates and the threshold must be in pixel, too. For this algorithm,
 * image coordinates do not need to be normalized for stability.
 */
class RansacPoseP3P
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
        /** The pose [R|t] which led to the inliers. */
        math::Matrix<double, 3, 4> pose;

        /** The correspondence indices which led to the result. */
        std::vector<int> inliers;
    };

public:
    explicit RansacPoseP3P (Options const& options);

    void estimate (Correspondences2D3D const& corresp,
        math::Matrix<double, 3, 3> const& k_matrix,
        Result* result);

private:
    typedef math::Matrix<double, 3, 4> Pose;
    typedef std::vector<Pose> PutativePoses;

private:
    void compute_p3p (Correspondences2D3D const& corresp,
        math::Matrix<double, 3, 3> const& inv_k_matrix,
        PutativePoses* poses);

    void find_inliers (Correspondences2D3D const& corresp,
        math::Matrix<double, 3, 3> const& k_matrix,
        Pose const& pose, std::vector<int>* inliers);

private:
    Options opts;
};

/* ------------------------ Implementation ------------------------ */

inline
RansacPoseP3P::Options::Options (void)
    : max_iterations(1000)
    , threshold(0.005)
    , verbose_output(false)
{
}

SFM_NAMESPACE_END

#endif /* SFM_RANSAC_POSE_P3P_HEADER */
