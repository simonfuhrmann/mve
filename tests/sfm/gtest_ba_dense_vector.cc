// Test cases for SfM dense vector class.
// Written by Simon Fuhrmann, Fabian Langguth.

#include <gtest/gtest.h>

#include "sfm/ba_dense_vector.h"

TEST(BundleAdjustmentVectorMatrixTest, VectorAllocTest)
{
    sfm::ba::DenseVector<double> a(10, 0.0);
    EXPECT_EQ(10, a.size());
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        EXPECT_EQ(0.0, a[i]);
        EXPECT_EQ(0.0, a.at(i));
    }
    a.resize(20, 1.0);
    EXPECT_EQ(20, a.size());
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        EXPECT_EQ(1.0, a[i]);
        EXPECT_EQ(1.0, a.at(i));
    }
}

TEST(BundleAdjustmentVectorMatrixTest, VectorSubtractTest)
{
    sfm::ba::DenseVector<double> a(10, 0.0);
    sfm::ba::DenseVector<double> b(10, 0.0);
    sfm::ba::DenseVector<double> c(11, 0.0);
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        a[i] = static_cast<double>(i);
        b[i] = static_cast<double>(i) * 2.0;
    }
    EXPECT_THROW(a.subtract(c), std::exception);
    EXPECT_THROW(c.subtract(a), std::exception);

    sfm::ba::DenseVector<double> d = a.subtract(b);
    for (std::size_t i = 0; i < a.size(); ++i)
        EXPECT_EQ(-static_cast<double>(i), d[i]);

    sfm::ba::DenseVector<double> e, f;
    sfm::ba::DenseVector<double> g = e.subtract(f);
    EXPECT_EQ(0, g.size());
    EXPECT_TRUE(e == f);
}

TEST(BundleAdjustmentVectorMatrixTest, VectorAddTest)
{
    sfm::ba::DenseVector<double> a(10, 0.0);
    sfm::ba::DenseVector<double> b(10, 0.0);
    sfm::ba::DenseVector<double> c(11, 0.0);
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        a[i] = static_cast<double>(i);
        b[i] = static_cast<double>(i) * 2.0;
    }
    EXPECT_THROW(a.add(c), std::exception);
    EXPECT_THROW(c.add(a), std::exception);

    sfm::ba::DenseVector<double> d = a.add(b);
    for (std::size_t i = 0; i < a.size(); ++i)
        EXPECT_EQ(static_cast<double>(i) * 3.0, d[i]);
}

TEST(BundleAdjustmentVectorMatrixTest, VectorMultScalarTest)
{
    sfm::ba::DenseVector<double> a(10, 0.0);
    sfm::ba::DenseVector<double> b(10, 0.0);
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        a[i] = static_cast<double>(i);
        b[i] = static_cast<double>(i) * 2.0;
    }

    sfm::ba::DenseVector<double> c = a.multiply(2.0);

   EXPECT_TRUE(b == c);
}

TEST(BundleAdjustmentVectorMatrixTest, VectorDotProductTest)
{
    sfm::ba::DenseVector<double> a(10, 0.0);
    double dot_product = 0;
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        a[i] = static_cast<double>(i);
        dot_product += static_cast<double>(i) * static_cast<double>(i);
    }

   EXPECT_EQ(dot_product, a.dot(a));
}

TEST(BundleAdjustmentVectorMatrixTest, VectorEqualityTest)
{
    sfm::ba::DenseVector<int> a(10);
    sfm::ba::DenseVector<int> b(10);
    sfm::ba::DenseVector<int> c(11);
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        a[i] = i;
        b[i] = i * 2;
    }
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
    EXPECT_FALSE(a == c);
    EXPECT_FALSE(c == a);
    EXPECT_FALSE(b == c);
    EXPECT_FALSE(c == b);
    EXPECT_TRUE(b.subtract(a) == a);
}

TEST(BundleAdjustmentVectorMatrixTest, VectorIterationTest)
{
    sfm::ba::DenseVector<int> a(10, 0.0);
    for (std::size_t i = 0; i < a.size(); ++i)
        a[i] = static_cast<int>(i);

    int i = 0;
    for (int* p = a.begin(); p != a.end(); ++p, ++i)
        EXPECT_EQ(i, *p);
}
