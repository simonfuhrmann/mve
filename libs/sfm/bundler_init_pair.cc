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
InitialPair::compute (ViewportList const& viewports,
    PairwiseMatching const& matching, Result* result)
{
    result->view_1_id = -1;
    result->view_2_id = -1;

    /* Sort pairwise matching indices according to number of matches. */
    if (this->opts.verbose_output)
        std::cout << "Sorting pairwise matches..." << std::endl;

    std::vector<int> pairs(matching.size(), -1);
    for (std::size_t i = 0; i < pairs.size(); ++i)
        pairs[i] = i;
    PairsComparator cmp(matching);
    std::sort(pairs.begin(), pairs.end(), cmp);

    /* Prepare homography RANSAC. */
    if (this->opts.verbose_output)
        std::cout << "Searching for initial pair..." << std::endl;

    RansacHomography homography_ransac(this->opts.homography_opts);
    Correspondences correspondences;
    for (std::size_t i = 0; i < pairs.size(); ++i)
    {
        TwoViewMatching const& tvm = matching[pairs[i]];
        FeatureSet const& view1 = viewports[tvm.view_1_id].features;
        FeatureSet const& view2 = viewports[tvm.view_2_id].features;

        /* Prepare correspondences for RANSAC. */
        correspondences.resize(tvm.matches.size());
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
        homography_ransac.estimate(correspondences, &ransac_result);

        /* Compute homography inliers percentage. */
        float num_matches = tvm.matches.size();
        float num_inliers = ransac_result.inliers.size();
        float percentage = num_inliers / num_matches;

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
            result->view_1_id = tvm.view_1_id;
            result->view_2_id = tvm.view_2_id;
            break;
        }
    }

    /* Check if initial pair is valid. */
    if (result->view_1_id == -1 || result->view_2_id == -1)
        throw std::runtime_error("Initial pair failure");
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

