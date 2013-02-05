#include <algorithm>

#include "poseransac.h"

SFM_NAMESPACE_BEGIN

void
PoseRansac::estimate (std::vector<Match> const& matches, Result* result)
{
    if (matches.size() < 8)
        throw std::invalid_argument("At least 8 matches required");

    std::vector<int> inliers;
    inliers.reserve(matches.size());
    for (int iteration = 0; iteration < this->opts.max_iterations; ++iteration)
    {
        /* Randomly select points and estimate fundamental matrix. */
        FundamentalMatrix fundamental;
        estimate_8_point(matches, &fundamental);

        /* Find matches supporting the fundamental matrix. */
        find_inliers(matches, fundamental, &inliers);

        /* Check if current selection is best so far. */
        if (inliers.size() > result->inliers.size())
        {
            result->fundamental = fundamental;
            std::swap(result->inliers, inliers);
        }
    }
}

void
PoseRansac::estimate_8_point (std::vector<Match> const& matches,
    FundamentalMatrix* fundamental)
{
    // TODO
}

void
PoseRansac::find_inliers (std::vector<Match> const& matches,
    FundamentalMatrix const& fundamental, std::vector<int>* result)
{
    result->resize(0);
    // TODO
}

SFM_NAMESPACE_END
