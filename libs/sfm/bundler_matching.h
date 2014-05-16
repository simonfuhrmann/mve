/*
 * Bundler component that performs pairwise matching between all views.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_BUNDLER_MATCHING_HEADER
#define SFM_BUNDLER_MATCHING_HEADER

#include <vector>
#include <string>

#include "sfm/matching.h"
#include "sfm/ransac_fundamental.h"
#include "sfm/bundler_common.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

/**
 * Bundler Component: Matching between views in an MVE scene.
 *
 * For every view the feature embedding is loaded and matched to all other
 * views with smaller ID (since the matching is symmetric). Two-view matching
 * involves RANSAC to compute the fundamental matrix (geometric filtering).
 * Only views with a minimum number of matches are considered "connected".
 *
 * The global matching result can be saved to and loaded from file.
 * The file format is a binary sequence of numbers and IDs (all int32_t):
 *
 * <number of results>
 * <view ID 1> <view ID 2> <number of matches>
 *   <match 1 feature ID 1> <match 1 feature ID 2>
 *   <match 2 feature ID 1> <match 2 feature ID 2>
 *   ...
 * <view ID 3> <view ID 4> <number of matches>
 * ...
 *
 * Note:
 * - Only supports SIFT at the moment.
 */
class Matching
{
public:
    struct Options
    {
        Options (void);

        /** Two-view feature matching options. */
        sfm::Matching::Options matching_opts;
        /** Minimum number of matching features before RANSAC. */
        int min_feature_matches;

        /** Options for RANSAC computation of the fundamental matrix. */
        sfm::RansacFundamental::Options ransac_opts;
        /** Minimum number of matching features after RANSAC. */
        int min_matching_inliers;
    };

    struct Progress
    {
        std::size_t num_total;
        std::size_t num_done;
    };

public:
    Matching (Options const& options, Progress* progress = NULL);

    /**
     * Computes the pairwise matching between all pairs of views.
     * Computation requires both descriptor data and 2D feature positions
     * in the viewports.
     */
    void compute (ViewportList const& viewports,
        PairwiseMatching* pairwise_matching);

private:
    void two_view_matching (std::size_t id_1, std::size_t id_2,
        ViewportList const& viewports, PairwiseMatching* pairwise_matching);

private:
    Options opts;
    Progress* progress;
};

/* ------------------------ Implementation ------------------------ */

inline
Matching::Options::Options (void)
    : min_feature_matches(24)
    , min_matching_inliers(12)
{
}

inline
Matching::Matching (Options const& options, Progress* progress)
    : opts(options)
    , progress(progress)
{
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

#endif /* SFM_BUNDLER_MATCHING_HEADER */
