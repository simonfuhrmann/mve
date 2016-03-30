/*
 * Test cases for the matrix class and tools.
 * Written by Simon Fuhrmann.
 */

#include <gtest/gtest.h>

#include "math/matrix.h"
#include "math/matrix_tools.h"

TEST(MatrixTest, RowMajor)
{
    /* Test row-major functionality. */
    math::Matrix2f mat;
    mat(0, 0) = 1.0f; mat(0, 1) = 2.0f;
    mat(1, 0) = 3.0f; mat(1, 1) = 4.0f;
    EXPECT_EQ(mat[0], 1.0f);
    EXPECT_EQ(mat[1], 2.0f);
    EXPECT_EQ(mat[2], 3.0f);
    EXPECT_EQ(mat[3], 4.0f);
}

TEST(MatrixTest, MatrixMultiplication)
{
    using namespace math;
    Matrix<float,3,2> M1;
    M1(0,0) = 1.0f; M1(0,1) = 2.0f;
    M1(1,0) = 3.0f; M1(1,1) = 4.0f;
    M1(2,0) = 5.0f; M1(2,1) = 6.0f;

    Matrix<float,2,3> M2;
    M2(0,0) = 5.0f; M2(0,1) = 6.0f; M2(0,2) = 1.0f;
    M2(1,0) = 1.0f; M2(1,1) = 2.0f; M2(1,2) = 3.0f;

    /* Matrix matrix multiplication. */
    Matrix3f R1 = M1.mult(M2);
    EXPECT_EQ(R1(0,0), 7.0f);
    EXPECT_EQ(R1(0,1), 10.0f);
    EXPECT_EQ(R1(0,2), 7.0f);
    EXPECT_EQ(R1(1,0), 19.0f);
    EXPECT_EQ(R1(1,1), 26.0f);
    EXPECT_EQ(R1(1,2), 15.0f);
    EXPECT_EQ(R1(2,0), 31.0f);
    EXPECT_EQ(R1(2,1), 42.0f);
    EXPECT_EQ(R1(2,2), 23.0f);

    /* Matrix matrix multiplication. */
    Matrix2f R2 = M2.mult(M1);
    EXPECT_EQ(R2(0,0), 28.0f);
    EXPECT_EQ(R2(0,1), 40.0f);
    EXPECT_EQ(R2(1,0), 22.0f);
    EXPECT_EQ(R2(1,1), 28.0f);

    /* Matrix vector multiplication. */
    EXPECT_EQ(M1.mult(Vec2f(1.0f, 2.0f)), Vec3f(5.0f, 11.0f, 17.0f));
}

TEST(MatrixTest, MatrixSubtraction)
{
    using namespace math;
    Matrix3f test(999.0f);
    test(0,0) = 1.0f; test(0,1) = 2.0f; test(0,2) = 3.0f;
    test(1,0) = 4.0f; test(1,1) = 5.0f; test(1,2) = 6.0f;
    test(2,0) = 7.0f; test(2,1) = 8.0f; test(2,2) = 9.0f;

    /* Matrix matrix subtraction. */
    Matrix3f ones(1.0f);
    EXPECT_EQ((test - ones)(0,0), 0.0f);
    EXPECT_EQ((test - ones)(0,1), 1.0f);
    EXPECT_EQ((test - ones)(0,2), 2.0f);
}

TEST(MatrixTest, MatrixOperations)
{
    using namespace math;
    Matrix3f ones(1.0f);
    Matrix3f test(999.0f);
    test(0,0) = 1.0f; test(0,1) = 2.0f; test(0,2) = -3.0f;
    test(1,0) = 4.0f; test(1,1) = 5.0f; test(1,2) = 6.0f;
    test(2,0) = 7.0f; test(2,1) = 8.0f; test(2,2) = 9.0f;

    /* Matrix access, min, max, square check, . */
    EXPECT_EQ((Matrix<int,3,4>::rows), 3);
    EXPECT_EQ((Matrix<int,3,4>::cols), 4);
    EXPECT_EQ(test.col(1), Vec3f(2.0f, 5.0f, 8.0f));
    EXPECT_EQ(test.row(1), Vec3f(4.0f, 5.0f, 6.0f));
    EXPECT_EQ(Matrix3f(1.0f).minimum(), 1.0f);
    EXPECT_EQ(Matrix3f(1.0f).maximum(), 1.0f);
    EXPECT_EQ(test.maximum(), 9.0f);
    EXPECT_EQ(test.minimum(), -3.0f);
    EXPECT_TRUE((Matrix<float,3,3>()).is_square());
    EXPECT_FALSE((Matrix<float,3,4>()).is_square());
    EXPECT_EQ(test(1,2), 6.0f);
    EXPECT_EQ(test.transposed()(1,2), 8.0f);

    EXPECT_FALSE(test.is_similar(ones, 0.0f));
    EXPECT_FALSE(test.is_similar(ones, 5.0f));
    EXPECT_TRUE(test.is_similar(ones, 8.0f));
}

TEST(MatrixTest, MatrixNegate)
{
    math::Matrix<float,2,3> M1;
    M1(0,0) = 5.0f; M1(0,1) = 6.0f; M1(0,2) = 1.0f;
    M1(1,0) = 1.0f; M1(1,1) = 2.0f; M1(1,2) = 3.0f;

    math::Matrix<float,2,3> M2;
    M2(0,0) = -5.0f; M2(0,1) = -6.0f; M2(0,2) = -1.0f;
    M2(1,0) = -1.0f; M2(1,1) = -2.0f; M2(1,2) = -3.0f;

    for (int i = 0; i < 6; ++i)
        EXPECT_EQ(M2[i], M1.negated()[i]);

    M1.negate();
    for (int i = 0; i < 6; ++i)
        EXPECT_EQ(M2[i], M1[i]);
}

