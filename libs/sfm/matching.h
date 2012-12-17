/*
 * Feature matching.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_MATCHING_HEADER
#define SFM_MATCHING_HEADER

#include <vector>
#include <limits>

#include "defines.h"

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
 * Matches all elements in set 1 to all elements in set 2 and reports
 * matching results in two lists with indices. Unsuccessful matches
 * are indicated with a negative index.
 */
template <typename T>
void
match_features (MatchingOptions const& options,
    T const* set_1, std::size_t set_1_size,
    T const* set_2, std::size_t set_2_size,
    MatchingResult* matches);

/**
 * This function enforces that matches are consistent. Inconsistent matches
 * are removed. A consistent match of a feature F1 in the first image to
 * feature F2 in the second image requires that F2 also matches to F1.
 */
void
remove_inconsistent_matches (MatchingResult* matches);

/* ---------------------------------------------------------------- */

inline
MatchingOptions::MatchingOptions (void)
{
    this->descriptor_length = 64;
    this->lowe_ratio_threshold = 0.8f;
    this->distance_threshold = std::numeric_limits<float>::max();
}

SFM_NAMESPACE_END

#endif  /* SFM_MATCHING_HEADER */
