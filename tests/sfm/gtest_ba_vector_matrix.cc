// Test cases for SfM vector and matrix classes.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "sfm/ba_dense_vector.h"
#include "sfm/ba_sparse_matrix.h"

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

TEST(BundleAdjustmentVectorMatrixTest, MatrixSetFromTripletsTest)
{
    typedef sfm::ba::SparseMatrix<double> SparseMatrix;

    SparseMatrix::Triplets triplets;
    triplets.emplace_back(0, 2, 1.0);
    triplets.emplace_back(1, 1, 2.0);
    triplets.emplace_back(2, 1, 3.0);

    /* Test constructor, num_* functions. */
    SparseMatrix m(3, 4);
    EXPECT_EQ(3, m.num_rows());
    EXPECT_EQ(4, m.num_cols());
    EXPECT_EQ(0, m.num_non_zero());

    m.set_from_triplets(&triplets);
    EXPECT_EQ(3, m.num_non_zero());
    EXPECT_EQ(1.0, m.begin()[0]);
    EXPECT_EQ(2.0, m.begin()[1]);
    EXPECT_EQ(3.0, m.begin()[2]);

    triplets.clear();
    triplets.emplace_back(2, 1, 3.0);
    triplets.emplace_back(1, 1, 2.0);
    triplets.emplace_back(0, 2, 1.0);
    m.set_from_triplets(&triplets);
    EXPECT_EQ(3, m.num_non_zero());
    EXPECT_EQ(1.0, m.begin()[0]);
    EXPECT_EQ(2.0, m.begin()[1]);
    EXPECT_EQ(3.0, m.begin()[2]);

    triplets.clear();
    triplets.emplace_back(2, 1, 3.0);
    triplets.emplace_back(1, 2, 2.0);
    triplets.emplace_back(1, 1, 1.0);
    m.set_from_triplets(&triplets);
    EXPECT_EQ(3, m.num_non_zero());
    EXPECT_EQ(1.0, m.begin()[0]);
    EXPECT_EQ(2.0, m.begin()[1]);
    EXPECT_EQ(3.0, m.begin()[2]);
}

TEST(BundleAdjustmentVectorMatrixTest, MatrixTransposeTest)
{
    typedef sfm::ba::SparseMatrix<double> SparseMatrix;
    SparseMatrix m1(5, 4);
    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(1, 2, 3.0);
        triplets.emplace_back(2, 0, 1.0);
        triplets.emplace_back(2, 2, 5.0);
        m1.set_from_triplets(&triplets);
    }
    EXPECT_EQ(5, m1.num_rows());
    EXPECT_EQ(4, m1.num_cols());
    EXPECT_EQ(3, m1.num_non_zero());

    SparseMatrix m2 = m1.transpose();
    EXPECT_EQ(4, m2.num_rows());
    EXPECT_EQ(5, m2.num_cols());
    EXPECT_EQ(3, m2.num_non_zero());
    EXPECT_EQ(1.0, m2.begin()[0]);
    EXPECT_EQ(3.0, m2.begin()[1]);
    EXPECT_EQ(5.0, m2.begin()[2]);
}

TEST(BundleAdjustmentVectorMatrixTest, MatrixSubtractTest)
{
    typedef sfm::ba::SparseMatrix<int> SparseMatrix;
    SparseMatrix m1(2, 4);
    SparseMatrix m2(2, 4);
    SparseMatrix m3(4, 2);

    EXPECT_NO_THROW(m1.subtract(m2));
    EXPECT_THROW(m1.subtract(m3), std::exception);

    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(0, 1, 1);
        triplets.emplace_back(0, 3, 4);
        triplets.emplace_back(1, 2, 3);
        m1.set_from_triplets(&triplets);
    }

    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(0, 0, 1);
        triplets.emplace_back(1, 1, 2);
        triplets.emplace_back(1, 2, 3);
        m2.set_from_triplets(&triplets);
    }

    m3 = m1.subtract(m2);
    EXPECT_EQ(2, m3.num_rows());
    EXPECT_EQ(4, m3.num_cols());
    EXPECT_EQ(5, m3.num_non_zero());

    EXPECT_EQ(-1, m3.begin()[0]);
    EXPECT_EQ(1, m3.begin()[1]);
    EXPECT_EQ(4, m3.begin()[2]);
    EXPECT_EQ(-2, m3.begin()[3]);
    EXPECT_EQ(0, m3.begin()[4]);
}

