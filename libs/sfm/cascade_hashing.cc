/*
 * Copyright (C) 2016, Andre Schulz
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <algorithm>
#include <cstdint>
#include <vector>

#include "sfm/cascade_hashing.h"

SFM_NAMESPACE_BEGIN

void
CascadeHashing::GlobalData::generate_proj_matrices (Options const& opts)
{
    generate_proj_matrices(
        &this->sift.prim_proj_mat,
        &this->sift.sec_proj_mats,
        opts);
    generate_proj_matrices(
        &this->surf.prim_proj_mat,
        &this->surf.sec_proj_mats,
        opts);
}

/* ---------------------------------------------------------------- */

void
CascadeHashing::init (bundler::ViewportList* viewports)
{
    ExhaustiveMatching::init(viewports);

    util::WallTimer timer;
    this->local_data_sift.clear();
    this->local_data_sift.resize(viewports->size());
    this->local_data_surf.clear();
    this->local_data_surf.resize(viewports->size());

    this->global_data.generate_proj_matrices(this->cashash_opts);

    /* Convert feature descriptors to zero mean. */
    math::Vec128f sift_avg;
    math::Vec64f surf_avg;
    compute_avg_descriptors(this->processed_feature_sets, &sift_avg, &surf_avg);

#pragma omp parallel for schedule(dynamic)
    for (std::size_t i = 0; i < viewports->size(); i++)
    {
        LocalData* ld_sift = &this->local_data_sift[i];
        LocalData* ld_surf = &this->local_data_surf[i];

        ProcessedFeatureSet const& pfs = this->processed_feature_sets[i];
        std::vector<math::Vec128f> sift_zero_mean_descs;
        std::vector<math::Vec64f> surf_zero_mean_descs;
        this->compute_zero_mean_descs(&sift_zero_mean_descs,
            &surf_zero_mean_descs, pfs.sift_descr, pfs.surf_descr, sift_avg,
            surf_avg);

        this->compute(ld_sift, ld_surf, sift_zero_mean_descs,
            surf_zero_mean_descs, this->global_data, this->cashash_opts);
    }
    std::cout << "Computing cascade hashes took " << timer.get_elapsed()
        << " ms" << std::endl;
}

void
CascadeHashing::pairwise_match (int view_1_id, int view_2_id,
    Matching::Result* result) const
{
    /* SIFT matching. */
    ProcessedFeatureSet const& pfs_1 = this->processed_feature_sets[view_1_id];
    ProcessedFeatureSet const& pfs_2 = this->processed_feature_sets[view_2_id];
    LocalData const& ld_sift_1 = this->local_data_sift[view_1_id];
    LocalData const& ld_sift_2 = this->local_data_sift[view_2_id];
    Matching::Result sift_result;
    if (pfs_1.sift_descr.size() > 0)
    {
        this->twoway_match(this->opts.sift_matching_opts,
            ld_sift_1, ld_sift_2,
            pfs_1.sift_descr, pfs_2.sift_descr,
            &sift_result, this->cashash_opts);
        Matching::remove_inconsistent_matches(&sift_result);
    }

    /* SURF matching. */
    LocalData const& ld_surf_1 = this->local_data_surf[view_1_id];
    LocalData const& ld_surf_2 = this->local_data_surf[view_2_id];
    Matching::Result surf_result;
    if (pfs_1.surf_descr.size() > 0)
    {
        this->twoway_match(this->opts.surf_matching_opts,
            ld_surf_1, ld_surf_2,
            pfs_1.surf_descr, pfs_2.surf_descr,
            &surf_result, this->cashash_opts);
        Matching::remove_inconsistent_matches(&surf_result);
    }

    Matching::combine_results(sift_result, surf_result, result);
}

