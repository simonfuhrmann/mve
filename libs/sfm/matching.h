/*
 * Feature matching.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_MATCHING_HEADER
#define SFM_MATCHING_HEADER

#include <vector>
#include <limits>

#include "defines.h"
#include "nearestneighbor.h"

SFM_NAMESPACE_BEGIN

/*
 * Feature matching options.
 */
struct MatchingOptions
{
    MatchingOptions();

    /**
     * The length of the descriptor. Typically 128 for SIFT, 64 for SURF.
     */
    int descriptor_length;

    /**
     * Requires that the ratio between the best and second best matching
     * distance is below some threshold. If this ratio is near 1, the match
     * is ambiguous. Defaults to 0.8. Set to 1.0 to disable test.
     */
    float lowe_ratio_threshold;

    /**
     * Does not accept matches with distances larger than this value.
     * This needs to be tuned to the descriptor and data type used.
     * Disabled by default.
     */
    float distance_threshold;
};

/**
 * Feature matching result reported as two lists, each with indices in the
 * other set. An unsuccessful match is indicated with a negative index.
 */
struct MatchingResult
{
    /* Matches from set 1 in set 2. */
    std::vector<int> matches_1_2;
    /* Matches from set 2 in set 1. */
    std::vector<int> matches_2_1;
};

/**
 * Matches all elements in set 1 to all elements in set 2 and vice versa.
 * It reports matching results in two lists with indices.
 * Unsuccessful matches are indicated with a negative index.
 */
template <typename T>
void
match_features (MatchingOptions const& options,
    T const* set_1, std::size_t set_1_size,
    T const* set_2, std::size_t set_2_size,
    MatchingResult* matches);

/**
 * This function removes inconsistent matches.
 * A consistent match of a feature F1 in the first image to
 * feature F2 in the second image requires that F2 also matches to F1.
 */
void
remove_inconsistent_matches (MatchingResult* matches);

/**
 * Function that counts the number of valid matches.
 */
int
count_consistent_matches (MatchingResult const& matches);

/* ---------------------------------------------------------------- */

inline
MatchingOptions::MatchingOptions (void)
{
    this->descriptor_length = 64;
    this->lowe_ratio_threshold = 0.8f;
    this->distance_threshold = std::numeric_limits<float>::max();
}

template <typename T>
void
oneway_match (MatchingOptions const& options,
    T const* set_1, std::size_t set_1_size,
    T const* set_2, std::size_t set_2_size,
    std::vector<int>* result)
{
    float const square_lowe_threshold = options.lowe_ratio_threshold
        * options.lowe_ratio_threshold;
    float const square_dist_threshold = options.distance_threshold
        * options.distance_threshold;
    NearestNeighbor<T> nn;
    nn.set_elements(set_2, set_2_size);
    nn.set_element_dimensions(options.descriptor_length);
    T const* query_pointer = set_1;
    for (std::size_t i = 0; i < set_1_size; ++i,
        query_pointer += options.descriptor_length)
    {
        typename NearestNeighbor<T>::Result nn_result;
        nn.find(query_pointer, &nn_result);
        if (nn_result.dist_1st_best > square_dist_threshold)
            continue;
        if (static_cast<float>(nn_result.dist_1st_best)
            / static_cast<float>(nn_result.dist_2nd_best)
            > square_lowe_threshold)
            continue;
        result->at(i) = nn_result.index_1st_best;
    }
}

template <typename T>
void
match_features (MatchingOptions const& options,
    T const* set_1, std::size_t set_1_size,
    T const* set_2, std::size_t set_2_size,
    MatchingResult* matches)
{
    matches->matches_1_2.clear();
    matches->matches_2_1.clear();
    matches->matches_1_2.resize(set_1_size, -1);
    matches->matches_2_1.resize(set_2_size, -1);
    if (set_1_size == 0 || set_2_size == 0)
        return;

    oneway_match(options, set_1, set_1_size,
        set_2, set_2_size, &matches->matches_1_2);
    oneway_match(options, set_2, set_2_size,
        set_1, set_1_size, &matches->matches_2_1);
}

SFM_NAMESPACE_END

#endif  /* SFM_MATCHING_HEADER */
