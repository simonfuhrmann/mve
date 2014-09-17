/*
 * Test cases for matrix QR decomposition.
 * Written by Simon Fuhrmann.
 */

#include <gtest/gtest.h>
#include <iostream>

#include "sfm/matrixqrdecomp.h"

TEST(MatrixQRDecompTest, BeforeAfter1)
{
    math::Matrix<double, 2, 2> A;
    A(0,0) = 1.0f; A(0,1) = 2.0f;
    A(1,0) = -3.0f; A(1,1) = 4.0f;

    math::Matrix<double, 2, 2> Q, R;
    math::matrix_qr_decomp(A, &Q, &R);
    EXPECT_TRUE(A.is_similar(Q * R, 1e-15));
    EXPECT_EQ(0.0, R(1,0));
    EXPECT_NEAR(0.0, Q.col(0).dot(Q.col(1)), 1e-15);
}

TEST(MatrixQRDecompTest, BeforeAfter2)
{
    math::Matrix<double, 3, 3> A;
    A(0,0) = 1.0f; A(0,1) = 2.0f; A(0,2) = 8.0f;
    A(1,0) = 2.0f; A(1,1) = -3.0f; A(1,2) = 18.0f;
    A(2,0) = -4.0f; A(2,1) = 5.0f; A(2,2) = -2.0f;

    math::Matrix<double, 3, 3> Q, R;
    math::matrix_qr_decomp(A, &Q, &R);
    EXPECT_TRUE(A.is_similar(Q * R, 1e-12));
    EXPECT_EQ(0.0, R(1,0));
    EXPECT_EQ(0.0, R(2,0));
    EXPECT_EQ(0.0, R(2,1));
    EXPECT_NEAR(0.0, Q.col(0).dot(Q.col(1)), 1e-12);
    EXPECT_NEAR(0.0, Q.col(1).dot(Q.col(2)), 1e-12);
    EXPECT_NEAR(0.0, Q.col(0).dot(Q.col(2)), 1e-12);
}
