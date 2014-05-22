#include <cassert>
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
        int& j = matches->matches_1_2[i];
        if (j == -1)
            continue;
        assert(j >= 0 && std::size_t(j) < matches->matches_2_1.size());
        if (matches->matches_2_1[j] != (int)i)
            j = -1;
    }
    for (std::size_t j = 0; j < matches->matches_2_1.size(); ++j)
    {
        int& i = matches->matches_2_1[j];
        if (i == -1)
            continue;
        assert(i >= 0 && std::size_t(i) < matches->matches_1_2.size());
        if (matches->matches_1_2[i] != (int)j)
            i = -1;
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