TEST(BundleAdjustmentVectorMatrixTest, MatrixMatrixMultiplyTest)
{
    typedef sfm::ba::SparseMatrix<int> SparseMatrix;
    SparseMatrix m1(2, 4);
    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(1, 0, 1);
        triplets.emplace_back(2, 1, 3);
        triplets.emplace_back(3, 0, 1);
        m1.set_from_triplets(&triplets);
    }

    SparseMatrix m2(4, 3);
    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(0, 2, 4);
        triplets.emplace_back(1, 1, 5);
        triplets.emplace_back(2, 1, 2);
        triplets.emplace_back(2, 2, 2);
        triplets.emplace_back(2, 3, 2);
        m2.set_from_triplets(&triplets);
    }

    SparseMatrix m3 = m1.multiply(m2);
    EXPECT_EQ(2, m3.num_rows());
    EXPECT_EQ(3, m3.num_cols());
    EXPECT_EQ(4, m3.num_non_zero());

    EXPECT_EQ(12, m3.begin()[0]);
    EXPECT_EQ(5, m3.begin()[1]);
    EXPECT_EQ(4, m3.begin()[2]);
    EXPECT_EQ(6, m3.begin()[3]);

    SparseMatrix m4(5, 3);
    EXPECT_THROW(m1.multiply(m4), std::exception);
}

TEST(BundleAdjustmentVectorMatrixTest, MatrixVectorFormatsTest)
{
    typedef sfm::ba::SparseMatrix<int> SparseMatrix;
    typedef sfm::ba::DenseVector<int> DenseVector;

    SparseMatrix m1(3, 4);
    SparseMatrix m2(4, 3);
    DenseVector v1(4, 0);
    EXPECT_NO_THROW(m1.multiply(v1));
    EXPECT_THROW(m2.multiply(v1), std::exception);
}

TEST(BundleAdjustmentVectorMatrixTest, MatrixVectorMultiplyTest)
{
    typedef sfm::ba::SparseMatrix<int> SparseMatrix;
    typedef sfm::ba::DenseVector<int> DenseVector;

    SparseMatrix m1(3, 4);
    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(1, 0, 1);
        triplets.emplace_back(3, 0, 1);
        triplets.emplace_back(2, 1, 3);
        triplets.emplace_back(0, 2, 4);
        m1.set_from_triplets(&triplets);
    }

    DenseVector v1(4, 0);
    v1[1] = 1;
    v1[2] = 2;

    DenseVector ret = m1.multiply(v1);
    EXPECT_EQ(3, ret.size());
    EXPECT_EQ(1, ret[0]);
    EXPECT_EQ(6, ret[1]);
    EXPECT_EQ(0, ret[2]);
}

TEST(BundleAdjustmentVectorMatrixTest, MatrixMultiplyDiagonalTest)
{
    typedef sfm::ba::SparseMatrix<int> SparseMatrix;
    SparseMatrix m1(3, 4);
    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(1, 1, 2);
        triplets.emplace_back(2, 0, 4);
        m1.set_from_triplets(&triplets);
    }

    EXPECT_EQ(2, m1.begin()[0]);
    EXPECT_EQ(4, m1.begin()[1]);
    m1.mult_diagonal(3);
    EXPECT_EQ(6, m1.begin()[0]);
    EXPECT_EQ(4, m1.begin()[1]);

    SparseMatrix m2(3, 4);
    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(1, 1, 2);
        triplets.emplace_back(2, 0, 4);
        m2.set_from_triplets(&triplets);
    }

    EXPECT_EQ(2, m2.begin()[0]);
    EXPECT_EQ(4, m2.begin()[1]);
    m2.mult_diagonal(3);
    EXPECT_EQ(6, m2.begin()[0]);
    EXPECT_EQ(4, m2.begin()[1]);
}
