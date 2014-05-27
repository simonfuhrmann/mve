// Test cases for ISO octree.
// Written by Simon Fuhrmann.

#include <iostream>
#include <gtest/gtest.h>

#include "fssr/iso_octree.h"
#include "fssr/sample.h"

#if 0
TEST(IsoOctreeTest, TempTest)
{
    fssr::Octree::NodePath path;
    fssr::VoxelIndex index;

    path.level = 0;
    path.path = 0;
    index.from_path_and_corner(path, 1);
    std::cout << index.index << std::endl;

    path.level = 1;
    path.path = 1;
    index.from_path_and_corner(path, 1);
    std::cout << index.index << std::endl;

    path.level = 2;
    path.path = 9;
    index.from_path_and_corner(path, 1);
    std::cout << index.index << std::endl;


    path.level = 0;
    path.path = 0;
    index.from_path_and_corner(path, 2);
    std::cout << index.index << std::endl;

    path.level = 1;
    path.path = 2;
    index.from_path_and_corner(path, 2);
    std::cout << index.index << std::endl;

    path.level = 2;
    path.path = 18;
    index.from_path_and_corner(path, 2);
    std::cout << index.index << std::endl;
}

TEST(IsoOctreeTest, Temp2Test)
{
    fssr::Octree::NodePath path;

    path.level = 1;
    path.path = 0;
    fssr::VoxelIndex index1;
    index1.from_path_and_corner(path, 7);
    std::cout << index1.index << std::endl;

    path.level = 2;
    path.path = 14;
    fssr::VoxelIndex index2;
    index2.from_path_and_corner(path, 6);
    std::cout << index2.index << std::endl;
}
#endif
