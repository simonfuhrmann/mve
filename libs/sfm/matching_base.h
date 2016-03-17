/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_MATCHING_BASE_HEADER
#define SFM_MATCHING_BASE_HEADER

#include <limits>

#include "sfm/bundler_common.h"
#include "sfm/defines.h"
#include "sfm/matching.h"
#include "sfm/matching_base.h"

SFM_NAMESPACE_BEGIN

class MatchingBase
{
public:
    struct Options
    {
        Matching::Options sift_matching_opts{ 128, 0.8f,
            std::numeric_limits<float>::max() };
        Matching::Options surf_matching_opts{ 64, 0.7f,
            std::numeric_limits<float>::max() };
    };

    virtual ~MatchingBase (void) = default;

    /**
     * Initialize the matcher. This is used for preprocessing the features
     * of the given viewports. For example, in the exhaustive matcher the
     * features are discretized.
     */
    virtual void init (bundler::ViewportList* viewports) = 0;

    /** Matches all feature types yielding a single matching result. */
    virtual void pairwise_match (int view_1_id, int view_2_id,
        Matching::Result* result) const = 0;

    /**
     * Matches the N lowest resolution features and returns the number of
     * matches. Can be used as a guess for full matchability. Useful values
     * are at most 3 matches for 500 features, or 2 matches with 300 features.
     */
    virtual int pairwise_match_lowres (int view_1_id, int view_2_id,
        std::size_t num_features) const = 0;

    Options opts;
};

SFM_NAMESPACE_END

#endif /* SFM_MATCHING_BASE_HEADER */

