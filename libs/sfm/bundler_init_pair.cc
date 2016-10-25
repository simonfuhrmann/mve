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
#include <sstream>
#include <random>

#include "sfm/ransac_homography.h"
#include "sfm/triangulate.h"
#include "sfm/bundler_init_pair.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

void
InitialPair::compute_pair (Result* result)
{
    if (this->viewports == nullptr || this->tracks == nullptr)
        throw std::invalid_argument("Null viewports or tracks");

    std::cout << "Searching for initial pair..." << std::endl;
    result->view_1_id = -1;
    result->view_2_id = -1;

    /* Convert tracks to pairwise information. */
    std::vector<CandidatePair> candidates;
    this->compute_candidate_pairs(&candidates);

    /* Sort the candidate pairs by number of matches. */
    std::sort(candidates.rbegin(), candidates.rend());

    /*
     * Search for a good initial pair and return the first pair that
     * satisfies all thresholds (min matches, max homography inliers,
     * min triangulation angle). If no pair satisfies all thresholds, the
     * pair with the best score is returned.
     */
    bool found_pair = false;
    std::size_t found_pair_id = std::numeric_limits<std::size_t>::max();
    std::vector<float> pair_scores(candidates.size(), 0.0f);
#pragma omp parallel for schedule(dynamic)
    for (std::size_t i = 0; i < candidates.size(); ++i)
    {
        if (found_pair)
            continue;

        /* Reject pairs with 8 or fewer matches. */
        CandidatePair const& candidate = candidates[i];
        std::size_t num_matches = candidate.matches.size();
        if (num_matches < static_cast<std::size_t>(this->opts.min_num_matches))
        {
            this->debug_output(candidate);
            continue;
        }

        /* Reject pairs with too high percentage of homograhy inliers. */
        std::size_t num_inliers = this->compute_homography_inliers(candidate);
        float percentage = static_cast<float>(num_inliers) / num_matches;
        if (percentage > this->opts.max_homography_inliers)
        {
            this->debug_output(candidate, num_inliers);
            continue;
        }

        /* Compute initial pair pose. */
        CameraPose pose1, pose2;
        bool const found_pose = this->compute_pose(candidate, &pose1, &pose2);
        if (!found_pose)
        {
            this->debug_output(candidate, num_inliers);
            continue;
        }

        /* Rejects pairs with bad triangulation angle. */
        double const angle = this->angle_for_pose(candidate, pose1, pose2);
        pair_scores[i] = this->score_for_pair(candidate, num_inliers, angle);
        this->debug_output(candidate, num_inliers, angle);
        if (angle < this->opts.min_triangulation_angle)
            continue;

        /* Run triangulation to ensure correct pair */
        Triangulate::Options triangulate_opts;
        Triangulate triangulator(triangulate_opts);
        std::vector<CameraPose const*> poses;
        poses.push_back(&pose1);
        poses.push_back(&pose2);
        std::size_t successful_triangulations = 0;
        std::vector<math::Vec2f> positions(2);
        Triangulate::Statistics stats;
        for (std::size_t j = 0; j < candidate.matches.size(); ++j)
        {
            positions[0] = math::Vec2f(candidate.matches[j].p1);
            positions[1] = math::Vec2f(candidate.matches[j].p2);
            math::Vec3d pos3d;
            if (triangulator.triangulate(poses, positions, &pos3d, &stats))
                successful_triangulations += 1;
        }
        if (successful_triangulations * 2 < candidate.matches.size())
            continue;

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

    /* Return if a pair satisfying all thresholds has been found. */
    if (found_pair)
        return;

    /* Return pair with best score (larger than 0.0). */
    std::cout << "Searching for pair with best score..." << std::endl;
    float best_score = 0.0f;
    std::size_t best_pair_id = 0;
    for (std::size_t i = 0; i < pair_scores.size(); ++i)
    {
        if (pair_scores[i] <= best_score)
            continue;

        best_score = pair_scores[i];
        best_pair_id = i;
    }

    /* Recompute pose for pair with best score. */
    if (best_score > 0.0f)
    {
        result->view_1_id = candidates[best_pair_id].view_1_id;
        result->view_2_id = candidates[best_pair_id].view_2_id;
        this->compute_pose(candidates[best_pair_id],
            &result->view_1_pose, &result->view_2_pose);
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
    CandidatePair* candidate = nullptr;
    for (std::size_t i = 0; candidate == nullptr && i < candidates.size(); ++i)
    {
        if (view_1_id == candidates[i].view_1_id
            && view_2_id == candidates[i].view_2_id)
            candidate = &candidates[i];
    }
    if (candidate == nullptr)
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

std::size_t
InitialPair::compute_homography_inliers (CandidatePair const& candidate)
{
    /* Execute homography RANSAC. */
    RansacHomography::Result ransac_result;
    RansacHomography homography_ransac(this->opts.homography_opts);
    homography_ransac.estimate(candidate.matches, &ransac_result);
    return ransac_result.inliers.size();
}

bool
InitialPair::compute_pose (CandidatePair const& candidate,
    CameraPose* pose1, CameraPose* pose2)
{
    /* Compute fundamental matrix from pair correspondences. */
    FundamentalMatrix fundamental;
    {
        Correspondences2D2D matches = candidate.matches;
        if (matches.size() > 1000ul) {
            std::mt19937 g;
            std::shuffle(matches.begin(), matches.end(), g);
            matches.resize(1000ul);
        }
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

double
InitialPair::angle_for_pose (CandidatePair const& candidate,
    CameraPose const& pose1, CameraPose const& pose2)
{
    /* Compute transformation from image coordinates to viewing direction. */
    math::Matrix3d T1 = pose1.R.transposed() * math::matrix_inverse(pose1.K);
    math::Matrix3d T2 = pose2.R.transposed() * math::matrix_inverse(pose2.K);

    /* Compute triangulation angle for each correspondence. */
    std::vector<double> cos_angles;
    cos_angles.reserve(candidate.matches.size());
    for (std::size_t i = 0; i < candidate.matches.size(); ++i)
    {
        Correspondence2D2D const& match = candidate.matches[i];
        math::Vec3d p1(match.p1[0], match.p1[1], 1.0);
        p1 = T1.mult(p1).normalized();
        math::Vec3d p2(match.p2[0], match.p2[1], 1.0);
        p2 = T2.mult(p2).normalized();
        cos_angles.push_back(p1.dot(p2));
    }

    /* Return 50% median. */
    std::size_t median_index = cos_angles.size() / 2;
    std::nth_element(cos_angles.begin(),
        cos_angles.begin() + median_index, cos_angles.end());
    double const cos_angle = math::clamp(cos_angles[median_index], -1.0, 1.0);
    return std::acos(cos_angle);
}

float
InitialPair::score_for_pair (CandidatePair const& candidate,
    std::size_t num_inliers, double angle)
{
    float const matches = static_cast<float>(candidate.matches.size());
    float const inliers = static_cast<float>(num_inliers) / matches;
    float const angle_d = MATH_RAD2DEG(angle);

    /* Score for matches (min: 20, good: 200). */
    float f1 = 2.0 / (1.0 + std::exp((20.0 - matches) * 6.0 / 200.0)) - 1.0;
    /* Score for angle (min 1 degree, good 8 degree). */
    float f2 = 2.0 / (1.0 + std::exp((1.0 - angle_d) * 6.0 / 8.0)) - 1.0;
    /* Score for H-Inliers (max 70%, good 40%). */
    float f3 = 2.0 / (1.0 + std::exp((inliers - 0.7) * 6.0 / 0.4)) - 1.0;

    f1 = math::clamp(f1, 0.0f, 1.0f);
    f2 = math::clamp(f2, 0.0f, 1.0f);
    f3 = math::clamp(f3, 0.0f, 1.0f);
    return f1 * f2 * f3;
}

void
InitialPair::debug_output (CandidatePair const& candidate,
    std::size_t num_inliers, double angle)
{
    if (!this->opts.verbose_output)
        return;

    std::stringstream message;
    std::size_t num_matches = candidate.matches.size();
    message << "  Pair " << std::setw(3) << candidate.view_1_id
        << "," << std::setw(3) << candidate.view_2_id
        << ": " << std::setw(4) << num_matches << " matches";

    if (num_inliers > 0)
    {
        float percentage = static_cast<float>(num_inliers) / num_matches;
        message << ", " << std::setw(4) << num_inliers
            << " H-inliers (" << (int)(100.0f * percentage) << "%)";
    }

    if (angle > 0.0)
    {
        message << ", " << std::setw(5)
            << util::string::get_fixed(MATH_RAD2DEG(angle), 2)
            << " pair angle";
    }

#pragma omp critical
    std::cout << message.str() << std::endl;
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

