// Test cases for feature matching.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "sfm/matching.h"

TEST(MatchingTest, RemoveInconsistentMatches)
{
    sfm::Matching::Result result;
    result.matches_1_2.resize(4, -1);
    result.matches_2_1.resize(5, -1);
    result.matches_1_2[0] = 1;
    result.matches_1_2[2] = 3;
    result.matches_1_2[3] = 2;
    result.matches_2_1[0] = 3;
    result.matches_2_1[1] = 1;
    result.matches_2_1[3] = 2;
    sfm::Matching::remove_inconsistent_matches(&result);

    EXPECT_EQ(-1, result.matches_1_2[0]);
    EXPECT_EQ(-1, result.matches_1_2[1]);
    EXPECT_EQ(3, result.matches_1_2[2]);
    EXPECT_EQ(-1, result.matches_1_2[3]);

    EXPECT_EQ(-1, result.matches_2_1[0]);
    EXPECT_EQ(-1, result.matches_2_1[1]);
    EXPECT_EQ(-1, result.matches_2_1[2]);
    EXPECT_EQ(2, result.matches_2_1[3]);
    EXPECT_EQ(-1, result.matches_2_1[4]);
}
