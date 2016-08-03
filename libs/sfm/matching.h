/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_MATCHING_HEADER
#define SFM_MATCHING_HEADER

#include <vector>
#include <limits>

#include "math/defines.h"
#include "sfm/defines.h"
#include "sfm/nearest_neighbor.h"

SFM_NAMESPACE_BEGIN

class Matching
{
public:
    /**
     * Feature matching options.
     * There are no default values, all fields must be initialized.
     */
    struct Options
    {
        /**
         * The length of the descriptor. Typically 128 for SIFT, 64 for SURF.
         */
        int descriptor_length;

        /**
         * Requires that the ratio between the best and second best matching
         * distance is below some threshold. If this ratio is near 1, the match
         * is ambiguous. Good values are 0.8 for SIFT and 0.7 for SURF.
         * Set to 1.0 to disable the test.
         */
        float lowe_ratio_threshold;

        /**
         * Does not accept matches with distances larger than this value.
         * This needs to be tuned to the descriptor and data type used.
         * Set to FLOAT_MAX to disable the test.
         */
        float distance_threshold;
    };

    /**
     * Feature matching result reported as two lists, each with indices in the
     * other set. An unsuccessful match is indicated with a negative index.
     */
    struct Result
    {
        /* Matches from set 1 in set 2. */
        std::vector<int> matches_1_2;
        /* Matches from set 2 in set 1. */
        std::vector<int> matches_2_1;
    };

public:
    /**
     * Matches all elements in set 1 to all elements in set 2.
     * It reports as result for each element of set 1 to which element
     * in set 2 it maches. An unsuccessful match which did not pass
     * one of the thresholds is indicated with a negative index.
     */
    template <typename T>
    static void
    oneway_match (Options const& options,
        T const* set_1, int set_1_size,
        T const* set_2, int set_2_size,
        std::vector<int>* result);

    /**
     * Matches all elements in set 1 to all elements in set 2 and vice versa.
     * It reports matching results in two lists with indices.
     * Unsuccessful matches are indicated with a negative index.
     */
    template <typename T>
    static void
    twoway_match (Options const& options,
        T const* set_1, int set_1_size,
        T const* set_2, int set_2_size,
        Result* matches);

    /**
     * This function removes inconsistent matches.
     * A consistent match of a feature F1 in the first image to
     * feature F2 in the second image requires that F2 also matches to F1.
     */
    static void
    remove_inconsistent_matches (Result* matches);

    /**
     * Function that counts the number of valid matches.
     */
    static int
    count_consistent_matches (Result const& matches);

    /**
     * Combines matching results of different descriptors.
     */
    static void
    combine_results(Result const& sift_result,
        Result const& surf_result, Matching::Result* result);
};

/* ---------------------------------------------------------------- */

template <typename T>
void
Matching::oneway_match (Options const& options,
    T const* set_1, int set_1_size,
    T const* set_2, int set_2_size,
    std::vector<int>* result)
{
    result->clear();
    result->resize(set_1_size, -1);
    if (set_1_size == 0 || set_2_size == 0)
        return;

    float const square_lowe_thres = MATH_POW2(options.lowe_ratio_threshold);
    float const square_dist_thres = MATH_POW2(options.distance_threshold);
    NearestNeighbor<T> nn;
    nn.set_elements(set_2);
    nn.set_num_elements(set_2_size);
    nn.set_element_dimensions(options.descriptor_length);

    for (int i = 0; i < set_1_size; ++i)
    {
        typename NearestNeighbor<T>::Result nn_result;
        T const* query_pointer = set_1 + i * options.descriptor_length;
        nn.find(query_pointer, &nn_result);
        if (nn_result.dist_1st_best > square_dist_thres)
            continue;
        if (static_cast<float>(nn_result.dist_1st_best)
            / static_cast<float>(nn_result.dist_2nd_best)
            > square_lowe_thres)
            continue;
        result->at(i) = nn_result.index_1st_best;
    }
}

template <typename T>
void
Matching::twoway_match (Options const& options,
    T const* set_1, int set_1_size,
    T const* set_2, int set_2_size,
    Result* matches)
{
    Matching::oneway_match(options, set_1, set_1_size,
        set_2, set_2_size, &matches->matches_1_2);
    Matching::oneway_match(options, set_2, set_2_size,
        set_1, set_1_size, &matches->matches_2_1);
}

SFM_NAMESPACE_END

#endif  /* SFM_MATCHING_HEADER */
