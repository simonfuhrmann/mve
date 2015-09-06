/*
 * Test cases for algo routines.
 * Written by Simon Fuhrmann.
 */

#include <gtest/gtest.h>

#include "math/algo.h"
#include "math/permute.h"

TEST(AlgoTest, PermutationTest)
{
    /* Test permutations. */
    std::vector<float> v;
    v.push_back(0.5f);
    v.push_back(1.5f);
    v.push_back(2.5f);
    v.push_back(3.5f);
    v.push_back(4.5f);
    v.push_back(5.5f);

    std::vector<std::size_t> p;
    p.push_back(0); // 0
    p.push_back(4); // 1
    p.push_back(5); // 2
    p.push_back(2); // 3
    p.push_back(1); // 4
    p.push_back(3); // 5

    std::vector<float> bak(v);
    math::algo::permute_reloc<float, std::size_t>(v, p);
    for (std::size_t i = 0; i < p.size(); ++i)
        EXPECT_EQ(v[p[i]], bak[i]);

    v = bak;
    math::algo::permute_math<float, std::size_t>(v, p);
    for (std::size_t i = 0; i < p.size(); ++i)
        EXPECT_EQ(v[i], bak[p[i]]);
}

TEST(AlgoTest, VectorCleanTest)
{
    std::vector<int> vec;
    vec.push_back(99);
    vec.push_back(98);
    vec.push_back(0);
    vec.push_back(97);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(96);
    vec.push_back(4);
    vec.push_back(5);
    vec.push_back(95);
    vec.push_back(94);

    std::vector<bool> delete_list;
    delete_list.push_back(true);
    delete_list.push_back(true);
    delete_list.push_back(false);
    delete_list.push_back(true);
    delete_list.push_back(false);
    delete_list.push_back(false);
    delete_list.push_back(false);
    delete_list.push_back(true);
    delete_list.push_back(false);
    delete_list.push_back(false);
    delete_list.push_back(true);
    delete_list.push_back(true);

    math::algo::vector_clean(delete_list, &vec);
    for (int i = 0; i < (int)vec.size(); ++i)
        EXPECT_EQ(vec[i], i);

    vec.clear();
    delete_list.clear();
    math::algo::vector_clean(delete_list, &vec);
    EXPECT_TRUE(vec.empty());

    vec.push_back(1);
    delete_list.push_back(true);
    math::algo::vector_clean(delete_list, &vec);
    EXPECT_TRUE(vec.empty());

    vec.clear();
    delete_list.clear();
    vec.push_back(21);
    delete_list.push_back(false);
    math::algo::vector_clean(delete_list, &vec);
    ASSERT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], 21);
}

TEST(AlgoTest, MaxMinElementIdTest)
{
    {
        float f[5] = { 1.0f, 0.5f, 0.0f, 0.2f, 0.4f };
        std::size_t id = math::algo::min_element_id(f, f+5);
        EXPECT_EQ(id, 2);
        id = math::algo::max_element_id(f, f+5);
        EXPECT_EQ(id, 0);
    }

    {
        float f[5] = { -1.0f, 0.5f, 0.0f, 0.2f, 1.4f };
        std::size_t id = math::algo::min_element_id(f, f+5);
        EXPECT_EQ(id, 0);
        id = math::algo::max_element_id(f, f+5);
        EXPECT_EQ(id, 4);
    }

    {
        float f[5] = { 1.0f, 0.5f, 1.1f, 0.2f, -0.4f };
        std::size_t id = math::algo::min_element_id(f, f+5);
        EXPECT_EQ(id, 4);
        id = math::algo::max_element_id(f, f+5);
        EXPECT_EQ(id, 2);
    }
}

TEST(AlgoTest, SortValues)
{
    int a, b, c;

    a = 1; b = 2; c = 3;
    math::algo::sort_values(&a, &b, &c);
    EXPECT_EQ(1, a); EXPECT_EQ(2, b); EXPECT_EQ(3, c);

    a = 1; b = 3; c = 2;
    math::algo::sort_values(&a, &b, &c);
    EXPECT_EQ(1, a); EXPECT_EQ(2, b); EXPECT_EQ(3, c);

    a = 2; b = 1; c = 3;
    math::algo::sort_values(&a, &b, &c);
    EXPECT_EQ(1, a); EXPECT_EQ(2, b); EXPECT_EQ(3, c);

    a = 2; b = 3; c = 1;
    math::algo::sort_values(&a, &b, &c);
    EXPECT_EQ(1, a); EXPECT_EQ(2, b); EXPECT_EQ(3, c);

    a = 3; b = 1; c = 2;
    math::algo::sort_values(&a, &b, &c);
    EXPECT_EQ(1, a); EXPECT_EQ(2, b); EXPECT_EQ(3, c);

    a = 3; b = 2; c = 1;
    math::algo::sort_values(&a, &b, &c);
    EXPECT_EQ(1, a); EXPECT_EQ(2, b); EXPECT_EQ(3, c);
}

TEST(AlgoTest, BinarySearch)
{
    std::vector<std::pair<int, int> > vector;
    EXPECT_EQ(nullptr, math::algo::binary_search(vector, 0));

    vector.clear();
    vector.push_back(std::make_pair(1, 101));
    EXPECT_EQ(nullptr, math::algo::binary_search(vector, 0));
    EXPECT_EQ(&vector[0].second, math::algo::binary_search(vector, 1));
    EXPECT_EQ(nullptr, math::algo::binary_search(vector, 2));

    vector.clear();
    vector.push_back(std::make_pair(1, 101));
    vector.push_back(std::make_pair(2, 102));
    EXPECT_EQ(nullptr, math::algo::binary_search(vector, 0));
    EXPECT_EQ(&vector[0].second, math::algo::binary_search(vector, 1));
    EXPECT_EQ(&vector[1].second, math::algo::binary_search(vector, 2));
    EXPECT_EQ(nullptr, math::algo::binary_search(vector, 3));

    vector.clear();
    vector.push_back(std::make_pair(0, 100));
    vector.push_back(std::make_pair(1, 101));
    vector.push_back(std::make_pair(2, 102));
    vector.push_back(std::make_pair(3, 103));
    vector.push_back(std::make_pair(4, 104));
    EXPECT_EQ(nullptr, math::algo::binary_search(vector, -1));
    EXPECT_EQ(&vector[0].second, math::algo::binary_search(vector, 0));
    EXPECT_EQ(&vector[1].second, math::algo::binary_search(vector, 1));
    EXPECT_EQ(&vector[2].second, math::algo::binary_search(vector, 2));
    EXPECT_EQ(&vector[3].second, math::algo::binary_search(vector, 3));
    EXPECT_EQ(&vector[4].second, math::algo::binary_search(vector, 4));
    EXPECT_EQ(nullptr, math::algo::binary_search(vector, 5));
}
