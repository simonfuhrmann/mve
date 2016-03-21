/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "sfm/exhaustive_matching.h"

SFM_NAMESPACE_BEGIN

#if DISCRETIZE_DESCRIPTORS
namespace
{
    void
    convert_descriptor (Sift::Descriptor const& descr, unsigned short* data)
    {
        for (int i = 0; i < 128; ++i)
        {
            float value = descr.data[i];
            value = math::clamp(value, 0.0f, 1.0f);
            value = math::round(value * 255.0f);
            data[i] = static_cast<unsigned char>(value);
        }
    }

    void
    convert_descriptor (Surf::Descriptor const& descr, signed short* data)
    {
        for (int i = 0; i < 64; ++i)
        {
            float value = descr.data[i];
            value = math::clamp(value, -1.0f, 1.0f);
            value = math::round(value * 127.0f);
            data[i] = static_cast<signed char>(value);
        }
    }
#else // DISCRETIZE_DESCRIPTORS
    void
    convert_descriptor (Sift::Descriptor const& descr, float* data)
    {
        std::copy(descr.data.begin(), descr.data.end(), data);
    }

    void
    convert_descriptor (Surf::Descriptor const& descr, float* data)
    {
        std::copy(descr.data.begin(), descr.data.end(), data);
    }
#endif // DISCRETIZE_DESCRIPTORS
}

void
ExhaustiveMatching::init (bundler::ViewportList* viewports)
{
    this->processed_feature_sets.clear();
    this->processed_feature_sets.resize(viewports->size());

#pragma omp parallel for schedule(dynamic)
#ifdef _MSC_VER
    for (int64_t i = 0; i < viewports->size(); i++)
#else
    for (size_t i = 0; i < viewports->size(); i++)
#endif
    {
        FeatureSet const& fs = (*viewports)[i].features;
        ProcessedFeatureSet& pfs = this->processed_feature_sets[i];

        this->init_sift(&pfs.sift_descr, fs.sift_descriptors);
        this->init_surf(&pfs.surf_descr, fs.surf_descriptors);
    }
}

void
ExhaustiveMatching::init_sift (SiftDescriptors* dst,
    Sift::Descriptors const& src)
{
    /* Prepare and copy to data structures. */
    dst->resize(src.size());

#if DISCRETIZE_DESCRIPTORS
    uint16_t* ptr = dst->data()->begin();
#else
    float* ptr = dst->data()->begin();
#endif
    for (std::size_t i = 0; i < src.size(); ++i, ptr += 128)
    {
        Sift::Descriptor const& d = src[i];
        convert_descriptor(d, ptr);
    }
}

void
ExhaustiveMatching::init_surf (SurfDescriptors* dst,
    Surf::Descriptors const& src)
{
    /* Prepare and copy to data structures. */
    dst->resize(src.size());

#if DISCRETIZE_DESCRIPTORS
    int16_t* ptr = dst->data()->begin();
#else
    float* ptr = dst->data()->begin();
#endif
    for (std::size_t i = 0; i < src.size(); ++i, ptr += 64)
    {
        Surf::Descriptor const& d = src[i];
        convert_descriptor(d, ptr);
    }
}

void
ExhaustiveMatching::pairwise_match (int view_1_id, int view_2_id,
    Matching::Result* result) const
{
    ProcessedFeatureSet const& pfs_1 = this->processed_feature_sets[view_1_id];
    ProcessedFeatureSet const& pfs_2 = this->processed_feature_sets[view_2_id];

    /* SIFT matching. */
    Matching::Result sift_result;
    if (pfs_1.sift_descr.size() > 0)
    {
        Matching::twoway_match(this->opts.sift_matching_opts,
            pfs_1.sift_descr.data()->begin(), pfs_1.sift_descr.size(),
            pfs_2.sift_descr.data()->begin(), pfs_2.sift_descr.size(),
            &sift_result);
        Matching::remove_inconsistent_matches(&sift_result);
    }

    /* SURF matching. */
    Matching::Result surf_result;
    if (pfs_1.surf_descr.size() > 0)
    {
        Matching::twoway_match(this->opts.surf_matching_opts,
            pfs_1.surf_descr.data()->begin(), pfs_1.surf_descr.size(),
            pfs_2.surf_descr.data()->begin(), pfs_2.surf_descr.size(),
            &surf_result);
        Matching::remove_inconsistent_matches(&surf_result);
    }

    /* Fix offsets in the matching result. */
    std::size_t other_surf_offset = pfs_2.sift_descr.size();
    if (other_surf_offset > 0)
        for (std::size_t i = 0; i < surf_result.matches_1_2.size(); ++i)
            if (surf_result.matches_1_2[i] >= 0)
                surf_result.matches_1_2[i] += other_surf_offset;

    std::size_t this_surf_offset = pfs_1.sift_descr.size();
    if (this_surf_offset > 0)
        for (std::size_t i = 0; i < surf_result.matches_2_1.size(); ++i)
            if (surf_result.matches_2_1[i] >= 0)
                surf_result.matches_2_1[i] += this_surf_offset;

    /* Create a combined matching result. */
    std::size_t this_num_descriptors = pfs_1.sift_descr.size()
        + pfs_1.surf_descr.size();
    std::size_t other_num_descriptors = pfs_2.sift_descr.size()
        + pfs_2.surf_descr.size();

    result->matches_1_2.clear();
    result->matches_1_2.reserve(this_num_descriptors);
    result->matches_1_2.insert(result->matches_1_2.end(),
        sift_result.matches_1_2.begin(), sift_result.matches_1_2.end());
    result->matches_1_2.insert(result->matches_1_2.end(),
        surf_result.matches_1_2.begin(), surf_result.matches_1_2.end());

    result->matches_2_1.clear();
    result->matches_2_1.reserve(other_num_descriptors);
    result->matches_2_1.insert(result->matches_2_1.end(),
        sift_result.matches_2_1.begin(), sift_result.matches_2_1.end());
    result->matches_2_1.insert(result->matches_2_1.end(),
        surf_result.matches_2_1.begin(), surf_result.matches_2_1.end());
}

int
ExhaustiveMatching::pairwise_match_lowres (int view_1_id, int view_2_id,
    std::size_t num_features) const
{
    ProcessedFeatureSet const& pfs_1 = this->processed_feature_sets[view_1_id];
    ProcessedFeatureSet const& pfs_2 = this->processed_feature_sets[view_2_id];

    /* SIFT lowres matching. */
    if (pfs_1.sift_descr.size() > 0)
    {
        Matching::Result sift_result;
        Matching::twoway_match(this->opts.sift_matching_opts,
            pfs_1.sift_descr.data()->begin(),
            std::min(num_features, pfs_1.sift_descr.size()),
            pfs_2.sift_descr.data()->begin(),
            std::min(num_features, pfs_2.sift_descr.size()),
            &sift_result);
        return Matching::count_consistent_matches(sift_result);
    }

    /* SURF lowres matching. */
    if (pfs_1.surf_descr.size() > 0)
    {
        Matching::Result surf_result;
        Matching::twoway_match(this->opts.surf_matching_opts,
            pfs_1.surf_descr.data()->begin(),
            std::min(num_features, pfs_1.surf_descr.size()),
            pfs_2.surf_descr.data()->begin(),
            std::min(num_features, pfs_2.surf_descr.size()),
            &surf_result);
        return Matching::count_consistent_matches(surf_result);
    }

    return 0;
}

SFM_NAMESPACE_END

