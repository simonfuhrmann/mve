#include <iostream>
#include <fstream>
#include <cstring>
#include <cerrno>

#include "util/exception.h"
#include "util/timer.h"
#include "sfm/sift.h"
#include "sfm/ransac.h"
#include "sfm/bundler_matching.h"


SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

void
Matching::compute (ViewportList const& viewports,
    PairwiseMatching* pairwise_matching)
{
    std::size_t num_pairs = viewports.size() * (viewports.size() - 1) / 2;
    std::size_t current_pair = 0;

    if (this->progress != NULL)
    {
        this->progress->num_total = num_pairs;
        this->progress->num_done = 0;
    }

    for (std::size_t i = 0; i < viewports.size(); ++i)
        for (std::size_t j = 0; j < i; ++j)
        {
            current_pair += 1;
            if (viewports[i].positions.empty()
                || viewports[j].positions.empty())
                continue;

            int percent = current_pair * 100 / num_pairs;
            std::cout << "Processing pair " << i << ","
                << j << " (" << percent << "%)..." << std::endl;

            this->two_view_matching(i, j, viewports, pairwise_matching);

            if (this->progress != NULL)
                this->progress->num_done += 1;
        }

    std::cout << "Found a total of " << pairwise_matching->size()
        << " matching image pairs." << std::endl;
}

void
Matching::two_view_matching (std::size_t id_1, std::size_t id_2,
    ViewportList const& viewports, PairwiseMatching* pairwise_matching)
{
    Viewport const& view_1 = viewports[id_1];
    Viewport const& view_2 = viewports[id_2];

    /* Perform two-view descriptor matching. */
    sfm::Matching::Result matching_result;
    int num_matches = 0;
    {
        util::WallTimer timer;
        sfm::Matching::twoway_match(this->opts.matching_opts,
            view_1.descr_data.begin(), view_1.positions.size(),
            view_2.descr_data.begin(), view_2.positions.size(),
            &matching_result);
        sfm::Matching::remove_inconsistent_matches(&matching_result);
        num_matches = sfm::Matching::count_consistent_matches(matching_result);
        std::cout << "  Matching took " << timer.get_elapsed() << "ms, "
            << num_matches << " matches." << std::endl;
    }

    /* Require at least 8 matches. Check threshold. */
    if (num_matches < std::max(8, this->opts.min_feature_matches))
    {
        std::cout << "  Matches below threshold. Skipping." << std::endl;
        return;
    }

    /* Build correspondences from feature matching result. */
    sfm::Correspondences unfiltered_matches;
    CorrespondenceIndices unfiltered_indices;
    {
        std::vector<int> const& m12 = matching_result.matches_1_2;
        for (std::size_t i = 0; i < m12.size(); ++i)
        {
            if (m12[i] < 0)
                continue;

            sfm::Correspondence match;
            match.p1[0] = view_1.positions[i][0];
            match.p1[1] = view_1.positions[i][1];
            match.p2[0] = view_2.positions[m12[i]][0];
            match.p2[1] = view_2.positions[m12[i]][1];
            unfiltered_matches.push_back(match);
            unfiltered_indices.push_back(std::make_pair(i, m12[i]));
        }
    }

    /* Pose RANSAC. */
    sfm::RansacFundamental::Result ransac_result;
    int num_inliers = 0;
    {
        sfm::RansacFundamental ransac(this->opts.ransac_opts);
        util::WallTimer timer;
        ransac.estimate(unfiltered_matches, &ransac_result);
        num_inliers = ransac_result.inliers.size();
        std::cout << "  RANSAC took " << timer.get_elapsed() << "ms, "
            << ransac_result.inliers.size() << " inliers." << std::endl;
    }

    /* Require at least 8 inlier matches. */
    if (num_inliers < std::max(8, this->opts.min_matching_inliers))
    {
        std::cout << "  Inliers below threshold. Skipping." << std::endl;
        return;
    }

    /* Create Two-View matching result. */
    pairwise_matching->push_back(TwoViewMatching());
    TwoViewMatching& twoview_matching = pairwise_matching->back();
    twoview_matching.view_1_id = id_1;
    twoview_matching.view_2_id = id_2;
    for (int i = 0; i < num_inliers; ++i)
    {
        int const inlier_id = ransac_result.inliers[i];
        twoview_matching.matches.push_back(unfiltered_indices[inlier_id]);
    }
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END
