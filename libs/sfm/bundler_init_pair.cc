/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>
#include <vector>

#include "sfm/ransac_homography.h"
#include "sfm/triangulate.h"
#include "sfm/bundler_init_pair.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

void
InitialPair::compute_pair (Result* result)
{
    if (this->viewports == NULL || this->tracks == NULL)
        throw std::invalid_argument("NULL viewports or tracks");

    std::cout << "Searching for initial pair..." << std::endl;

    /* Convert tracks to pairwise information. */
    std::vector<CandidatePair> candidates;
    this->compute_candidate_pairs(&candidates);

    /* Sort the candidate pairs by number of matches. */
    std::sort(candidates.rbegin(), candidates.rend());

    /* Search for a good initial pair with few homography inliers. */
    bool found_pair = false;
    std::size_t found_pair_id = std::numeric_limits<std::size_t>::max();
#pragma omp parallel for schedule(dynamic)
    for (std::size_t i = 0; i < candidates.size(); ++i)
    {
        if (found_pair)
            continue;

        /* Employ threshold on homography inliers percentage. */
        CandidatePair const& candidate = candidates[i];
        float const percentage = this->compute_homography_ratio(candidate);
        if (percentage > this->opts.max_homography_inliers)
            continue;

        /* Compute initial pair pose. */
        CameraPose pose1, pose2;
        bool const found_pose = this->compute_pose(candidate, &pose1, &pose2);
        if (!found_pose)
            continue;

        /* Test initial pair quality. */
        // TODO

#pragma omp critical
        if (i < found_pair_id)
        {
            result->view_1_id = candidate.view_1_id;
            result->view_2_id = candidate.view_2_id;
            result->view_1_pose = pose1;
            result->view_2_pose = pose2;
            found_pair_id = i;
            found_pair = true;
        }
    }

    if (!found_pair)
    {
        result->view_1_id = -1;
        result->view_2_id = -1;
    }
}

void
InitialPair::compute_pair (int view_1_id, int view_2_id, Result* result)
{
    if (view_1_id > view_2_id)
        std::swap(view_1_id, view_2_id);

    /* Convert tracks to pairwise information. */
    std::vector<CandidatePair> candidates;
    this->compute_candidate_pairs(&candidates);

    /* Find candidate pair. */
    CandidatePair* candidate = NULL;
    for (std::size_t i = 0; candidate == NULL && i < candidates.size(); ++i)
    {
        if (view_1_id == candidates[i].view_1_id
            && view_2_id == candidates[i].view_2_id)
            candidate = &candidates[i];
    }
    if (candidate == NULL)
        throw std::runtime_error("No matches for initial pair");

    /* Compute initial pair pose. */
    result->view_1_id = view_1_id;
    result->view_2_id = view_2_id;
    bool const found_pose = this->compute_pose(*candidate,
        &result->view_1_pose, &result->view_2_pose);
    if (!found_pose)
        throw std::runtime_error("Cannot compute pose for initial pair");
}

void
InitialPair::compute_candidate_pairs (CandidatePairs* candidates)
{
    /*
     * Convert the tracks to pairwise information. This is similar to using
     * the pairwise matching result directly, however, the tracks have been
     * further filtered during track generation.
     */
    int const num_viewports = static_cast<int>(this->viewports->size());
    std::vector<int> candidate_lookup(MATH_POW2(num_viewports), -1);
    candidates->reserve(1000);
    for (std::size_t i = 0; i < this->tracks->size(); ++i)
    {
        Track const& track = this->tracks->at(i);
        for (std::size_t j = 1; j < track.features.size(); ++j)
            for (std::size_t k = 0; k < j; ++k)
            {
                int v1id = track.features[j].view_id;
                int v2id = track.features[k].view_id;
                int f1id = track.features[j].feature_id;
                int f2id = track.features[k].feature_id;
                if (v1id > v2id)
                {
                    std::swap(v1id, v2id);
                    std::swap(f1id, f2id);
                }

                /* Lookup pair. */
                int const lookup_id = v1id * num_viewports + v2id;
                int pair_id = candidate_lookup[lookup_id];
                if (pair_id == -1)
                {
                    pair_id = static_cast<int>(candidates->size());
                    candidate_lookup[lookup_id] = pair_id;
                    candidates->push_back(CandidatePair());
                    candidates->back().view_1_id = v1id;
                    candidates->back().view_2_id = v2id;
                }

                /* Fill candidate with additional info. */
                Viewport const& view1 = this->viewports->at(v1id);
                Viewport const& view2 = this->viewports->at(v2id);
                math::Vec2f const v1pos = view1.features.positions[f1id];
                math::Vec2f const v2pos = view2.features.positions[f2id];
                Correspondence2D2D match;
                std::copy(v1pos.begin(), v1pos.end(), match.p1);
                std::copy(v2pos.begin(), v2pos.end(), match.p2);
                candidates->at(pair_id).matches.push_back(match);
            }
    }
}

float
InitialPair::compute_homography_ratio (CandidatePair const& candidate)
{
    /* Execute homography RANSAC. */
    RansacHomography::Result ransac_result;
    RansacHomography homography_ransac(this->opts.homography_opts);
    homography_ransac.estimate(candidate.matches, &ransac_result);

    float const num_matches = candidate.matches.size();
    float const num_inliers = ransac_result.inliers.size();
    float const percentage = num_inliers / num_matches;

#pragma omp critical
    if (this->opts.verbose_output)
    {
        std::cout << "  Pair (" << candidate.view_1_id
            << "," << candidate.view_2_id << "): "
            << num_matches << " matches, "
            << num_inliers << " homography inliers ("
            << util::string::get_fixed(100.0f * percentage, 2)
            << "%)." << std::endl;
    }

    return percentage;
}

bool
InitialPair::compute_pose (CandidatePair const& candidate,
    CameraPose* pose1, CameraPose* pose2)
{
    /* Compute fundamental matrix from pair correspondences. */
    FundamentalMatrix fundamental;
    {
        Correspondences2D2D matches = candidate.matches;
        fundamental_least_squares(matches, &fundamental);
        enforce_fundamental_constraints(&fundamental);
    }

    /* Populate K-matrices. */
    Viewport const& view_1 = this->viewports->at(candidate.view_1_id);
    Viewport const& view_2 = this->viewports->at(candidate.view_2_id);
    pose1->set_k_matrix(view_1.focal_length, 0.0, 0.0);
    pose1->init_canonical_form();
    pose2->set_k_matrix(view_2.focal_length, 0.0, 0.0);

    /* Compute essential matrix from fundamental matrix (HZ (9.12)). */
    EssentialMatrix E = pose2->K.transposed() * fundamental * pose1->K;

    /* Compute pose from essential. */
    std::vector<CameraPose> poses;
    pose_from_essential(E, &poses);

    /* Find the correct pose using point test (HZ Fig. 9.12). */
    bool found_pose = false;
    for (std::size_t i = 0; i < poses.size(); ++i)
    {
        poses[i].K = pose2->K;
        if (is_consistent_pose(candidate.matches[0], *pose1, poses[i]))
        {
            *pose2 = poses[i];
            found_pose = true;
            break;
        }
    }
    return found_pose;
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

