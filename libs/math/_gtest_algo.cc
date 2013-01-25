/*
 * Test cases for algo routines.
 * Written by Simon Fuhrmann.
 */

#include <gtest/gtest.h>

#include "math/matrix.h"
#include "math/algo.h"
#include "math/permute.h"

TEST(AlgoTest, GaussianTest)
{
    using namespace math;

    EXPECT_EQ(algo::gaussian(0.0f, 1.0f), 1.0f);

    EXPECT_TRUE(MATH_EPSILON_EQ(algo::gaussian(1.0f, 1.0f),
        0.606530659712633f, 0.00000000000001f));
    EXPECT_TRUE(MATH_EPSILON_EQ(algo::gaussian(-1.0f, 1.0f),
        0.606530659712633f, 0.00000000000001f));
    EXPECT_TRUE(MATH_EPSILON_EQ(algo::gaussian(2.0f, 1.0f),
        0.135335283236613f, 0.00000000000001f));
    EXPECT_TRUE(MATH_EPSILON_EQ(algo::gaussian(-2.0f, 1.0f),
        0.135335283236613f, 0.00000000000001f));

    EXPECT_TRUE(MATH_EPSILON_EQ(algo::gaussian(1.0f, 2.0f),
        0.882496902584595f, 0.00000000000001f));
    EXPECT_TRUE(MATH_EPSILON_EQ(algo::gaussian(-1.0f, 2.0f),
        0.882496902584595f, 0.00000000000001f));
    EXPECT_TRUE(MATH_EPSILON_EQ(algo::gaussian(2.0f, 2.0f),
        0.606530659712633f, 0.00000000000001f));
    EXPECT_TRUE(MATH_EPSILON_EQ(algo::gaussian(-2.0f, 2.0f),
        0.606530659712633f, 0.00000000000001f));
}

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

    std::vector<bool> dlist;
    dlist.push_back(true);
    dlist.push_back(true);
    dlist.push_back(false);
    dlist.push_back(true);
    dlist.push_back(false);
    dlist.push_back(false);
    dlist.push_back(false);
    dlist.push_back(true);
    dlist.push_back(false);
    dlist.push_back(false);
    dlist.push_back(true);
    dlist.push_back(true);

    math::algo::vector_clean(vec, dlist);
    for (int i = 0; i < (int)vec.size(); ++i)
        EXPECT_EQ(vec[i], i);

    vec.clear();
    dlist.clear();
    math::algo::vector_clean(vec, dlist);
    EXPECT_TRUE(vec.empty());

    vec.push_back(1);
    dlist.push_back(true);
    math::algo::vector_clean(vec, dlist);
    EXPECT_TRUE(vec.empty());

    vec.clear();
    dlist.clear();
    vec.push_back(21);
    dlist.push_back(false);
    math::algo::vector_clean(vec, dlist);
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
