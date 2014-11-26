#include <iostream>

#include "math/algo.h"
#include "sfm/nearest_neighbor.h"
#include "sfm/matching.h"

SFM_NAMESPACE_BEGIN

void
Matching::remove_inconsistent_matches (Matching::Result* matches)
{
    for (std::size_t i = 0; i < matches->matches_1_2.size(); ++i)
    {
        if (matches->matches_1_2[i] < 0)
            continue;
        if (matches->matches_2_1[matches->matches_1_2[i]] != (int)i)
            matches->matches_1_2[i] = -1;
    }

    for (std::size_t i = 0; i < matches->matches_2_1.size(); ++i)
    {
        if (matches->matches_2_1[i] < 0)
            continue;
        if (matches->matches_1_2[matches->matches_2_1[i]] != (int)i)
            matches->matches_2_1[i] = -1;
    }
}

int
Matching::count_consistent_matches (Matching::Result const& matches)
{
    int counter = 0;
    for (int i = 0; i < static_cast<int>(matches.matches_1_2.size()); ++i)
        if (matches.matches_1_2[i] != -1
            && matches.matches_2_1[matches.matches_1_2[i]] == i)
            counter++;
    return counter;
}

SFM_NAMESPACE_END
