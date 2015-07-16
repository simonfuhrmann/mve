// Test cases for octree.
// Written by Simon Fuhrmann.

#include <sstream>
#include <gtest/gtest.h>

#include "fssr/octree.h"
#include "fssr/sample.h"

TEST(OctreeTest, TestEmptyOctreeOperations)
{
    fssr::Octree octree;
    EXPECT_EQ(0, octree.get_num_levels());
    EXPECT_EQ(0, octree.get_num_samples());
    EXPECT_EQ(0, octree.get_num_nodes());

    std::vector<std::size_t> stats;
    octree.get_samples_per_level(&stats);
    EXPECT_TRUE(stats.empty());
}

TEST(OctreeTest, TestOneSampleOctreeOperations)
{
    fssr::Sample s;
    s.pos = math::Vec3f(0.0f);
    s.scale = 1.0f;

    fssr::Octree octree;
    octree.insert_sample(s);
    EXPECT_EQ(1, octree.get_num_levels());
    EXPECT_EQ(1, octree.get_num_samples());
    EXPECT_EQ(1, octree.get_num_nodes());

    std::vector<std::size_t> stats;
    octree.get_samples_per_level(&stats);
    ASSERT_EQ(1, stats.size());
    EXPECT_EQ(1, stats[0]);
}

TEST(OctreeTest, TestTwoSamplesDescend)
{
    fssr::Sample s1, s2;
    s1.pos = math::Vec3f(0.0f);
    s1.scale = 1.0f;
    s2.pos = math::Vec3f(0.0f);
    s2.scale = 0.5f;

    fssr::Octree octree;
    octree.insert_sample(s1);
    octree.insert_sample(s2);

    EXPECT_EQ(2, octree.get_num_levels());
    EXPECT_EQ(2, octree.get_num_samples());
    EXPECT_EQ(9, octree.get_num_nodes());
}

TEST(OctreeTest, TestTwoSamplesExpand)
{
    fssr::Sample s1, s2;
    s1.pos = math::Vec3f(0.0f);
    s1.scale = 1.0f;
    s2.pos = math::Vec3f(0.0f);
    s2.scale = 2.0f;

    fssr::Octree octree;
    octree.insert_sample(s1);
    octree.insert_sample(s2);

    EXPECT_EQ(2, octree.get_num_levels());
    EXPECT_EQ(2, octree.get_num_samples());
    EXPECT_EQ(9, octree.get_num_nodes());
}

TEST(OctreeTest, TestTwoSamplesSameScale)
{
    fssr::Sample s1, s2;
    s1.pos = math::Vec3f(0.0f);
    s1.scale = 1.0f;
    s2.pos = math::Vec3f(0.0f);
    s2.scale = 1.0f;

    fssr::Octree octree;
    octree.insert_sample(s1);
    octree.insert_sample(s2);

    EXPECT_EQ(1, octree.get_num_levels());
    EXPECT_EQ(2, octree.get_num_samples());
    EXPECT_EQ(1, octree.get_num_nodes());
}

TEST(OctreeTest, TestInsertIntoOctants)
{
    fssr::Sample root;
    root.pos = math::Vec3f(0.0f);
    root.scale = 1.0f;

    fssr::Sample oct[8];
    for (int i = 0; i < 8; ++i)
    {
        oct[i].scale = 0.5f;
        oct[i].pos = math::Vec3f(
            (i & 1 ? -0.1f : 0.1f),
            (i & 2 ? -0.1f : 0.1f),
            (i & 4 ? -0.1f : 0.1f));
    }

    fssr::Octree octree;
    octree.insert_sample(root);
    for (int i = 0; i < 8; ++i)
        octree.insert_sample(oct[i]);

    EXPECT_EQ(2, octree.get_num_levels());
    EXPECT_EQ(9, octree.get_num_samples());
    EXPECT_EQ(9, octree.get_num_nodes());
}