TEST(MatrixTest, MatrixStackMatrix)
{
    math::Matrix<float, 1, 1> m(1.0f);
    math::Matrix<float, 1, 2> m1 = m.hstack(math::Matrix<float, 1, 1>(2.0f));
    ASSERT_EQ(m1(0,0), 1.0f);
    ASSERT_EQ(m1(0,1), 2.0f);

    math::Matrix<float, 2, 1> m2 = m.vstack(math::Matrix<float, 1, 1>(3.0f));
    ASSERT_EQ(m2(0,0), 1.0f);
    ASSERT_EQ(m2(1,0), 3.0f);

    math::Matrix<float, 2, 1> m3(4.0f);
    math::Matrix<float, 2, 2> m4 = m2.hstack(m3);
    ASSERT_EQ(m4(0,0), 1.0f);  ASSERT_EQ(m4(0,1), 4.0f);
    ASSERT_EQ(m4(1,0), 3.0f);  ASSERT_EQ(m4(1,1), 4.0f);

    math::Matrix<float, 1, 2> m5; m5(0,0) = 5.0f; m5(0,1) = 6.0f;
    math::Matrix<float, 3, 2> m6 = m4.vstack(m5);
    ASSERT_EQ(m6(0,0), 1.0f);  ASSERT_EQ(m6(0,1), 4.0f);
    ASSERT_EQ(m6(1,0), 3.0f);  ASSERT_EQ(m6(1,1), 4.0f);
    ASSERT_EQ(m6(2,0), 5.0f);  ASSERT_EQ(m6(2,1), 6.0f);
}

TEST(MatrixTest, MatrixStackVector)
{
    math::Matrix<float, 1, 1> m(1.0f);
    math::Matrix<float, 1, 2> m1 = m.hstack(math::Vec1f(2.0f));
    ASSERT_EQ(m1(0,0), 1.0f);
    ASSERT_EQ(m1(0,1), 2.0f);

    math::Matrix<float, 2, 1> m2 = m.vstack(math::Vec1f(3.0f));
    ASSERT_EQ(m2(0,0), 1.0f);
    ASSERT_EQ(m2(1,0), 3.0f);

    math::Matrix<float, 2, 2> m3 = m2.hstack(math::Vec2f(4.0f, 5.0f));
    ASSERT_EQ(m3(0,0), 1.0f);  ASSERT_EQ(m3(0,1), 4.0f);
    ASSERT_EQ(m3(1,0), 3.0f);  ASSERT_EQ(m3(1,1), 5.0f);

    math::Matrix<float, 3, 2> m4 = m3.vstack(math::Vec2f(7.0f, 6.0f));
    ASSERT_EQ(m4(0,0), 1.0f);  ASSERT_EQ(m4(0,1), 4.0f);
    ASSERT_EQ(m4(1,0), 3.0f);  ASSERT_EQ(m4(1,1), 5.0f);
    ASSERT_EQ(m4(2,0), 7.0f);  ASSERT_EQ(m4(2,1), 6.0f);
}

TEST(MatrixTest, MatrixBeginEnd)
{
    math::Matrix<int, 2, 3> m;
    for (int i = 0; i < 6; ++i)
        m[i] = i;

    int vec[7];
    std::fill(vec, vec + 7, -1);
    std::copy(m.begin(), m.end(), vec);

    for (int i = 0; i < 6; ++i)
        EXPECT_EQ(vec[i], i);
    EXPECT_EQ(vec[6], -1);
}

TEST(MatrixTest, MatrixDeleteRow)
{
    math::Matrix<int, 2, 3> m;
    for (int i = 0; i < 6; ++i)
        m[i] = i;

    math::Matrix<int, 1, 3> m2 = m.delete_row(0);
    EXPECT_EQ(3, m2[0]);
    EXPECT_EQ(4, m2[1]);
    EXPECT_EQ(5, m2[2]);

    m2 = m.delete_row(1);
    EXPECT_EQ(0, m2[0]);
    EXPECT_EQ(1, m2[1]);
    EXPECT_EQ(2, m2[2]);
}

TEST(MatrixTest, MatrixDeleteCol)
{
    math::Matrix<int, 2, 3> m;
    for (int i = 0; i < 6; ++i)
        m[i] = i;

    math::Matrix<int, 2, 2> m2 = m.delete_col(0);
    EXPECT_EQ(1, m2[0]);
    EXPECT_EQ(2, m2[1]);
    EXPECT_EQ(4, m2[2]);
    EXPECT_EQ(5, m2[3]);

    m2 = m.delete_col(1);
    EXPECT_EQ(0, m2[0]);
    EXPECT_EQ(2, m2[1]);
    EXPECT_EQ(3, m2[2]);
    EXPECT_EQ(5, m2[3]);

    m2 = m.delete_col(2);
    EXPECT_EQ(0, m2[0]);
    EXPECT_EQ(1, m2[1]);
    EXPECT_EQ(3, m2[2]);
    EXPECT_EQ(4, m2[3]);
}
