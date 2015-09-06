#include <gtest/gtest.h>

#include "fssr/octree.h"

namespace
{
    //                  0
    //                  |
    //    1   2   3   4   5   6   7   8
    //            |
    //  9 10 11 12 13 14 15 16
    fssr::Octree::Node*
    get_test_hierarchy (void)
    {
        fssr::Octree::Node* root = new fssr::Octree::Node();
        root->mc_index = 0;

        root->children = new fssr::Octree::Node[8];
        for (int i = 0; i < 8; ++i)
        {
            root->children[i].parent = root;
            root->children[i].mc_index = i + 1;
        }

        root->children[2].children = new fssr::Octree::Node[8];
        for (int i = 0; i < 8; ++i)
        {
            root->children[2].children[i].parent = root->children + 2;
            root->children[2].children[i].mc_index = i + 9;
        }

        return root;
    }
}

TEST(OctreeIteratorTest, NextLeafTest)
{
    fssr::Octree::Node* root = get_test_hierarchy();
    fssr::Octree::Iterator iter;
    iter.root = root;
    std::vector<int> ordering;
    for (iter.first_leaf(); iter.current != nullptr; iter.next_leaf())
        ordering.push_back(iter.current->mc_index);
    delete root;

    ASSERT_EQ(15, ordering.size());
    EXPECT_EQ(1, ordering[0]);
    EXPECT_EQ(2, ordering[1]);
    EXPECT_EQ(9, ordering[2]);
    EXPECT_EQ(10, ordering[3]);
    EXPECT_EQ(11, ordering[4]);
    EXPECT_EQ(12, ordering[5]);
    EXPECT_EQ(13, ordering[6]);
    EXPECT_EQ(14, ordering[7]);
    EXPECT_EQ(15, ordering[8]);
    EXPECT_EQ(16, ordering[9]);
    EXPECT_EQ(4, ordering[10]);
    EXPECT_EQ(5, ordering[11]);
    EXPECT_EQ(6, ordering[12]);
    EXPECT_EQ(7, ordering[13]);
    EXPECT_EQ(8, ordering[14]);
}

TEST(OctreeIteratorTest, NextNodeTest)
{
    fssr::Octree::Node* root = get_test_hierarchy();
    fssr::Octree::Iterator iter;
    iter.root = root;
    std::vector<int> ordering;
    for (iter.first_node(); iter.current != nullptr; iter.next_node())
        ordering.push_back(iter.current->mc_index);
    delete root;

    ASSERT_EQ(17, ordering.size());
    EXPECT_EQ(0, ordering[0]);
    EXPECT_EQ(1, ordering[1]);
    EXPECT_EQ(2, ordering[2]);
    EXPECT_EQ(3, ordering[3]);
    EXPECT_EQ(9, ordering[4]);
    EXPECT_EQ(10, ordering[5]);
    EXPECT_EQ(11, ordering[6]);
    EXPECT_EQ(12, ordering[7]);
    EXPECT_EQ(13, ordering[8]);
    EXPECT_EQ(14, ordering[9]);
    EXPECT_EQ(15, ordering[10]);
    EXPECT_EQ(16, ordering[11]);
    EXPECT_EQ(4, ordering[12]);
    EXPECT_EQ(5, ordering[13]);
    EXPECT_EQ(6, ordering[14]);
    EXPECT_EQ(7, ordering[15]);
    EXPECT_EQ(8, ordering[16]);
}

TEST(OctreeIteratorTest, NextBranchTest)
{
    fssr::Octree::Node* root = get_test_hierarchy();
    fssr::Octree::Iterator iter;
    iter.root = root;
    iter.first_leaf();
    iter.current = root->children;

    std::vector<int> ordering;
    for (; iter.current != nullptr; iter.next_branch())
        ordering.push_back(iter.current->mc_index);
    delete root;

    ASSERT_EQ(8, ordering.size());
    EXPECT_EQ(1, ordering[0]);
    EXPECT_EQ(2, ordering[1]);
    EXPECT_EQ(3, ordering[2]);
    EXPECT_EQ(4, ordering[3]);
    EXPECT_EQ(5, ordering[4]);
    EXPECT_EQ(6, ordering[5]);
    EXPECT_EQ(7, ordering[6]);
    EXPECT_EQ(8, ordering[7]);
}
