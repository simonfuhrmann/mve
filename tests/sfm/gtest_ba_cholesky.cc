// Test cases for the Cholesky decomposition.
// Written by Simon Fuhrmann

#include <gtest/gtest.h>

#include "sfm/ba_cholesky.h"

TEST(CholeskyDecompTest, CholeskyGoldenData1Test)
{
    // From wikipedia.
    double const matrix[9] = { 4, 12, -16,  12, 37, -43,  -16, -43, 98 };
    double const oracle[9] = { 2, 0, 0,  6, 1, 0,  -8, 5, 3 };
    double result[9];

    sfm::ba::cholesky_decomposition(matrix, 3, result);
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR(oracle[i], result[i], 1e-18);
}

TEST(CholeskyDecompTest, CholeskyGoldenData2Test)
{
    // From wikipedia.
    double const matrix[9] = { 25, 15, -5,  15, 18, 0,  -5, 0, 11 };
    double const oracle[9] = { 5, 0, 0,  3, 3, 0,  -1, 1, 3 };
    double result[9];

    sfm::ba::cholesky_decomposition(matrix, 3, result);
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR(oracle[i], result[i], 1e-18);
}

TEST(CholeskyDecompTest, CholeskyInplaceComputationTest)
{
    // From wikipedia.
    double matrix[9] = { 25, 15, -5,  15, 18, 0,  -5, 0, 11 };
    double const oracle[9] = { 5, 0, 0,  3, 3, 0,  -1, 1, 3 };

    sfm::ba::cholesky_decomposition(matrix, 3, matrix);
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR(oracle[i], matrix[i], 1e-18);
}

TEST(CholeskyDecompTest, InvertLDGoldenData1Test)
{
    // From wikipedia.
    double matrix[9] = { 3, 0, 0,  9, 27, 0,  3, 3, 3 };
    double const oracle[9] =
    {
        1.0/3,  0,       0,
        -1.0/9, 1.0/27,  0,
        -2.0/9, -1.0/27, 1.0/3
    };

    double result[9];
    sfm::ba::invert_lower_diagonal(matrix, 3, result);
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR(oracle[i], result[i], 1e-16);
}

TEST(CholeskyDecompTest, InvertLDGoldenData2Test)
{
    double const matrix[9] = { 2, 0, 0,  6, 1, 0,  -8, 5, 3 };
    double const oracle[9] = { 1.0/2, 0, 0, -3, 1, 0, 19.0/3, -5.0/3, 1.0/3 };

    double result[9];
    sfm::ba::invert_lower_diagonal(matrix, 3, result);
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR(oracle[i], result[i], 1e-16);
}

TEST(CholeskyDecompTest, CholeskyInvertGoldenData1Test)
{
    double const matrix[9] = { 2, -1, 0,  -1, 2, -1,  0, -1, 2 };
    double const oracle[9] = { 3.0/4, 1.0/2, 1.0/4,  1.0/2, 1, 1.0/2,  1.0/4, 1.0/2, 3.0/4 };

    double result[9];
    sfm::ba::cholesky_invert(matrix, 3, result);
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR(oracle[i], result[i], 1e-15);
}
