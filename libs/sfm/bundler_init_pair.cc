#include <iostream>
#include <vector>

#include "sfm/ransac_fundamental.h"
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

    this->opts.homography_opts.already_normalized = false;
    this->opts.homography_opts.threshold = 3.0f;
    RansacHomography homography_ransac(this->opts.homography_opts);
    Correspondences correspondences;
    for (std::size_t i = 0; i < pairs.size(); ++i)
    {
        TwoViewMatching const& tvm = matching[pairs[i]];
        Viewport const& view1 = viewports[tvm.view_1_id];
        Viewport const& view2 = viewports[tvm.view_2_id];

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
        float num_matches = matching[pairs[i]].matches.size();
        float num_inliers = ransac_result.inliers.size();
        float percentage = num_inliers / num_matches;

        if (this->opts.verbose_output)
        {
            std::cout << "Pair " << i
                << " (" << tvm.view_1_id << "," << tvm.view_2_id << "): "
                << num_matches << " homography matches, "
                << num_inliers << " inliers ("
                << util::string::get_fixed(100.0f * percentage, 2)
                << "%)." << std::endl;
        }

        if (percentage < 0.6f)
        {
            result->view_1_id = tvm.view_1_id;
            result->view_2_id = tvm.view_2_id;
            break;
        }
    }

    /* Check if initial pair is valid. */
    if (result->view_1_id == -1 || result->view_2_id == -1)
        throw std::runtime_error("Initial pair failure");

    if (this->opts.verbose_output)
    {
        std::cout << "Computing fundamental matrix for initial pair "
            << result->view_1_id << " " << result->view_2_id
            << "..." << std::endl;
    }

    /* Compute fundamental matrix. */
    this->opts.fundamental_opts.already_normalized = false;
    this->opts.fundamental_opts.threshold = 3.0f;
    RansacFundamental::Result ransac_result;
    RansacFundamental fundamental_ransac(this->opts.fundamental_opts);
    fundamental_ransac.estimate(correspondences, &ransac_result);

    /* Build correspondences from inliers only. */
    std::size_t const num_inliers = ransac_result.inliers.size();
    Correspondences inliers(num_inliers);
    for (std::size_t i = 0; i < num_inliers; ++i)
        inliers[i] = correspondences[ransac_result.inliers[i]];

    /* Find normalization for inliers and re-compute fundamental. */
    {
        math::Matrix3d T1, T2;
        sfm::FundamentalMatrix F;
        sfm::compute_normalization(inliers, &T1, &T2);
        sfm::apply_normalization(T1, T2, &inliers);
        sfm::fundamental_least_squares(inliers, &F);
        sfm::enforce_fundamental_constraints(&F);
        result->fundamental = T2.transposed() * F * T1;
    }
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

