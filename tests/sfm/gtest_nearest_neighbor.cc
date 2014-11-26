// Test cases for nearest neighbor search.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "util/aligned_memory.h"
#include "sfm/nearest_neighbor.h"

TEST(NearestNeighborTest, TestSingnedShort)
{
    util::AlignedMemory<short> elements;
    elements.allocate(8 * 4);
    short* ptr = elements.begin();

    // Fill elements.
    (*ptr++) = 127;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;

    (*ptr++) = 0;
    (*ptr++) = -127;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;

    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 90;
    (*ptr++) = 90;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;

    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 90;
    (*ptr++) = -90;
    (*ptr++) = 0;
    (*ptr++) = 0;

    sfm::NearestNeighbor<short>::Result result;
    sfm::NearestNeighbor<short> nn;
    nn.set_elements(elements.begin(), 4);
    nn.set_element_dimensions(8);

    short query1[8] = { 127, 0, 0, 0, 0, 0, 0, 0 };
    nn.find(query1, &result);
    EXPECT_EQ(0, result.dist_1st_best);
    EXPECT_EQ(32258, result.dist_2nd_best);

    short query2[8] = { -127, 0, 0, 0, 0, 0, 0, 0 };
    nn.find(query2, &result);
    EXPECT_EQ(32258, result.dist_1st_best);
    EXPECT_EQ(32258, result.dist_2nd_best);

    short query3[8] = { 0, 0, 90, 90, 0, 0, 0, 0 };
    nn.find(query3, &result);
    EXPECT_EQ(0, result.dist_1st_best);
    EXPECT_EQ(32258, result.dist_2nd_best);

    short query4[8] = { 0, 0, 90, 0, 0, -90, 0, 0 };
    nn.find(query4, &result);
    EXPECT_EQ(16058, result.dist_1st_best);
    EXPECT_EQ(16058, result.dist_2nd_best);

    short query5[8] = { 0, 0, 90, 0, 0, 90, 0, 0 };
    nn.find(query5, &result);
    EXPECT_EQ(16058, result.dist_1st_best);
    EXPECT_EQ(32258, result.dist_2nd_best);
}

TEST(NearestNeighborTest, TestUnsingnedShort)
{
    util::AlignedMemory<unsigned short> elements;
    elements.allocate(8 * 2);
    unsigned short* ptr = elements.begin();

    // Fill elements.
    (*ptr++) = 255;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;

    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 181;
    (*ptr++) = 181;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;
    (*ptr++) = 0;

    sfm::NearestNeighbor<unsigned short>::Result result;
    sfm::NearestNeighbor<unsigned short> nn;
    nn.set_elements(elements.begin(), 2);
    nn.set_element_dimensions(8);

    unsigned short query1[8] = { 255, 0, 0, 0, 0, 0, 0, 0 };
    nn.find(query1, &result);
    EXPECT_EQ(0, result.dist_1st_best);
    EXPECT_EQ(65534, result.dist_2nd_best);

    unsigned short query2[8] = { 0, 0, 255, 0, 0, 0, 0, 0 };
    nn.find(query2, &result);
    EXPECT_EQ(37740, result.dist_1st_best);
    EXPECT_EQ(65534, result.dist_2nd_best);

    unsigned short query3[8] = { 0, 0, 181, 181, 0, 0, 0, 0 };
    nn.find(query3, &result);
    EXPECT_EQ(0, result.dist_1st_best);
    EXPECT_EQ(65534, result.dist_2nd_best);
}
