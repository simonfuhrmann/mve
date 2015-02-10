#include <iostream>
#include <vector>

#include "sfm/ransac_homography.h"
#include "sfm/bundler_init_pair.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

namespace
{
    struct PairsComparator
    {
        PairsComparator (PairwiseMatching const& matching) : matching(&matching) {}
        PairwiseMatching const* matching;
        bool operator() (int const& a, int const& b)
        {
            return matching->at(a).matches.size() > matching->at(b).matches.size();
        }
    };
}  /* namespace */

void
InitialPair::initialize (ViewportList const& viewports,
    PairwiseMatching const& matching)
{
    this->viewports = &viewports;
    this->matching = &matching;
}

void
InitialPair::compute_pair (Result* result)
{
    if (this->viewports == NULL || this->matching == NULL)
        throw std::invalid_argument("NULL viewports or matching");

    /* Sort pairwise matching indices according to number of matches. */
    if (this->opts.verbose_output)
        std::cout << "Sorting pairwise matches..." << std::endl;

    std::vector<int> pair_ids(this->matching->size(), -1);
    for (std::size_t i = 0; i < pair_ids.size(); ++i)
        pair_ids[i] = i;
    PairsComparator cmp(*this->matching);
    std::sort(pair_ids.begin(), pair_ids.end(), cmp);

    /* Prepare homography RANSAC. */
    if (this->opts.verbose_output)
        std::cout << "Searching for initial pair..." << std::endl;

    bool found_pair = false;
    std::size_t found_pair_id = std::numeric_limits<std::size_t>::max();

#pragma omp parallel for schedule(dynamic)
    for (std::size_t i = 0; i < pair_ids.size(); ++i)
    {
        if (found_pair)
            continue;

        TwoViewMatching const& tvm = this->matching->at(pair_ids[i]);
        FeatureSet const& view1 = this->viewports->at(tvm.view_1_id).features;
        FeatureSet const& view2 = this->viewports->at(tvm.view_2_id).features;

        /* Prepare correspondences for RANSAC. */
        Correspondences correspondences(tvm.matches.size());
        for (std::size_t j = 0; j < tvm.matches.size(); ++j)
        {
            Correspondence& c = correspondences[j];
            math::Vec2f const& pos1 = view1.positions[tvm.matches[j].first];
            math::Vec2f const& pos2 = view2.positions[tvm.matches[j].second];
            std::copy(pos1.begin(), pos1.end(), c.p1);
            std::copy(pos2.begin(), pos2.end(), c.p2);
        }

        /* Run RANSAC. */
        RansacHomography::Result ransac_result;
        RansacHomography homography_ransac(this->opts.homography_opts);
        homography_ransac.estimate(correspondences, &ransac_result);

        /* Compute homography inliers percentage. */
        float const num_matches = tvm.matches.size();
        float const num_inliers = ransac_result.inliers.size();
        float const percentage = num_inliers / num_matches;

#pragma omp critical
        if (this->opts.verbose_output)
        {
            std::cout << "  Pair "
                << "(" << tvm.view_1_id << "," << tvm.view_2_id << "): "
                << num_matches << " matches, "
                << num_inliers << " homography inliers ("
                << util::string::get_fixed(100.0f * percentage, 2)
                << "%)." << std::endl;
        }

        if (percentage < this->opts.max_homography_inliers)
        {
#pragma omp critical
            if (i < found_pair_id)
            {
                result->view_1_id = tvm.view_1_id;
                result->view_2_id = tvm.view_2_id;
                found_pair_id = i;
                found_pair = true;
            }
        }
    }

    if (!found_pair)
        throw std::invalid_argument("No more available pairs");
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