void
CascadeHashing::compute(
    LocalData* ld_sift, LocalData* ld_surf,
    std::vector<math::Vec128f> const& sift_zero_mean_descs,
    std::vector<math::Vec64f> const& surf_zero_mean_descs,
    GlobalData const& cashash_global_data,
    Options const& cashash_opts)
{
    /* Compute cascade hashes of zero mean SIFT/SURF descriptors. */
    compute_cascade_hashes(
        sift_zero_mean_descs,
        &ld_sift->comp_hash_data,
        &ld_sift->bucket_grps_bucket_ids,
        cashash_global_data.sift.prim_proj_mat,
        cashash_global_data.sift.sec_proj_mats,
        cashash_opts);
    compute_cascade_hashes(
        surf_zero_mean_descs,
        &ld_surf->comp_hash_data,
        &ld_surf->bucket_grps_bucket_ids,
        cashash_global_data.surf.prim_proj_mat,
        cashash_global_data.surf.sec_proj_mats,
        cashash_opts);

    /* Build buckets. */
    build_buckets(
        &ld_sift->bucket_grps_feature_ids,
        ld_sift->bucket_grps_bucket_ids,
        sift_zero_mean_descs.size(),
        cashash_opts);
    build_buckets(
        &ld_surf->bucket_grps_feature_ids,
        ld_surf->bucket_grps_bucket_ids,
        surf_zero_mean_descs.size(),
        cashash_opts);
}

/* ---------------------------------------------------------------- */

void
CascadeHashing::compute_avg_descriptors (ProcessedFeatureSets const& pfs,
    math::Vec128f* sift_avg, math::Vec64f* surf_avg)
{
    math::Vec128f sift_vec_sum(0.0f);
    math::Vec64f surf_vec_sum(0.0f);
    std::size_t num_sift_descs_total = 0;
    std::size_t num_surf_descs_total = 0;

    /* Compute sum of all SIFT/SURF descriptors. */
    for (std::size_t i = 0; i < pfs.size(); i++)
    {
        SiftDescriptors const& sift_descr = pfs[i].sift_descr;
        SurfDescriptors const& surf_descr = pfs[i].surf_descr;

        std::size_t num_sift_descriptors = sift_descr.size();
        std::size_t num_surf_descriptors = surf_descr.size();
        num_sift_descs_total += num_sift_descriptors;
        num_surf_descs_total += num_surf_descriptors;

        for (std::size_t j = 0; j < num_sift_descriptors; j++)
            for (int k = 0; k < 128; k++)
                sift_vec_sum[k] += sift_descr[j][k] / 255.0f;

        for (std::size_t j = 0; j < num_surf_descriptors; j++)
            for (int k = 0; k < 64; k++)
                surf_vec_sum[k] += surf_descr[j][k] / 127.0f;
    }

    /* Compute average vectors for SIFT/SURF. */
    *sift_avg = sift_vec_sum / num_sift_descs_total;
    *surf_avg = surf_vec_sum / num_surf_descs_total;
}

void
CascadeHashing::compute_zero_mean_descs(
    std::vector<math::Vec128f>* sift_zero_mean_descs,
    std::vector<math::Vec64f>* surf_zero_mean_descs,
    SiftDescriptors const& sift_descs, SurfDescriptors const& surf_descs,
    math::Vec128f const& sift_avg, math::Vec64f const& surf_avg)
{
    /* Compute zero mean descriptors. */
    sift_zero_mean_descs->resize(sift_descs.size());
    for (std::size_t i = 0; i < sift_descs.size(); i++)
        for (int j = 0; j < 128; j++)
            (*sift_zero_mean_descs)[i][j] = sift_descs[i][j] / 255.0f - sift_avg[j];

    surf_zero_mean_descs->resize(surf_descs.size());
    for (std::size_t i = 0; i < surf_descs.size(); i++)
        for (int j = 0; j < 64; j++)
            (*surf_zero_mean_descs)[i][j] = surf_descs[i][j] / 127.0f - surf_avg[j];
}

/* ---------------------------------------------------------------- */

void
CascadeHashing::build_buckets(
    BucketGroupsFeatures *bucket_grps_feature_ids,
    BucketGroupsBuckets const& bucket_grps_bucket_ids,
    size_t num_descs,
    Options const& opts)
{
    uint8_t const num_bucket_grps = opts.num_bucket_groups;
    uint8_t const num_bucket_bits = opts.num_bucket_bits;
    uint32_t const num_buckets_per_group = 1 << num_bucket_bits;

    bucket_grps_feature_ids->resize(num_bucket_grps);

    for (uint8_t grp_idx = 0; grp_idx < num_bucket_grps; grp_idx++)
    {
        BucketGroupFeatures &bucket_grp_features = (*bucket_grps_feature_ids)[grp_idx];
        bucket_grp_features.resize(num_buckets_per_group);
        for (size_t i = 0; i < num_descs; i++)
        {
            uint16_t bucket_id = bucket_grps_bucket_ids[grp_idx][i];
            bucket_grp_features[bucket_id].emplace_back(i);
        }
    }
}

SFM_NAMESPACE_END
