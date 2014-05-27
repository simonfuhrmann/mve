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
    octree.get_points_per_level(&stats);
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
    octree.get_points_per_level(&stats);
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
    EXPECT_EQ(2, octree.get_num_nodes());
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
    EXPECT_EQ(2, octree.get_num_nodes());
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

TEST(OctreeTest, OctreeReadWriteEmpty)
{
    // Just an empty octree.
    std::stringstream ss_in("0");
    std::stringstream ss_out;

    fssr::Octree octree;
    octree.read_hierarchy(ss_in, false);
    octree.write_hierarchy(ss_out, false);
    EXPECT_EQ(ss_out.str(), ss_in.str());
}

TEST(OctreeTest, OctreeReadWriteRootOnly)
{
    // Root node with no children.
    std::stringstream ss_in("100000000");
    std::stringstream ss_out;

    fssr::Octree octree;
    octree.read_hierarchy(ss_in, false);
    octree.write_hierarchy(ss_out, false);
    EXPECT_EQ(ss_out.str(), ss_in.str());
}

TEST(OctreeTest, OctreeReadWriteHierarcy1)
{
    // More complicated hierarchy. Root node with two children.
    // Each child has another child.
    std::stringstream ss_in(
    //       root       child 1    child 2    child 1.1  child 2.1
        "1" "00100100" "00010000" "00000010" "00000000" "00000000");
    std::stringstream ss_out;

    fssr::Octree octree;
    octree.read_hierarchy(ss_in, false);
    octree.write_hierarchy(ss_out, false);
    EXPECT_EQ(ss_out.str(), ss_in.str());
}
