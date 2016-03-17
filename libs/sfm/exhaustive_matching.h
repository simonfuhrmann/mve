/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_EXHAUSTIVE_MATCHING_HEADER
#define SFM_EXHAUSTIVE_MATCHING_HEADER

#include "sfm/bundler_common.h"
#include "sfm/defines.h"
#include "sfm/matching_base.h"
#include "sfm/nearest_neighbor.h"
#include "sfm/sift.h"
#include "sfm/surf.h"

/* Whether to use floating point or 8-bit descriptors for matching. */
#define DISCRETIZE_DESCRIPTORS 1

SFM_NAMESPACE_BEGIN

class ExhaustiveMatching : public MatchingBase
{
public:
    ~ExhaustiveMatching (void) override = default;

    /** Initialize matcher by preprocessing given SIFT/SURF features. */
    void init (bundler::ViewportList* viewports) override;

    /** Matches all feature types yielding a single matching result. */
    void pairwise_match (int view_1_id, int view_2_id,
        Matching::Result* result) const override;

    /**
     * Matches the N lowest resolution features and returns the number of
     * matches. Can be used as a guess for full matchability. Useful values
     * are at most 3 matches for 500 features, or 2 matches with 300 features.
     */
    int pairwise_match_lowres (int view_1_id, int view_2_id,
        std::size_t num_features) const override;

protected:
#if DISCRETIZE_DESCRIPTORS
    typedef util::AlignedMemory<math::Vec128us, 16> SiftDescriptors;
    typedef util::AlignedMemory<math::Vec64s, 16> SurfDescriptors;
#else
    typedef util::AlignedMemory<math::Vec128f, 16> SiftDescriptors;
    typedef util::AlignedMemory<math::Vec64f, 16> SurfDescriptors;
#endif

    /** Internal initialization methods for SIFT/SURF features. */
    void init_sift (SiftDescriptors* dst, Sift::Descriptors const& src);
    void init_surf (SurfDescriptors* dst, Surf::Descriptors const& src);

    struct ProcessedFeatureSet
    {
        SiftDescriptors sift_descr;
        SurfDescriptors surf_descr;
    };
    typedef std::vector<ProcessedFeatureSet> ProcessedFeatureSets;
    ProcessedFeatureSets processed_feature_sets;
};

SFM_NAMESPACE_END

#endif /* SFM_EXHAUSTIVE_MATCHING_HEADER */

