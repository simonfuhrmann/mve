/*
 * Test cases for matrix singular value decomposition (SVD).
 * Written by Simon Fuhrmann.
 */

#include <gtest/gtest.h>

#include "sfm/matrixsvd.h"

TEST(MatrixSVDTest, BeforeAfter1)
{
    math::Matrix<double, 2, 2> A;
    A(0,0) = 1.0f; A(0,1) = 2.0f;
    A(1,0) = 3.0f; A(1,1) = 4.0f;

    math::Matrix<double, 2, 2> U;
    math::Matrix<double, 2, 2> S;
    math::Matrix<double, 2, 2> V;
    math::matrix_svd(A, &U, &S, &V);

    EXPECT_TRUE(A.is_similar(U * S * V.transposed(), 1e-10));
    EXPECT_GT(S(0,0), S(1,1));
}

TEST(MatrixSVDTest, BeforeAfter2)
{
    math::Matrix<double, 3, 2> A;
    A(0,0) = 1.0f; A(0,1) = 2.0f;
    A(1,0) = 2.0f; A(1,1) = 3.0f;
    A(2,0) = 4.0f; A(2,1) = 5.0f;

    math::Matrix<double, 3, 3> U;
    math::Matrix<double, 3, 2> S;
    math::Matrix<double, 2, 2> V;
    math::matrix_svd(A, &U, &S, &V);

    EXPECT_TRUE(A.is_similar(U * S * V.transposed(), 1e-10));
    EXPECT_GT(S(0,0), S(1,1));
}

TEST(MatrixSVDTest, BeforeAfter3)
{
    math::Matrix<double, 2, 2> A;
    A(0,0) = 1.0f; A(0,1) = 2.0f;
    A(1,0) = 3.0f; A(1,1) = 4.0f;

    math::Matrix<double, 2, 2> U;
    math::Matrix<double, 2, 2> S(0.0);
    math::Matrix<double, 2, 2> V;
    math::matrix_svd<double>(*A, 2, 2, *U, *S, *V);
    std::swap(S(0,1), S(1,1));

    EXPECT_TRUE(A.is_similar(U * S * V.transposed(), 1e-10));
    EXPECT_GT(S(0,0), S(1,1));
}

TEST(MatrixSVDTest, BeforeAfter4)
{
    math::Matrix<double, 3, 2> A;
    A(0,0) = 1.0f; A(0,1) = 2.0f;
    A(1,0) = 2.0f; A(1,1) = 3.0f;
    A(2,0) = 4.0f; A(2,1) = 5.0f;

    math::Matrix<double, 3, 3> U;
    math::Matrix<double, 3, 2> S(0.0);
    math::Matrix<double, 2, 2> V;
    math::matrix_svd<double>(*A, 3, 2, *U, *S, *V);
    std::swap(S(0,1), S(1,1));

    EXPECT_TRUE(A.is_similar(U * S * V.transposed(), 1e-10));
    EXPECT_GT(S(0,0), S(1,1));
}

namespace {

    double getrand (void)
    {
        return std::rand() / static_cast<double>(RAND_MAX);
    }

} // namespace

// Note: Random is a bad thing in test cases! Don't do it!
TEST(MatrixSVDTest, ForbiddenRandomTest)
{
    std::srand(0);
    math::Matrix<double, 3, 2> A;
    for (int i = 0; i < 10; ++i)
    {
        A(0,0) = getrand(); A(0,1) = getrand();
        A(1,0) = getrand(); A(1,1) = getrand();
        A(2,0) = getrand(); A(2,1) = getrand();

        math::Matrix<double, 3, 3> U;
        math::Matrix<double, 3, 2> S;
        math::Matrix<double, 2, 2> V;
        math::matrix_svd(A, &U, &S, &V);

        EXPECT_TRUE(A.is_similar(U * S * V.transposed(), 1e-10));
        EXPECT_GT(S(0,0), S(1,1));
    }
}
