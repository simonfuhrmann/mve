/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_BUNDLER_MATCHING_HEADER
#define SFM_BUNDLER_MATCHING_HEADER

#include <memory>
#include <stdexcept>
#include <vector>
#include <string>
#include <sstream>

#include "sfm/ransac_fundamental.h"
#include "sfm/bundler_common.h"
#include "sfm/defines.h"
#include "sfm/matching_base.h"

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
    enum MatcherType
    {
        MATCHER_EXHAUSTIVE,
        MATCHER_CASCADE_HASHING
    };

    /** Options for feature matching. */
    struct Options
    {
        /** Options for RANSAC computation of the fundamental matrix. */
        sfm::RansacFundamental::Options ransac_opts;
        /** Minimum number of matching features before RANSAC. */
        int min_feature_matches = 24;
        /** Minimum number of matching features after RANSAC. */
        int min_matching_inliers = 12;
        /** Perform low-resolution matching to reject unlikely pairs. */
        bool use_lowres_matching = false;
        /** Number of features used for low-res matching. */
        int num_lowres_features = 500;
        /** Minimum number of matches from low-res matching. */
        int min_lowres_matches = 5;
        /** Only match to a few previous frames. Disabled by default. */
        int match_num_previous_frames = 0;
        /** Matcher type. Exhaustive by default. */
        MatcherType matcher_type = MATCHER_EXHAUSTIVE;
    };

    struct Progress
    {
        std::size_t num_total;
        std::size_t num_done;
    };

public:
    Matching (Options const& options, Progress* progress = nullptr);

    /**
     * Initialize matching by passing features to the matcher for
     * preprocessing.
     */
    void init (ViewportList* viewports);

    /**
     * Computes the pairwise matching between all pairs of views.
     * Computation requires both descriptor data and 2D feature positions
     * in the viewports.
     */
    void compute (PairwiseMatching* pairwise_matching);

private:
    void two_view_matching (int view_1_id, int view_2_id,
        CorrespondenceIndices* matches, std::stringstream& message);

private:
    Options opts;
    Progress* progress;
    std::unique_ptr<MatchingBase> matcher;
    ViewportList const* viewports;
};

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

#endif /* SFM_BUNDLER_MATCHING_HEADER */
