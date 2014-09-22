/*
 * Test cases for matrix singular value decomposition (SVD).
 * Written by Simon Fuhrmann and Daniel Thuerck.
 */

#include <iostream>
#include <gtest/gtest.h>

#include "math/matrix_tools.h"
#include "math/matrix_qr.h"

TEST(MatrixQRTest, MatrixApplyGivensRotation)
{
    // create simple enumeration matrices
    double mat[5 * 4];
    double mat2[5 * 4];
    for (int i = 0; i < 20; ++i)
    {
        mat[i] = static_cast<double>(i + 1);
        mat2[i] = static_cast<double>(i + 1);
    }

    // ground truth for column rotation
    double groundtruth_mat_col[5 * 4];
    for (int i = 0; i < 20; ++i)
        groundtruth_mat_col[i] = static_cast<double>(i + 1);

    groundtruth_mat_col[0 * 4 + 1] = -3.577708763999663;
    groundtruth_mat_col[1 * 4 + 1] = -8.944271909999159;
    groundtruth_mat_col[2 * 4 + 1] = -14.310835055998654;
    groundtruth_mat_col[3 * 4 + 1] = -19.677398201998148;
    groundtruth_mat_col[4 * 4 + 1] = -25.043961347997644;

    groundtruth_mat_col[0 * 4 + 2] = 0.447213595499958;
    groundtruth_mat_col[1 * 4 + 2] = 2.236067977499790;
    groundtruth_mat_col[2 * 4 + 2] = 4.024922359499622;
    groundtruth_mat_col[3 * 4 + 2] = 5.813776741499454;
    groundtruth_mat_col[4 * 4 + 2] = 7.602631123499284;

    // ground truth for row rotation
    double groundtruth_mat_row[5 * 4];
    for (int i = 0; i < 20; ++i)
        groundtruth_mat_row[i] = static_cast<double>(i + 1);

    groundtruth_mat_row[1 * 4 + 0] = -10.285912696499032;
    groundtruth_mat_row[1 * 4 + 1] = -11.627553482998907;
    groundtruth_mat_row[1 * 4 + 2] = -12.969194269498779;
    groundtruth_mat_row[1 * 4 + 3] = -14.310835055998654;

    groundtruth_mat_row[2 * 4 + 0] = 0.447213595499958;
    groundtruth_mat_row[2 * 4 + 1] = 0.894427190999916;
    groundtruth_mat_row[2 * 4 + 2] = 1.341640786499874;
    groundtruth_mat_row[2 * 4 + 3] = 1.788854381999831;

    double givens_c, givens_s;
    math::internal::matrix_givens_rotation(1.0, 2.0, &givens_c, &givens_s, 1e-14);
    math::internal::matrix_apply_givens_column(mat, 5, 4, 1, 2, givens_c, givens_s);
    math::internal::matrix_apply_givens_row(mat2, 5, 4, 1, 2, givens_c, givens_s);

    // compare results
    for (int i = 0; i < 5 * 4; ++i)
    {
        EXPECT_NEAR(groundtruth_mat_col[i], mat[i], 1e-14);
        EXPECT_NEAR(groundtruth_mat_row[i], mat2[i], 1e-14);
    }
}

TEST(MatrixQRTest, MatrixQRQuadraticTest)
{
    // create simple test matrices
    double mat[3 * 3];
    for (int i = 0; i < 9; ++i)
    {
        mat[i] = static_cast<double>(i + 1);
    }

    double groundtruth_mat_q[3 * 3];
    double groundtruth_mat_r[3 * 3];

    groundtruth_mat_q[0 * 3 + 0] = 0.123091490979333;
    groundtruth_mat_q[0 * 3 + 1] = 0.904534033733291;
    groundtruth_mat_q[0 * 3 + 2] = -0.408248290463863;
    groundtruth_mat_q[1 * 3 + 0] = 0.492365963917331;
    groundtruth_mat_q[1 * 3 + 1] = 0.301511344577764;
    groundtruth_mat_q[1 * 3 + 2] = 0.816496580927726;
    groundtruth_mat_q[2 * 3 + 0] = 0.861640436855329;
    groundtruth_mat_q[2 * 3 + 1] = -0.301511344577764;
    groundtruth_mat_q[2 * 3 + 2] = -0.408248290463863;

    groundtruth_mat_r[0 * 3 + 0] = 8.124038404635961;
    groundtruth_mat_r[0 * 3 + 1] = 9.601136296387953;
    groundtruth_mat_r[0 * 3 + 2] = 11.078234188139948;
    groundtruth_mat_r[1 * 3 + 0] = 0;
    groundtruth_mat_r[1 * 3 + 1] = 0.904534033733291;
    groundtruth_mat_r[1 * 3 + 2] = 1.809068067466582;
    groundtruth_mat_r[2 * 3 + 0] = 0;
    groundtruth_mat_r[2 * 3 + 1] = 0;
    groundtruth_mat_r[2 * 3 + 2] = -0.000000000000001;

    double mat_q[3 * 3];
    double mat_r[3 * 3];

    math::matrix_qr(mat, 3, 3, mat_q, mat_r, 1e-14);

    // compare results up to EPSILON
    for (int i = 0; i < 9; ++i)
    {
        EXPECT_NEAR(groundtruth_mat_q[i], mat_q[i], 1e-14);
        EXPECT_NEAR(groundtruth_mat_r[i], mat_r[i], 1e-14);
    }
}

