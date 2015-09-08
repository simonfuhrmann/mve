// Test cases for SfM sparse matrix class.
// Written by Simon Fuhrmann, Fabian Langguth.

#include <gtest/gtest.h>

#include "sfm/ba_dense_vector.h"
#include "sfm/ba_sparse_matrix.h"


TEST(BundleAdjustmentVectorMatrixTest, MatrixSetFromTripletsTest)
{
    typedef sfm::ba::SparseMatrix<double> SparseMatrix;

    SparseMatrix m(3, 4);
    EXPECT_EQ(3, m.num_rows());
    EXPECT_EQ(4, m.num_cols());
    EXPECT_EQ(0, m.num_non_zero());

    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(2, 0, 1.0);
        triplets.emplace_back(1, 1, 2.0);
        triplets.emplace_back(1, 2, 3.0);
        m.set_from_triplets(triplets);
    }
    EXPECT_EQ(3, m.num_non_zero());
    EXPECT_EQ(1.0, m.begin()[0]);
    EXPECT_EQ(2.0, m.begin()[1]);
    EXPECT_EQ(3.0, m.begin()[2]);

    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(1, 2, 3.0);
        triplets.emplace_back(1, 1, 2.0);
        triplets.emplace_back(2, 0, 1.0);
        m.set_from_triplets(triplets);
    }
    EXPECT_EQ(3, m.num_non_zero());
    EXPECT_EQ(1.0, m.begin()[0]);
    EXPECT_EQ(2.0, m.begin()[1]);
    EXPECT_EQ(3.0, m.begin()[2]);

    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(1, 2, 3.0);
        triplets.emplace_back(2, 1, 2.0);
        triplets.emplace_back(1, 1, 1.0);
        m.set_from_triplets(triplets);
    }
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
        triplets.emplace_back(0, 2, 1.0);
        triplets.emplace_back(2, 1, 3.0);
        triplets.emplace_back(2, 2, 5.0);
        m1.set_from_triplets(triplets);
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
    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(0, 1, 1);
        triplets.emplace_back(0, 3, 4);
        triplets.emplace_back(1, 2, 3);
        m1.set_from_triplets(triplets);
    }

    SparseMatrix m2(2, 4);
    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(0, 0, 1);
        triplets.emplace_back(1, 1, 2);
        triplets.emplace_back(1, 2, 3);
        m2.set_from_triplets(triplets);
    }

    SparseMatrix m3(4, 2);
    EXPECT_NO_THROW(m1.subtract(m2));
    EXPECT_THROW(m1.subtract(m3), std::exception);

    m3 = m1.subtract(m2);
    EXPECT_EQ(2, m3.num_rows());
    EXPECT_EQ(4, m3.num_cols());
    EXPECT_EQ(5, m3.num_non_zero());

    EXPECT_EQ(-1, m3.begin()[0]);
    EXPECT_EQ(1, m3.begin()[1]);
    EXPECT_EQ(-2, m3.begin()[2]);
    EXPECT_EQ(0, m3.begin()[3]);
    EXPECT_EQ(4, m3.begin()[4]);
}

TEST(BundleAdjustmentVectorMatrixTest, MatrixMatrixMultiplyTest)
{
    typedef sfm::ba::SparseMatrix<int> SparseMatrix;
    SparseMatrix m1(2, 4);
    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(0, 3, 1);
        triplets.emplace_back(1, 1, 1);
        triplets.emplace_back(1, 2, 3);
        m1.set_from_triplets(triplets);
    }

    SparseMatrix m2(4, 3);
    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(1, 1, 5);
        triplets.emplace_back(1, 2, 2);
        triplets.emplace_back(2, 0, 4);
        triplets.emplace_back(2, 2, 2);
        triplets.emplace_back(3, 2, 2);
        m2.set_from_triplets(triplets);
    }

    SparseMatrix m3 = m1.multiply(m2);
    EXPECT_EQ(2, m3.num_rows());
    EXPECT_EQ(3, m3.num_cols());
    EXPECT_EQ(4, m3.num_non_zero());

    EXPECT_EQ(12, m3.begin()[0]);
    EXPECT_EQ(5, m3.begin()[1]);
    EXPECT_EQ(2, m3.begin()[2]);
    EXPECT_EQ(8, m3.begin()[3]);

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
        triplets.emplace_back(0, 1, 1);
        triplets.emplace_back(0, 2, 4);
        triplets.emplace_back(0, 3, 1);
        triplets.emplace_back(1, 2, 3);
        m1.set_from_triplets(triplets);
    }

    DenseVector v1(4, 0);
    v1[1] = 1;
    v1[2] = 2;

    DenseVector ret = m1.multiply(v1);
    EXPECT_EQ(3, ret.size());
    EXPECT_EQ(9, ret[0]);
    EXPECT_EQ(6, ret[1]);
    EXPECT_EQ(0, ret[2]);
}

