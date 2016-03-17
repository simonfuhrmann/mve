// Test cases for nearest neighbor search.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "util/aligned_memory.h"
#include "sfm/nearest_neighbor.h"

TEST(NearestNeighborTest, TestSingnedShort)
{
    util::AlignedMemory<short> elements;
    elements.resize(8 * 4);
    short* ptr = elements.data();

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
    nn.set_elements(elements.data());
    nn.set_num_elements(4);
    nn.set_element_dimensions(8);

    short query1[8] = { 127, 0, 0, 0, 0, 0, 0, 0 };
    nn.find(query1, &result);
    EXPECT_EQ(0, result.dist_1st_best);
    EXPECT_EQ(32258, result.dist_2nd_best);
    EXPECT_EQ(0, result.index_1st_best);
    EXPECT_EQ(3, result.index_2nd_best);

    short query2[8] = { -127, 0, 0, 0, 0, 0, 0, 0 };
    nn.find(query2, &result);
    EXPECT_EQ(32258, result.dist_1st_best);
    EXPECT_EQ(32258, result.dist_2nd_best);
    EXPECT_EQ(3, result.index_1st_best);
    EXPECT_EQ(2, result.index_2nd_best);

    short query3[8] = { 0, 0, 90, 90, 0, 0, 0, 0 };
    nn.find(query3, &result);
    EXPECT_EQ(0, result.dist_1st_best);
    EXPECT_EQ(32258, result.dist_2nd_best);
    EXPECT_EQ(2, result.index_1st_best);
    EXPECT_EQ(3, result.index_2nd_best);

    short query4[8] = { 0, 0, 90, 0, 0, -90, 0, 0 };
    nn.find(query4, &result);
    EXPECT_EQ(16058, result.dist_1st_best);
    EXPECT_EQ(16058, result.dist_2nd_best);
    EXPECT_EQ(3, result.index_1st_best);
    EXPECT_EQ(2, result.index_2nd_best);

    short query5[8] = { 0, 0, 90, 0, 0, 90, 0, 0 };
    nn.find(query5, &result);
    EXPECT_EQ(16058, result.dist_1st_best);
    EXPECT_EQ(32258, result.dist_2nd_best);
    EXPECT_EQ(2, result.index_1st_best);
    EXPECT_EQ(1, result.index_2nd_best);
}

TEST(NearestNeighborTest, TestUnsignedShort)
{
    util::AlignedMemory<unsigned short> elements;
    elements.resize(8 * 2);
    unsigned short* ptr = elements.data();

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
    nn.set_elements(elements.data());
    nn.set_num_elements(2);
    nn.set_element_dimensions(8);

    unsigned short query1[8] = { 255, 0, 0, 0, 0, 0, 0, 0 };
    nn.find(query1, &result);
    EXPECT_EQ(0, result.dist_1st_best);
    EXPECT_EQ(65534, result.dist_2nd_best);
    EXPECT_EQ(0, result.index_1st_best);
    EXPECT_EQ(1, result.index_2nd_best);

    unsigned short query2[8] = { 0, 0, 255, 0, 0, 0, 0, 0 };
    nn.find(query2, &result);
    EXPECT_EQ(37740, result.dist_1st_best);
    EXPECT_EQ(65534, result.dist_2nd_best);
    EXPECT_EQ(1, result.index_1st_best);
    EXPECT_EQ(0, result.index_2nd_best);

    unsigned short query3[8] = { 0, 0, 181, 181, 0, 0, 0, 0 };
    nn.find(query3, &result);
    EXPECT_EQ(0, result.dist_1st_best);
    EXPECT_EQ(65534, result.dist_2nd_best);
    EXPECT_EQ(1, result.index_1st_best);
    EXPECT_EQ(0, result.index_2nd_best);
}

TEST(NearestNeighborTest, TestFloat)
{
    util::AlignedMemory<float> elements;
    elements.resize(4 * 3);
    float* ptr = elements.data();

    // Fill elements.
    (*ptr++) = 1.0f;
    (*ptr++) = 0.0f;
    (*ptr++) = 0.0f;
    (*ptr++) = 0.0f;

    (*ptr++) = 0.0f;
    (*ptr++) = 1.0f;
    (*ptr++) = 0.0f;
    (*ptr++) = 0.0f;

    (*ptr++) = 0.0f;
    (*ptr++) = 0.0f;
    (*ptr++) = 1.0f;
    (*ptr++) = 0.0f;

    sfm::NearestNeighbor<float>::Result result;
    sfm::NearestNeighbor<float> nn;
    nn.set_elements(elements.data());
    nn.set_num_elements(3);
    nn.set_element_dimensions(4);

    float query1[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
    nn.find(query1, &result);
    EXPECT_EQ(0.0f, result.dist_1st_best);
    EXPECT_EQ(2.0f, result.dist_2nd_best);
    EXPECT_EQ(0, result.index_1st_best);
    EXPECT_EQ(2, result.index_2nd_best);

    float query2[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
    nn.find(query2, &result);
    EXPECT_EQ(0.0f, result.dist_1st_best);
    EXPECT_EQ(2.0f, result.dist_2nd_best);
    EXPECT_EQ(1, result.index_1st_best);
    EXPECT_EQ(2, result.index_2nd_best);

    float query3[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
    nn.find(query3, &result);
    EXPECT_EQ(0.0f, result.dist_1st_best);
    EXPECT_EQ(2.0f, result.dist_2nd_best);
    EXPECT_EQ(2, result.index_1st_best);
    EXPECT_EQ(1, result.index_2nd_best);

    float query4[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    nn.find(query4, &result);
    EXPECT_EQ(2.0f, result.dist_1st_best);
    EXPECT_EQ(2.0f, result.dist_2nd_best);
    EXPECT_EQ(2, result.index_1st_best);
    EXPECT_EQ(1, result.index_2nd_best);
}
