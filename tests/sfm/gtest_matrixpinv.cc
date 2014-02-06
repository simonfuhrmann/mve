/*
 * Test cases for matrix pseudo-inverse (Moore-Penrose inverse).
 * Written by Simon Fuhrmann.
 */

#include <gtest/gtest.h>

#include "sfm/matrixsvd.h"

TEST(MatrixPseudoInverse, GoldenData1)
{
    double a_values[] = { 2, -4,  5, 6, 0, 3, 2, -4, 5, 6, 0, 3 };
    double a_inv_values[] = { -2, 6, -2, 6, -5, 3, -5, 3, 4, 0, 4, 0 };
    math::Matrix<double, 4, 3> A(a_values);
    math::Matrix<double, 3, 4> Ainv(a_inv_values);
    Ainv /= 72.0;

    math::Matrix<double, 3, 4> result;
    math::matrix_pseudo_inverse(A, &result);

    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 4; ++c)
            EXPECT_NEAR(Ainv(r, c), result(r, c), 1e-16);
}

TEST(MatrixPseudoInverse, GoldenData2)
{
    double a_values[] = { 1, 1, 1, 1, 5, 7, 7, 9 };
    double a_inv_values[] = { 2, -0.25, 0.25, 0, 0.25, 0, -1.5, 0.25 };
    math::Matrix<double, 2, 4> A(a_values);
    math::Matrix<double, 4, 2> Ainv(a_inv_values);

    math::Matrix<double, 4, 2> result;
    math::matrix_pseudo_inverse(A, &result);

    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 2; ++c)
            EXPECT_NEAR(Ainv(r, c), result(r, c), 1e-14);
}