TEST(MatrixQRTest, MatrixQRRectangularTest)
{
    // create test matrix
    double mat[5 * 4];
    for (int i = 0; i < 20; ++i)
    {
        mat[i] = static_cast<double>(i + 1);
    }

    double mat_q[5 * 5];
    double mat_r[5 * 4];
    math::matrix_qr(mat, 5, 4, mat_q, mat_r, 1e-14);

    double groundtruth_mat_q[5 * 5];
    groundtruth_mat_q[0] = 0.042070316191167;
    groundtruth_mat_q[1] = 0.773453352501349;
    groundtruth_mat_q[2] = -0.632455532033676;
    groundtruth_mat_q[3] = 0;
    groundtruth_mat_q[4] = 0;
    groundtruth_mat_q[5] = 0.210351580955836;
    groundtruth_mat_q[6] = 0.505719499712420;
    groundtruth_mat_q[7] = 0.632455532033676;
    groundtruth_mat_q[8] = -0.547722557505167;
    groundtruth_mat_q[9] = 0;
    groundtruth_mat_q[10] = 0.378632845720504;
    groundtruth_mat_q[11] = 0.237985646923491;
    groundtruth_mat_q[12] = 0.316227766016837;
    groundtruth_mat_q[13] = 0.730296743340219;
    groundtruth_mat_q[14] = -0.408248290463868;
    groundtruth_mat_q[15] = 0.546914110485173;
    groundtruth_mat_q[16] = -0.029748205865435;
    groundtruth_mat_q[17] = 0.000000000000002;
    groundtruth_mat_q[18] = 0.182574185835059;
    groundtruth_mat_q[19] = 0.816496580927725;
    groundtruth_mat_q[20] = 0.715195375249841;
    groundtruth_mat_q[21] = -0.297482058654366;
    groundtruth_mat_q[22] = -0.316227766016839;
    groundtruth_mat_q[23] = -0.365148371670112;
    groundtruth_mat_q[24] = -0.408248290463860;

    double groundtruth_mat_r[5 * 4];
    groundtruth_mat_r[0] = 23.769728648009430;
    groundtruth_mat_r[1] = 25.662892876611952;
    groundtruth_mat_r[2] = 27.556057105214467;
    groundtruth_mat_r[3] = 29.449221333816997;
    groundtruth_mat_r[4] = 0;
    groundtruth_mat_r[5] = 1.189928234617459;
    groundtruth_mat_r[6] = 2.379856469234919;
    groundtruth_mat_r[7] = 3.569784703852378;
    groundtruth_mat_r[8] = 0;
    groundtruth_mat_r[9] = 0;
    groundtruth_mat_r[10] = 0;
    groundtruth_mat_r[11] = 0;
    groundtruth_mat_r[12] = 0;
    groundtruth_mat_r[13] = 0;
    groundtruth_mat_r[14] = 0;
    groundtruth_mat_r[15] = 0;
    groundtruth_mat_r[16] = 0;
    groundtruth_mat_r[17] = 0;
    groundtruth_mat_r[18] = 0;
    groundtruth_mat_r[19] = 0;

    for (int i = 0; i < 25; ++i)
    {
        EXPECT_NEAR(groundtruth_mat_q[i], mat_q[i], 1e-14);
    }
    for (int i = 0; i < 20; ++i)
    {
        EXPECT_NEAR(groundtruth_mat_r[i], mat_r[i], 1e-14);
    }
}

