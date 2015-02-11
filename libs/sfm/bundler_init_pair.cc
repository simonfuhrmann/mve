#include <iostream>
#include <vector>

#include "sfm/ransac_homography.h"
#include "sfm/bundler_init_pair.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

namespace
{
    struct CandidatePair
    {
        int view_1_id;
        int view_2_id;
        Correspondences matches;
    };

    bool
    candidate_pair_cmp (CandidatePair const& p1, CandidatePair const& p2)
    {
        return p1.matches.size() < p2.matches.size();
    }
}

void
InitialPair::compute_pair (Result* result)
{
    if (this->viewports == NULL || this->tracks == NULL)
        throw std::invalid_argument("NULL viewports or tracks");

    std::cout << "Searching for initial pair..." << std::endl;

    /* Convert the tracks to pairwise information. */
    int const num_viewports = static_cast<int>(this->viewports->size());
    std::vector<int> candidate_lookup(MATH_POW2(num_viewports), -1);
    std::vector<CandidatePair> candidates;
    candidates.reserve(1000);
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
                    pair_id = static_cast<int>(candidates.size());
                    candidate_lookup[lookup_id] = pair_id;
                    candidates.push_back(CandidatePair());
                    candidates.back().view_1_id = v1id;
                    candidates.back().view_2_id = v2id;
                }

                /* Fill candidate with additional info. */
                Viewport const& view1 = this->viewports->at(v1id);
                Viewport const& view2 = this->viewports->at(v2id);
                math::Vec2f const v1pos = view1.features.positions[f1id];
                math::Vec2f const v2pos = view2.features.positions[f2id];
                Correspondence match;
                std::copy(v1pos.begin(), v1pos.end(), match.p1);
                std::copy(v2pos.begin(), v2pos.end(), match.p2);
                candidates[pair_id].matches.push_back(match);
            }
    }
    candidate_lookup.clear();

    /* Sort the candidate pairs by number of matches. */
    std::sort(candidates.rbegin(), candidates.rend(), candidate_pair_cmp);

    /* Search for a good initial pair with few homography inliers. */
    bool found_pair = false;
    std::size_t found_pair_id = std::numeric_limits<std::size_t>::max();
#pragma omp parallel for schedule(dynamic)
    for (std::size_t i = 0; i < candidates.size(); ++i)
    {
        if (found_pair)
            continue;

        /* Execute homography RANSAC. */
        CandidatePair const& candidate = candidates[i];
        RansacHomography::Result ransac_result;
        RansacHomography homography_ransac(this->opts.homography_opts);
        homography_ransac.estimate(candidate.matches, &ransac_result);

        /* Compute homography inliers percentage. */
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

        if (percentage > this->opts.max_homography_inliers)
            continue;

        // TODO: Compute pose, triangulate tracks and check for quality.

#pragma omp critical
        if (i < found_pair_id)
        {
            result->view_1_id = candidate.view_1_id;
            result->view_2_id = candidate.view_2_id;
            found_pair_id = i;
            found_pair = true;
        }
    }

    if (!found_pair)
        throw std::invalid_argument("No more available pairs");
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

