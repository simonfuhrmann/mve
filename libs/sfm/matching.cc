#include <iostream>

#include "math/algo.h"
#include "nearestneighbor.h"
#include "matching.h"

SFM_NAMESPACE_BEGIN

template <>
void
match_features<short> (MatchingOptions const& options,
    short const* set_1, std::size_t set_1_size,
    short const* set_2, std::size_t set_2_size,
    MatchingResult* matches)
{
    matches->matches_1_2.clear();
    matches->matches_2_1.clear();
    matches->matches_1_2.resize(set_1_size, -1);
    matches->matches_2_1.resize(set_2_size, -1);
    if (set_1_size == 0 || set_2_size == 0)
        return;

    NearestNeighbor<short>::Result nn_result;
    NearestNeighbor<short> nn;
    nn.set_element_dimensions(options.descriptor_length);
    short const* query_pointer = 0;

    /* Match set 1 to set 2. */
    query_pointer = set_1;
    nn.set_elements(set_2, set_2_size);
    for (std::size_t i = 0; i < set_1_size; ++i, query_pointer += 64)
    {
        nn.find(query_pointer, &nn_result);
        if (nn_result.dist_1st_best > options.distance_threshold)
            continue;
        if (static_cast<float>(nn_result.dist_1st_best)
            / static_cast<float>(nn_result.dist_2nd_best)
            > options.lowe_ratio_threshold)
            continue;

        matches->matches_1_2[i] = nn_result.index_1st_best;
    }

    /* Match set 2 to set 1. */
    query_pointer = set_2;
    nn.set_elements(set_1, set_1_size);
    for (std::size_t i = 0; i < set_2_size; ++i, query_pointer += 64)
    {
        nn.find(query_pointer, &nn_result);
        if (nn_result.dist_1st_best > options.distance_threshold)
            continue;
        if (static_cast<float>(nn_result.dist_1st_best)
            / static_cast<float>(nn_result.dist_2nd_best)
            > options.lowe_ratio_threshold)
            continue;

        matches->matches_2_1[i] = nn_result.index_1st_best;
    }
}

/* ---------------------------------------------------------------- */

void
remove_inconsistent_matches (MatchingResult* matches)
{
    for (std::size_t i = 0; i < matches->matches_1_2.size(); ++i)
        if (matches->matches_2_1[matches->matches_1_2[i]] != (int)i)
            matches->matches_1_2[i] = -1;
    for (std::size_t i = 0; i < matches->matches_2_1.size(); ++i)
        if (matches->matches_1_2[matches->matches_2_1[i]] != (int)i)
            matches->matches_2_1[i] = -1;
}

SFM_NAMESPACE_END