TEST(MatrixQRTest, MatrixQRScalarTest)
{
    double mat[1];
    mat[0] = 1;

    double mat_q[1];
    double mat_r[1];

    math::matrix_qr(mat, 1, 1, mat_q, mat_r, 1e-14);

    EXPECT_NEAR(1, mat_q[0], 1e-14);
    EXPECT_NEAR(1, mat_r[0], 1e-14);
}

TEST(MatrixQRTest, MatrixQRVectorTest)
{
    double mat[2];
    mat[0] = 1;
    mat[1] = 2;

    double groundtruth_mat_q[4];
    groundtruth_mat_q[0] = -0.447213595499958;
    groundtruth_mat_q[1] = 0.894427190999916;
    groundtruth_mat_q[2] = -0.894427190999916;
    groundtruth_mat_q[3] = -0.447213595499958;

    double groundtruth_mat_r[2];
    groundtruth_mat_r[0] = -2.236067977499790;
    groundtruth_mat_r[1] = 0;

    double mat_q[4];
    double mat_r[2];

    math::matrix_qr(mat, 2, 1, mat_q, mat_r, 1e-14);

    EXPECT_NEAR(groundtruth_mat_q[0], mat_q[0], 1e-14);
    EXPECT_NEAR(groundtruth_mat_q[1], mat_q[1], 1e-14);
    EXPECT_NEAR(groundtruth_mat_q[2], mat_q[2], 1e-14);
    EXPECT_NEAR(groundtruth_mat_q[3], mat_q[3], 1e-14);

    EXPECT_NEAR(groundtruth_mat_r[0], mat_r[0], 1e-14);
    EXPECT_NEAR(groundtruth_mat_r[1], mat_r[1], 1e-14);
}

TEST(MatrixQRTest, TestMatrixInterface)
{
    double A_values[] = { 1, 2, 3,  4, 5, 6,  7, 8, 9,  10, 11, 12 };
    math::Matrix<double, 4, 3> A(A_values);
    math::Matrix<double, 4, 4> Q;
    math::Matrix<double, 4, 3> R;
    math::matrix_qr(A, &Q, &R, 1e-16);

    /* Check if Q is orthogonal. */
    math::Matrix<double, 4, 4> QQ1 = Q * Q.transposed();
    EXPECT_TRUE(math::matrix_is_identity(QQ1, 1e-14));
    math::Matrix<double, 4, 4> QQ2 = Q.transposed() * Q;
    EXPECT_TRUE(math::matrix_is_identity(QQ2, 1e-14));

    /* Check if R is upper diagonal. */
    for (int y = 1; y < 4; ++y)
        for (int x = 0; x < y; ++x)
            EXPECT_NEAR(0.0, R(y, x), 1e-14);

    /* Check if A can be reproduced. */
    math::Matrix<double, 4, 3> newA = Q * R;
    for (int i = 0; i < 12; ++i)
        EXPECT_NEAR(newA[i], A[i], 1e-14);
}

TEST(MatrixQRTest, BeforeAfter1)
{
    math::Matrix<double, 2, 2> A;
    A(0,0) = 1.0f; A(0,1) = 2.0f;
    A(1,0) = -3.0f; A(1,1) = 4.0f;

    math::Matrix<double, 2, 2> Q, R;
    math::matrix_qr(A, &Q, &R);
    EXPECT_TRUE(A.is_similar(Q * R, 1e-14));
    EXPECT_NEAR(0.0, R(1,0), 1e-14);
    EXPECT_NEAR(0.0, Q.col(0).dot(Q.col(1)), 1e-14);
}

TEST(MatrixQRTest, BeforeAfter2)
{
    math::Matrix<double, 3, 3> A;
    A(0,0) = 1.0f; A(0,1) = 2.0f; A(0,2) = 8.0f;
    A(1,0) = 2.0f; A(1,1) = -3.0f; A(1,2) = 18.0f;
    A(2,0) = -4.0f; A(2,1) = 5.0f; A(2,2) = -2.0f;

    math::Matrix<double, 3, 3> Q, R;
    math::matrix_qr(A, &Q, &R);
    EXPECT_TRUE(A.is_similar(Q * R, 1e-12));
    EXPECT_NEAR(0.0, R(1,0), 1e-12);
    EXPECT_NEAR(0.0, R(2,0), 1e-12);
    EXPECT_NEAR(0.0, R(2,1), 1e-12);
    EXPECT_NEAR(0.0, Q.col(0).dot(Q.col(1)), 1e-12);
    EXPECT_NEAR(0.0, Q.col(1).dot(Q.col(2)), 1e-12);
    EXPECT_NEAR(0.0, Q.col(0).dot(Q.col(2)), 1e-12);
}
