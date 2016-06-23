/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

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

void
Matching::combine_results(Matching::Result const& sift_result,
    Matching::Result const& surf_result, Matching::Result* result)
{
    /* Determine size of combined matching result. */
    std::size_t num_matches_1 = sift_result.matches_1_2.size()
        + surf_result.matches_1_2.size();
    std::size_t num_matches_2 = sift_result.matches_2_1.size()
        + surf_result.matches_2_1.size();

    /* Combine results. */
    result->matches_1_2.clear();
    result->matches_1_2.reserve(num_matches_1);
    result->matches_1_2.insert(result->matches_1_2.end(),
        sift_result.matches_1_2.begin(), sift_result.matches_1_2.end());
    result->matches_1_2.insert(result->matches_1_2.end(),
        surf_result.matches_1_2.begin(), surf_result.matches_1_2.end());

    result->matches_2_1.clear();
    result->matches_2_1.reserve(num_matches_2);
    result->matches_2_1.insert(result->matches_2_1.end(),
        sift_result.matches_2_1.begin(), sift_result.matches_2_1.end());
    result->matches_2_1.insert(result->matches_2_1.end(),
        surf_result.matches_2_1.begin(), surf_result.matches_2_1.end());

    /* Fix offsets. */
    std::size_t surf_offset_1 = sift_result.matches_1_2.size();
    std::size_t surf_offset_2 = sift_result.matches_2_1.size();

    if (surf_offset_2 > 0)
        for (std::size_t i = surf_offset_1; i < result->matches_1_2.size(); ++i)
            if (result->matches_1_2[i] >= 0)
                result->matches_1_2[i] += surf_offset_2;

    if (surf_offset_1 > 0)
        for (std::size_t i = surf_offset_2; i < result->matches_2_1.size(); ++i)
            if (result->matches_2_1[i] >= 0)
                result->matches_2_1[i] += surf_offset_1;

}

SFM_NAMESPACE_END