TEST(BundleAdjustmentVectorMatrixTest, MatrixMultiplyDiagonalTest)
{
    typedef sfm::ba::SparseMatrix<int> SparseMatrix;
    SparseMatrix m1(3, 4);
    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(0, 2, 4);
        triplets.emplace_back(1, 1, 2);
        m1.set_from_triplets(triplets);
    }

    EXPECT_EQ(2, m1.begin()[0]);
    EXPECT_EQ(4, m1.begin()[1]);
    m1.mult_diagonal(3);
    EXPECT_EQ(6, m1.begin()[0]);
    EXPECT_EQ(4, m1.begin()[1]);

    SparseMatrix m2(4, 2);
    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(0, 0, 3);
        triplets.emplace_back(1, 0, 4);
        triplets.emplace_back(2, 0, 5);
        m2.set_from_triplets(triplets);
    }

    EXPECT_EQ(3, m2.begin()[0]);
    EXPECT_EQ(4, m2.begin()[1]);
    EXPECT_EQ(5, m2.begin()[2]);
    m2.mult_diagonal(3);
    EXPECT_EQ(9, m2.begin()[0]);
    EXPECT_EQ(4, m2.begin()[1]);
    EXPECT_EQ(5, m2.begin()[2]);
}

TEST(BundleAdjustmentVectorMatrixTest, MatrixGetDiagonal1Test)
{
    typedef sfm::ba::SparseMatrix<int> SparseMatrix;
    SparseMatrix m1(4, 3);
    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(0, 0, 1);
        triplets.emplace_back(1, 2, 2);
        triplets.emplace_back(2, 2, 3);
        triplets.emplace_back(3, 0, 4);
        m1.set_from_triplets(triplets);
    }

    SparseMatrix d1 = m1.diagonal_matrix();
    EXPECT_EQ(3, d1.num_rows());
    EXPECT_EQ(3, d1.num_cols());
    EXPECT_EQ(2, d1.num_non_zero());
    EXPECT_EQ(1, d1.begin()[0]);
    EXPECT_EQ(3, d1.begin()[1]);
}

TEST(BundleAdjustmentVectorMatrixTest, MatrixGetDiagonal2Test)
{
    typedef sfm::ba::SparseMatrix<int> SparseMatrix;
    SparseMatrix m2(3, 4);
    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(1, 1, 1);
        triplets.emplace_back(2, 3, 2);
        m2.set_from_triplets(triplets);
    }

    SparseMatrix d2 = m2.diagonal_matrix();
    EXPECT_EQ(3, d2.num_rows());
    EXPECT_EQ(3, d2.num_cols());
    EXPECT_EQ(1, d2.num_non_zero());
    EXPECT_EQ(1, d2.begin()[0]);
}

TEST(BundleAdjustmentVectorMatrixTest, MatrixCWiseInvertTest)
{
    typedef sfm::ba::SparseMatrix<double> SparseMatrix;
    SparseMatrix m1(3, 4);
    {
        SparseMatrix::Triplets triplets;
        triplets.emplace_back(1, 1, 1.0);
        triplets.emplace_back(2, 3, 2.0);
        m1.set_from_triplets(triplets);
    }
    m1.cwise_invert();

    EXPECT_EQ(2, m1.num_non_zero());
    EXPECT_NEAR(1.0, m1.begin()[0], 1e-30);
    EXPECT_NEAR(0.5, m1.begin()[1], 1e-30);
}
