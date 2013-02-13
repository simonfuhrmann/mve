/*
 * Pose estimation from matches between two views in a RANSAC framework.
 * Written by Simon Fuhrmann.
 *
 * Questions:
 * - What is a good threshold value for the inlier test?
 */

#ifndef SFM_POSE_RANSAC_HEADER
#define SFM_POSE_RANSAC_HEADER

#include "defines.h"
#include "pose.h"

SFM_NAMESPACE_BEGIN

/**
 * RANSAC pose estimation from noisy image correspondences.
 *
 * The fundamental matrix for two views is to be determined from a set
 * of image correspondences contaminated with outliers. The algorithm
 * randomly selects N image correspondences (where N depends on the pose
 * algorithm) to estimate a fundamental matrix. Running for a number of
 * iterations, the fundamental matrix supporting the most matches is
 * returned as result.
 */
class PoseRansac
{
public:
    struct Options
    {
        int max_iterations;
        double threshold;
    };

    struct Result
    {
        FundamentalMatrix fundamental;
        std::vector<int> inliers;
    };

    struct Match
    {
        double p1[2];
        double p2[2];
    };

public:
    PoseRansac (Options const& options);
    void estimate (std::vector<Match> const& matches, Result* result);

private:
    void estimate_8_point (std::vector<Match> const& matches,
        FundamentalMatrix* fundamental);
    void find_inliers (std::vector<Match> const& matches,
        FundamentalMatrix const& fundamental, std::vector<int>* result);
    double sampson_distance (FundamentalMatrix const& fundamental,
        Match const& match);
private:
    Options opts;
};

SFM_NAMESPACE_END

#endif /* SFM_POSE_RANSAC_HEADER */
