/*
 * Test cases for matrix singular value decomposition (SVD).
 * Written by Simon Fuhrmann and Daniel Thuerck.
 */

#include <limits>
#include <gtest/gtest.h>

#include "math/matrix_svd.h"

TEST(MatrixSVDTest, MatrixSimpleTest1)
{
    math::Matrix<double, 3, 2> A;
    A(0, 0) = 1.0;  A(0, 1) = 4.0;
    A(1, 0) = 2.0;  A(1, 1) = 5.0;
    A(2, 0) = 3.0;  A(2, 1) = 6.0;

    math::Matrix<double, 3, 2> U;
    math::Matrix<double, 2, 2> S;
    math::Matrix<double, 2, 2> V;
    math::matrix_svd(A, &U, &S, &V, 1e-10);

    math::Matrix<double, 3, 2> A_svd = U * S * V.transposed();
    for (int i = 0; i < 6; ++i)
        EXPECT_NEAR(A_svd[i], A[i], 1e-13);
}

TEST(MatrixSVDTest, MatrixSimpleTest2)
{
    math::Matrix<double, 2, 3> A;
    A(0, 0) = 1.0;  A(0, 1) = 2.0;  A(0, 2) = 3.0;
    A(1, 0) = 4.0;  A(1, 1) = 5.0;  A(1, 2) = 6.0;

    math::Matrix<double, 2, 3> U;
    math::Matrix<double, 3, 3> S;
    math::Matrix<double, 3, 3> V;
    math::matrix_svd(A, &U, &S, &V, 1e-10);

    math::Matrix<double, 2, 3> A_svd = U * S * V.transposed();
    for (int i = 0; i < 6; ++i)
        EXPECT_NEAR(A_svd[i], A[i], 1e-13);
}

TEST(MatrixSVDTest, MatrixIsDiagonalTest)
{
    float mat[3 * 10];
    std::fill(mat, mat + 3 * 10, 0.0f);
    EXPECT_TRUE(math::matrix_is_diagonal(mat, 3, 10, 0.0f));
    EXPECT_TRUE(math::matrix_is_diagonal(mat, 10, 3, 0.0f));

    // Interpret as 3x10 matrix (3 rows, 10 columns).
    mat[0 * 10 + 0] = 10.0f;
    mat[1 * 10 + 1] = 20.0f;
    mat[2 * 10 + 2] = 30.0f;
    EXPECT_TRUE(math::matrix_is_diagonal(mat, 3, 10, 0.0f));

    mat[2 * 10 + 3] = 40.0f;
    EXPECT_FALSE(math::matrix_is_diagonal(mat, 3, 10, 0.0f));
}

TEST(MatrixSVDTest, MatrixIsSubmatrixZeroEnclosed)
{
    float mat[4 * 4];
    std::fill(mat, mat + 4 * 4, 0.0f);

    // Everything is zero.
    EXPECT_TRUE(math::internal::matrix_is_submatrix_zero_enclosed(mat, 4, 2, 0.0f));

    // Doesn't check diagonally.
    mat[1 * 4 + 1] = 1.0f;
    EXPECT_TRUE(math::internal::matrix_is_submatrix_zero_enclosed(mat, 4, 2, 0.0f));

    // Damage the upper row.
    mat[1 * 4 + 2] = 2.0f;
    EXPECT_FALSE(math::internal::matrix_is_submatrix_zero_enclosed(mat, 4, 2, 0.0f));
    mat[1 * 4 + 2] = 0.0f;

    // Damage the left column.
    mat[2 * 4 + 1] = 3.0f;
    EXPECT_FALSE(math::internal::matrix_is_submatrix_zero_enclosed(mat, 4, 2, 0.0f));
    mat[2 * 4 + 1] = 0.0f;

    // Check with submatrix as large as full matrix.
    EXPECT_TRUE(math::internal::matrix_is_submatrix_zero_enclosed(mat, 4, 4, 0.0f));
}

TEST(MatrixSVDTest, MatrixIsSuperdiagonalNonzero)
{
    float mat[3 * 10];
    std::fill(mat, mat + 3 * 10, 1.0f);
    EXPECT_TRUE(math::internal::matrix_is_superdiagonal_nonzero(mat, 3, 10, 0.0f));
    EXPECT_TRUE(math::internal::matrix_is_superdiagonal_nonzero(mat, 10, 3, 0.0f));

    // Interpret as 3x10. Inject a zero.
    mat[1 * 10 + 2] = 0.0f;
    EXPECT_FALSE(math::internal::matrix_is_superdiagonal_nonzero(mat, 3, 10, 0.0f));
}

TEST(MatrixSVDTest, Matrix2x2Eigenvalues)
{
    math::Matrix2d mat;
    mat(0,0) = 0.318765239858981; mat(0,1) = -0.433592022305684;
    mat(1,0) = -1.307688296305273; mat(1,1) = 0.342624466538650;

    double smaller_ev, larger_ev;
    math::internal::matrix_2x2_eigenvalues(mat.begin(), &smaller_ev, &larger_ev);

    EXPECT_NEAR(-0.422395797795416, smaller_ev, 1e-14);
    EXPECT_NEAR(1.083785504193047, larger_ev, 1e-14);
}

TEST(MatrixSVDTest, MatrixHouseholderOnZeroTest)
{
    double vec_zero[2];
    vec_zero[0] = 0;
    vec_zero[1] = 0;

    double house_vector[2];
    double house_beta;

    math::internal::matrix_householder_vector(vec_zero, 2, house_vector,
        &house_beta, 1e-14, 1.0);

    EXPECT_NEAR(1.0, house_vector[0], 1e-14);
    EXPECT_NEAR(0.0, house_vector[1], 1e-14);
    EXPECT_NEAR(0.0, house_beta, 1e-14);
}

TEST(MatrixSVDTest, MatrixHouseholderNormalTest)
{
    double vec[3];
    vec[0] = 1;
    vec[1] = 4;
    vec[2] = 7;

    double house_vector[3];
    double house_beta;

    math::internal::matrix_householder_vector(vec, 3, house_vector, &house_beta,
        1e-14, 1.0);

    EXPECT_NEAR(1.0, house_vector[0], 1e-14);
    EXPECT_NEAR(-0.561479286439136, house_vector[1], 1e-14);
    EXPECT_NEAR(-0.982588751268488, house_vector[2], 1e-14);
    EXPECT_NEAR(0.876908509020667, house_beta, 1e-14);
}

TEST(MatrixSVDTest, MatrixHouseholderMatrixTest)
{
    double house_vector[3];
    house_vector[0] = 1.000000000000000;
    house_vector[1] = -0.561479286439136;
    house_vector[2] = -0.982588751268488;

    double house_beta;
    house_beta = 0.876908509020667;

    double groundtruth_house_matrix[9];
    groundtruth_house_matrix[0] = 0.123091490979333;
    groundtruth_house_matrix[1] = 0.492365963917331;
    groundtruth_house_matrix[2] = 0.861640436855329;
    groundtruth_house_matrix[3] = 0.492365963917331;
    groundtruth_house_matrix[4] = 0.723546709912780;
    groundtruth_house_matrix[5] = -0.483793257652636;
    groundtruth_house_matrix[6] = 0.861640436855329;
    groundtruth_house_matrix[7] = -0.483793257652636;
    groundtruth_house_matrix[8] = 0.153361799107888;

    double house_matrix[9];
    math::internal::matrix_householder_matrix(house_vector, 3, house_beta,
        house_matrix);

    for (int i = 0; i < 9; ++i)
    {
        EXPECT_NEAR(groundtruth_house_matrix[i], house_matrix[i], 1e-14);
    }
}

TEST(MatrixSVDTest, MatrixHouseholderApplicationTest)
{
    double house_matrix[9];
    house_matrix[0] = 0.123091490979333;
    house_matrix[1] = 0.492365963917331;
    house_matrix[2] = 0.861640436855329;
    house_matrix[3] = 0.492365963917331;
    house_matrix[4] = 0.723546709912780;
    house_matrix[5] = -0.483793257652636;
    house_matrix[6] = 0.861640436855329;
    house_matrix[7] = -0.483793257652636;
    house_matrix[8] = 0.153361799107888;

    double groundtruth_mat[9];
    groundtruth_mat[0] = 8.124038404635961;
    groundtruth_mat[1] = 9.601136296387953;
    groundtruth_mat[2] = 11.078234188139946;
    groundtruth_mat[3] = 0;
    groundtruth_mat[4] = 0.732119416177475;
    groundtruth_mat[5] = 1.464238832354949;
    groundtruth_mat[6] = 0;
    groundtruth_mat[7] = 0.531208978310581;
    groundtruth_mat[8] = 1.062417956621162;

    double mat[9];
    for (int i =0; i < 9; ++i)
    {
        mat[i] = static_cast<double>(i + 1);
    }

    math::internal::matrix_apply_householder_matrix(mat, 3, 3, house_matrix, 3, 0, 0);

    for (int i = 0; i < 9; ++i)
    {
        EXPECT_NEAR(groundtruth_mat[i], mat[i], 1e-14);
    }
}

TEST(MatrixSVDTest, MatrixBidiagonalizationStandardTest)
{
    const int M = 5;
    const int N = 4;
    double test_matrix[M * N];
    for (int i = 0; i < M*N; ++i)
    {
        test_matrix[i] = static_cast<double>(i + 1);
    }

    double mat_u[M * M];
    double mat_b[M * N];
    double mat_v[N * N];

    math::internal::matrix_bidiagonalize(test_matrix, M, N, mat_u, mat_b, mat_v, 1e-14);

    double groundtruth_mat_u[M * M];
    groundtruth_mat_u[0] = 0.042070316191167;
    groundtruth_mat_u[1] = 0.773453352501348;
    groundtruth_mat_u[2] = -0.565028398052320;
    groundtruth_mat_u[3] = -0.167888229103364;
    groundtruth_mat_u[4] = 0.229251939845590;
    groundtruth_mat_u[5] = 0.210351580955836;
    groundtruth_mat_u[6] = 0.505719499712420;
    groundtruth_mat_u[7] = 0.667776546677112;
    groundtruth_mat_u[8] = 0.448708735648741;
    groundtruth_mat_u[9] = 0.229640924620372;
    groundtruth_mat_u[10] = 0.378632845720504;
    groundtruth_mat_u[11] = 0.237985646923492;
    groundtruth_mat_u[12] = 0.312183334193103;
    groundtruth_mat_u[13] = -0.623816560842154;
    groundtruth_mat_u[14] = -0.559816455877411;
    groundtruth_mat_u[15] = 0.546914110485173;
    groundtruth_mat_u[16] = -0.029748205865437;
    groundtruth_mat_u[17] = -0.367582716208264;
    groundtruth_mat_u[18] = 0.573059831151541;
    groundtruth_mat_u[19] = -0.486297621488654;
    groundtruth_mat_u[20] = 0.715195375249841;
    groundtruth_mat_u[21] = -0.297482058654365;
    groundtruth_mat_u[22] = -0.047348766609631;
    groundtruth_mat_u[23] = -0.230063776854764;
    groundtruth_mat_u[24] = 0.587221212900103;

    double groundtruth_mat_b[M * N];
    std::fill(groundtruth_mat_b, groundtruth_mat_b + M * N, 0);
    groundtruth_mat_b[0] = 23.769728648009426;
    groundtruth_mat_b[1] = 47.80352488206745;
    groundtruth_mat_b[5] = 4.209811764688732;
    groundtruth_mat_b[6] = 1.449308026420137;
    groundtruth_mat_b[10] = -0.000000000000001;
    groundtruth_mat_b[16] = 0.000000000000002;

    double groundtruth_mat_v[N * N];
    groundtruth_mat_v[0] = 1;
    groundtruth_mat_v[1] = 0;
    groundtruth_mat_v[2] = 0;
    groundtruth_mat_v[3] = 0;
    groundtruth_mat_v[4] = 0;
    groundtruth_mat_v[5] = 0.536841016220519;
    groundtruth_mat_v[6] = -0.738332619241934;
    groundtruth_mat_v[7] = 0.408248290463862;
    groundtruth_mat_v[8] = 0;
    groundtruth_mat_v[9] = 0.576444042007278;
    groundtruth_mat_v[10] = -0.032335735149281;
    groundtruth_mat_v[11] = -0.816496580927726;
    groundtruth_mat_v[12] = 0;
    groundtruth_mat_v[13] = 0.616047067794038;
    groundtruth_mat_v[14] = 0.673661148943370;
    groundtruth_mat_v[15] = 0.408248290463864;
}

TEST(MatrixSVDTest, MatrixBidiagonalizationQuadraticTest)
{
    const int M = 3;
    double test_matrix[M * M];
    for (int i = 0; i < M*M; ++i)
    {
        test_matrix[i] = static_cast<double>(i + 1);
    }

    double mat_u[M * M];
    double mat_b[M * M];
    double mat_v[M * M];

    math::internal::matrix_bidiagonalize(test_matrix, M, M, mat_u, mat_b, mat_v, 1e-14);

    double groundtruth_mat_u[M * M];
    groundtruth_mat_u[0] = 0.123091490979333;
    groundtruth_mat_u[1] = 0.904534033733291;
    groundtruth_mat_u[2] = -0.408248290463863;
    groundtruth_mat_u[3] = 0.492365963917331;
    groundtruth_mat_u[4] = 0.301511344577764;
    groundtruth_mat_u[5] = 0.816496580927726;
    groundtruth_mat_u[6] = 0.861640436855329;
    groundtruth_mat_u[7] = -0.301511344577764;
    groundtruth_mat_u[8] = -0.408248290463863;

    double groundtruth_mat_b[M * M];
    groundtruth_mat_b[0] = 8.124038404635961;
    groundtruth_mat_b[1] = 14.659777996582722;
    groundtruth_mat_b[2] = 0;
    groundtruth_mat_b[3] = 0;
    groundtruth_mat_b[4] = 1.959499950338375;
    groundtruth_mat_b[5] = -0.501267429156329;
    groundtruth_mat_b[6] = 0;
    groundtruth_mat_b[7] = 0;
    groundtruth_mat_b[8] = 0;

    double groundtruth_mat_v[M * M];
    groundtruth_mat_v[0] = 1;
    groundtruth_mat_v[1] = 0;
    groundtruth_mat_v[2] = 0;
    groundtruth_mat_v[3] = 0;
    groundtruth_mat_v[4] = 0.654930538417842;
    groundtruth_mat_v[5] = 0.755689082789818;
    groundtruth_mat_v[6] = 0;
    groundtruth_mat_v[7] = 0.755689082789818;
    groundtruth_mat_v[8] = -0.654930538417842;

    for (int i = 0; i < M * M; ++i)
    {
        EXPECT_NEAR(groundtruth_mat_u[i], mat_u[i], 1e-14);
        EXPECT_NEAR(groundtruth_mat_b[i], mat_b[i], 1e-14);
        EXPECT_NEAR(groundtruth_mat_v[i], mat_v[i], 1e-14);
    }
}

TEST(MatrixSVDTest, MatrixBidiagonalizationScalarTest)
{
    double mat_a = 2;
    double mat_u, mat_b, mat_v;

    math::internal::matrix_bidiagonalize(&mat_a, 1, 1, &mat_u, &mat_b, &mat_v, 1e-14);
    EXPECT_NEAR(1, mat_u, 1e-14);
    EXPECT_NEAR(2, mat_b, 1e-14);
    EXPECT_NEAR(1, mat_v, 1e-14);
}

TEST(MatrixSVDTest, MatrixSVDQuadraticSTest)
{
    double mat_a[9];
    for (int i = 0; i < 9; ++i)
    {
        mat_a[i] = static_cast<double>(i + 1);
    }

    double mat_u[9];
    double mat_s[3];
    double mat_v[9];

    math::internal::matrix_gk_svd(mat_a, 3, 3, mat_u, mat_s, mat_v, 1e-6);

    double groundtruth_mat_s[3];
    groundtruth_mat_s[0] = 16.848103352614210;
    groundtruth_mat_s[1] = 1.068369514554709;
    groundtruth_mat_s[2] = 0;

    EXPECT_NEAR(groundtruth_mat_s[0], mat_s[0], 1e-14);
    EXPECT_NEAR(groundtruth_mat_s[1], mat_s[1], 1e-14);
    EXPECT_NEAR(groundtruth_mat_s[2], mat_s[2], 1e-14);
}

TEST(MatrixSVDTest, MatrixSVDQuadraticUVTest)
{
    double mat_a[9];
    for (int i = 0; i < 9; ++i)
    {
        mat_a[i] = static_cast<double>(i + 1);
    }

    double mat_u[9];
    double mat_s[3];
    double mat_v[9];

    math::internal::matrix_gk_svd(mat_a, 3, 3, mat_u, mat_s, mat_v, 1e-14);

    double groundtruth_mat_u[9];
    groundtruth_mat_u[0] = -0.214837238368396;
    groundtruth_mat_u[1] = -0.887230688346370;
    groundtruth_mat_u[2] = 0.408248290463863;
    groundtruth_mat_u[3] = -0.520587389464737;
    groundtruth_mat_u[4] = -0.249643952988298;
    groundtruth_mat_u[5] = -0.816496580927726;
    groundtruth_mat_u[6] = -0.826337540561078;
    groundtruth_mat_u[7] = 0.387942782369775;
    groundtruth_mat_u[8] = 0.408248290463863;

    double groundtruth_mat_v[9];
    groundtruth_mat_v[0] = -0.479671177877772;
    groundtruth_mat_v[1] = 0.776690990321559;
    groundtruth_mat_v[2] = -0.408248290463863;
    groundtruth_mat_v[3] = -0.572367793972062;
    groundtruth_mat_v[4] = 0.075686470104559;
    groundtruth_mat_v[5] = 0.816496580927726;
    groundtruth_mat_v[6] = -0.665064410066353;
    groundtruth_mat_v[7] = -0.625318050112443;
    groundtruth_mat_v[8] = -0.408248290463863;

    for (int i = 0; i < 9; ++i)
    {
        EXPECT_NEAR(groundtruth_mat_u[i], mat_u[i], 1e-14);
        EXPECT_NEAR(groundtruth_mat_v[i], mat_v[i], 1e-14);
    }
}

TEST(MatrixSVDTest, MatrixSVDNonQuadraticFullTest)
{
    double mat_a[20];
    for (int i = 0; i < 20; ++i)
    {
        mat_a[i] = static_cast<double>(i + 1);
    }

    double mat_u[20];
    double mat_s[4];
    double mat_v[20];

    math::internal::matrix_gk_svd(mat_a, 5, 4, mat_u, mat_s, mat_v, 1e-14);

    double groundtruth_mat_s[4];
    groundtruth_mat_s[0] = 53.520222492850067;
    groundtruth_mat_s[1] = 2.363426393147627;
    groundtruth_mat_s[2] = 0;
    groundtruth_mat_s[3] = 0;

    double groundtruth_mat_u[20];
    groundtruth_mat_u[0] = -0.096547843444803;
    groundtruth_mat_u[1] = -0.768556122821332;
    groundtruth_mat_u[2] = 0.565028398052320;
    groundtruth_mat_u[3] = 0.167888229103364;
    groundtruth_mat_u[4] = -0.245515644353003;
    groundtruth_mat_u[5] = -0.489614203611302;
    groundtruth_mat_u[6] = -0.667776546677112;
    groundtruth_mat_u[7] = -0.448708735648741;
    groundtruth_mat_u[8] = -0.394483445261204;
    groundtruth_mat_u[9] = -0.210672284401273;
    groundtruth_mat_u[10] = -0.312183334193103;
    groundtruth_mat_u[11] = 0.623816560842154;
    groundtruth_mat_u[12] = -0.543451246169405;
    groundtruth_mat_u[13] = 0.068269634808757;
    groundtruth_mat_u[14] = 0.367582716208264;
    groundtruth_mat_u[15] = -0.573059831151541;
    groundtruth_mat_u[16] = -0.692419047077605;
    groundtruth_mat_u[17] = 0.347211554018787;
    groundtruth_mat_u[18] = 0.047348766609631;
    groundtruth_mat_u[19] = 0.230063776854764;

    double groundtruth_mat_v[20];
    groundtruth_mat_v[0] = -0.443018843508167;
    groundtruth_mat_v[1] = 0.709742421091395;
    groundtruth_mat_v[2] = 0.547722557505176;
    groundtruth_mat_v[3] = 0;
    groundtruth_mat_v[4] = -0.479872524872618;
    groundtruth_mat_v[5] = 0.264049919281154;
    groundtruth_mat_v[6] = -0.730296743340218;
    groundtruth_mat_v[7] = 0.408248290463862;
    groundtruth_mat_v[8] = -0.516726206237069;
    groundtruth_mat_v[9] = -0.181642582529112;
    groundtruth_mat_v[10] = -0.182574185835057;
    groundtruth_mat_v[11] = -0.816496580927726;
    groundtruth_mat_v[12] = -0.553579887601520;
    groundtruth_mat_v[13] = -0.627335084339378;
    groundtruth_mat_v[14] = 0.365148371670102;
    groundtruth_mat_v[15] = 0.408248290463864;

    for (int i = 0; i < 4; ++i)
    {
        EXPECT_NEAR(groundtruth_mat_s[i], mat_s[i], 1e-13);
    }
    for (int i = 0; i < 20; ++i)
    {
        EXPECT_NEAR(groundtruth_mat_u[i], mat_u[i], 1e-13);
    }
    for (int i = 0; i < 16; ++i)
    {
        EXPECT_NEAR(std::abs(groundtruth_mat_v[i]), std::abs(mat_v[i]), 1e-13);
    }

}
TEST(MatrixSVDTest, MatrixSVDNonQuadraticEconomyTest)
{
    double mat_a[20];
    for (int i = 0; i < 20; ++i)
    {
        mat_a[i] = static_cast<double>(i + 1);
    }

    double mat_u[20];
    double mat_s[4];
    double mat_v[20];

    math::internal::matrix_r_svd(mat_a, 5, 4, mat_u, mat_s, mat_v, 1e-10);

    double groundtruth_mat_s[4];
    groundtruth_mat_s[0] = 53.520222492850067;
    groundtruth_mat_s[1] = 2.363426393147627;
    groundtruth_mat_s[2] = 0;
    groundtruth_mat_s[3] = 0;

    double groundtruth_mat_u[20];
    groundtruth_mat_u[0] = -0.096547843444803;
    groundtruth_mat_u[1] = -0.768556122821332;
    groundtruth_mat_u[2] = 0.632455532033676;
    groundtruth_mat_u[3] = 0;
    groundtruth_mat_u[4] = -0.245515644353003;
    groundtruth_mat_u[5] = -0.489614203611302;
    groundtruth_mat_u[6] = -0.632455532033676;
    groundtruth_mat_u[7] = 0.547722557505167;
    groundtruth_mat_u[8] = -0.394483445261204;
    groundtruth_mat_u[9] = -0.210672284401272;
    groundtruth_mat_u[10] = -0.316227766016837;
    groundtruth_mat_u[11] = -0.730296743340219;
    groundtruth_mat_u[12] = -0.543451246169405;
    groundtruth_mat_u[13] = 0.068269634808755;
    groundtruth_mat_u[14] = -0.000000000000002;
    groundtruth_mat_u[15] = -0.182574185835059;
    groundtruth_mat_u[16] = -0.692419047077606;
    groundtruth_mat_u[17] = 0.347211554018788;
    groundtruth_mat_u[18] = 0.316227766016839;
    groundtruth_mat_u[19] = 0.365148371670112;

    double groundtruth_mat_v[16];
    groundtruth_mat_v[0] = -0.443018843508167;
    groundtruth_mat_v[1] = 0.709742421091395;
    groundtruth_mat_v[2] = 0.547722557505176;
    groundtruth_mat_v[3] = 0;
    groundtruth_mat_v[4] = -0.479872524872618;
    groundtruth_mat_v[5] = 0.264049919281154;
    groundtruth_mat_v[6] = -0.730296743340218;
    groundtruth_mat_v[7] = 0.408248290463862;
    groundtruth_mat_v[8] = -0.516726206237069;
    groundtruth_mat_v[9] = -0.181642582529112;
    groundtruth_mat_v[10] = -0.182574185835057;
    groundtruth_mat_v[11] = -0.816496580927726;
    groundtruth_mat_v[12] = -0.553579887601520;
    groundtruth_mat_v[13] = -0.627335084339378;
    groundtruth_mat_v[14] = 0.365148371670102;
    groundtruth_mat_v[15] = 0.408248290463864;

    for (int i = 0; i < 4; ++i)
    {
        EXPECT_NEAR(groundtruth_mat_s[i], mat_s[i], 1e-10);
    }
    for (int i = 0; i < 20; ++i)
    {
        EXPECT_NEAR(groundtruth_mat_u[i], mat_u[i], 1e-10);
    }
    for (int i = 0; i < 16; ++i)
    {
        EXPECT_NEAR(std::abs(groundtruth_mat_v[i]), std::abs(mat_v[i]), 1e-10);
    }

}

TEST(MatrixSVDTest, MatrixTransposeTest)
{
    int mat_a[6];
    mat_a[0] = 1;
    mat_a[1] = 3;
    mat_a[2] = 5;
    mat_a[3] = 2;
    mat_a[4] = 4;
    mat_a[5] = 6;

    math::matrix_transpose(mat_a, 2, 3);

    int groundtruth_mat_a_t[6];
    groundtruth_mat_a_t[0] = 1;
    groundtruth_mat_a_t[1] = 2;
    groundtruth_mat_a_t[2] = 3;
    groundtruth_mat_a_t[3] = 4;
    groundtruth_mat_a_t[4] = 5;
    groundtruth_mat_a_t[5] = 6;

    for (int i = 0; i < 6; ++i)
    {
        EXPECT_NEAR(groundtruth_mat_a_t[i], mat_a[i], 1e-14);
    }
}

TEST(MatrixSVDTest, MatrixSVDUnderdeterminedTest)
{
    double mat_a[2 * 3];
    mat_a[0] = 1.0;
    mat_a[1] = 3.0;
    mat_a[2] = 5.0;
    mat_a[3] = 2.0;
    mat_a[4] = 4.0;
    mat_a[5] = 6.0;

    double mat_u[2 * 3];
    double mat_s[3];
    double mat_v[3 * 3];
    math::matrix_svd(mat_a, 2, 3, mat_u, mat_s, mat_v, 1e-14);

    double groundtruth_mat_u[2 * 3];
    groundtruth_mat_u[0] = -0.619629483829340;
    groundtruth_mat_u[1] = -0.784894453267053;
    groundtruth_mat_u[2] = 0.0;
    groundtruth_mat_u[3] = -0.784894453267052;
    groundtruth_mat_u[4] = 0.619629483829340;
    groundtruth_mat_u[5] = 0.0;

    double groundtruth_mat_s[3];
    groundtruth_mat_s[0] = 9.525518091565104;
    groundtruth_mat_s[1] = 0.514300580658644;
    groundtruth_mat_s[2] = 0.0;

    double groundtruth_mat_v[3 * 3];
    groundtruth_mat_v[0] = -0.229847696400071;
    groundtruth_mat_v[1] = 0.883461017698525;
    groundtruth_mat_v[2] = -0.408248290463863;
    groundtruth_mat_v[3] = -0.524744818760294;
    groundtruth_mat_v[4] = 0.240782492132546;
    groundtruth_mat_v[5] = 0.816496580927726;
    groundtruth_mat_v[6] = -0.819641941120516;
    groundtruth_mat_v[7] = -0.401896033433432;
    groundtruth_mat_v[8] = -0.408248290463863;

    for (int i = 0; i < 6; ++i)
        EXPECT_NEAR(groundtruth_mat_u[i], mat_u[i], 1e-13);
    for (int i = 0; i < 3; ++i)
        EXPECT_NEAR(groundtruth_mat_s[i], mat_s[i], 1e-13);
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR(groundtruth_mat_v[i], mat_v[i], 1e-13);
}

TEST(MatrixSVDTest, TestLargeBeforeAfter)
{
    const int rows = 100;
    const int cols = 50;
    double mat_a[rows * cols];
    for (int i = 0; i < rows * cols; ++i)
        mat_a[i] = static_cast<double>(i + 1);

    /* Allocate results on heap to allow SVD of very big matrices. */
    double* mat_u = new double[rows * rows];
    double* vec_s = new double[cols];
    double* mat_s = new double[cols * cols];
    double* mat_v = new double[cols * cols];

    math::matrix_svd(mat_a, rows, cols, mat_u, vec_s, mat_v, 1e-8);
    math::matrix_transpose(mat_v, cols, cols);
    std::fill(mat_s, mat_s + cols * cols, 0.0);
    for (int i = 0; i < cols; ++i)
        mat_s[i * cols + i] = vec_s[i];

    double* multiply_temp = new double[cols * cols];
    double* multiply_result = new double[rows * cols];
    math::matrix_multiply(mat_s, cols, cols, mat_v, cols, multiply_temp);
    math::matrix_multiply(mat_u, rows, cols, multiply_temp, cols,
        multiply_result);

    for (int i = 0; i < rows * cols; ++i)
        EXPECT_NEAR(mat_a[i], multiply_result[i], 1e-7);

    delete[] mat_u;
    delete[] vec_s;
    delete[] mat_s;
    delete[] mat_v;
    delete[] multiply_temp;
    delete[] multiply_result;
}

TEST(MatrixSVDTest, BeforeAfter1)
{
    math::Matrix<double, 2, 2> A;
    A(0,0) = 1.0f; A(0,1) = 2.0f;
    A(1,0) = 3.0f; A(1,1) = 4.0f;

    math::Matrix<double, 2, 2> U;
    math::Matrix<double, 2, 2> S;
    math::Matrix<double, 2, 2> V;
    math::matrix_svd(A, &U, &S, &V, 1e-12);

    EXPECT_TRUE(A.is_similar(U * S * V.transposed(), 1e-12));
    EXPECT_GT(S(0,0), S(1,1));
}

TEST(MatrixSVDTest, BeforeAfter2)
{
    double values[] = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
    math::Matrix<double, 3, 2> A(values);

    math::Matrix<double, 3, 2> U;
    math::Matrix<double, 2, 2> S;
    math::Matrix<double, 2, 2> V;
    math::matrix_svd(A, &U, &S, &V, 1e-12);

    EXPECT_TRUE(A.is_similar(U * S * V.transposed(), 1e-12));
    EXPECT_GT(S(0,0), S(1,1));
}

TEST(MatrixSVDTest, BeforeAfter3)
{
    double values[] = {1.0, 2.0, 3.0, 4.0 };
    math::Matrix<double, 2, 2> A(values);

    math::Matrix<double, 2, 2> U;
    math::Matrix<double, 2, 2> S(0.0);
    math::Matrix<double, 2, 2> V;
    math::matrix_svd<double>(*A, 2, 2, *U, *S, *V, 1e-12);
    std::swap(S(0,1), S(1,1)); // Swap the eigenvalues into place.

    EXPECT_TRUE(A.is_similar(U * S * V.transposed(), 1e-12));
    EXPECT_GT(S(0,0), S(1,1));
}

TEST(MatrixSVDTest, BeforeAfter4)
{
    double values[] = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
    math::Matrix<double, 3, 2> A(values);

    math::Matrix<double, 3, 2> U;
    math::Matrix<double, 2, 2> S(0.0);
    math::Matrix<double, 2, 2> V;
    math::matrix_svd(*A, 3, 2, *U, *S, *V, 1e-12);
    std::swap(S(0,1), S(1,1)); // Swap the eigenvalues into place.

    EXPECT_TRUE(A.is_similar(U * S * V.transposed(), 1e-12));
    EXPECT_GT(S(0,0), S(1,1));
}

TEST(MatrixSVDTest, BeforeAfter5)
{
    math::Matrix<double, 3, 3> A;
    A[0] = 1.0; A[1] = 0.0; A[2] = 1.0;
    A[3] = 1.0; A[4] = 0.0; A[5] = 1.0;
    A[6] = 1.0; A[7] = 0.0; A[8] = 1.0;

    math::Matrix<double, 3, 3> U, S, V;
    math::matrix_svd<double>(A, &U, &S, &V);
    math::Matrix<double, 3, 3> X = U * S * V.transposed();
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR(A[i], X[i], 1e-6) << " at " << i;
}

TEST(MatrixSVDTest, BeforeAfter6)
{
    math::Matrix<double, 3, 3> A;
    A[0] = 1.0; A[1] = 1.0; A[2] = 0.0;
    A[3] = 1.0; A[4] = 1.0; A[5] = 0.0;
    A[6] = 1.0; A[7] = 1.0; A[8] = 0.0;

    math::Matrix<double, 3, 3> U, S, V;
    math::matrix_svd<double>(A, &U, &S, &V);
    math::Matrix<double, 3, 3> X = U * S * V.transposed();
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR(A[i], X[i], 1e-6) << " at " << i;
}

TEST(MatrixSVDTest, BeforeAfter7)
{
    math::Matrix<double, 3, 3> A;
    A[0] = 0.0; A[1] = 1.0; A[2] = 1.0;
    A[3] = 0.0; A[4] = 1.0; A[5] = 1.0;
    A[6] = 0.0; A[7] = 1.0; A[8] = 1.0;

    math::Matrix<double, 3, 3> U, S, V;
    math::matrix_svd<double>(A, &U, &S, &V);
    math::Matrix<double, 3, 3> X = U * S * V.transposed();
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR(A[i], X[i], 1e-6) << " at " << i;
}

TEST(MatrixSVDTest, BeforeAfter8)
{
    math::Matrix<double, 3, 3> A;
    A[0] = 0.0; A[1] = 0.0; A[2] = 0.0;
    A[3] = 1.0; A[4] = 1.0; A[5] = 1.0;
    A[6] = 1.0; A[7] = 1.0; A[8] = 1.0;

    math::Matrix<double, 3, 3> U, S, V;
    math::matrix_svd<double>(A, &U, &S, &V);
    math::Matrix<double, 3, 3> X = U * S * V.transposed();
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR(A[i], X[i], 1e-6) << " at " << i;
}

TEST(MatrixSVDTest, BeforeAfter9)
{
    math::Matrix<double, 3, 3> A;
    A[0] = 1.0; A[1] = 1.0; A[2] = 1.0;
    A[3] = 0.0; A[4] = 0.0; A[5] = 0.0;
    A[6] = 1.0; A[7] = 1.0; A[8] = 1.0;

    math::Matrix<double, 3, 3> U, S, V;
    math::matrix_svd<double>(A, &U, &S, &V);
    math::Matrix<double, 3, 3> X = U * S * V.transposed();
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR(A[i], X[i], 1e-6) << " at " << i;
}

TEST(MatrixSVDTest, BeforeAfter10)
{
    math::Matrix<double, 3, 3> A;
    A[0] = 1.0; A[1] = 1.0; A[2] = 1.0;
    A[3] = 1.0; A[4] = 1.0; A[5] = 1.0;
    A[6] = 0.0; A[7] = 0.0; A[8] = 0.0;

    math::Matrix<double, 3, 3> U, S, V;
    math::matrix_svd<double>(A, &U, &S, &V);
    math::Matrix<double, 3, 3> X = U * S * V.transposed();
    for (int i = 0; i < 9; ++i)
        EXPECT_NEAR(A[i], X[i], 1e-6) << " at " << i;
}

TEST(MatrixSVDTest, SortedEigenvalues)
{
    double values[] = { 0.0697553, 0.949327, 0.525995, 0.0860558, 0.192214, 0.663227 };
    math::Matrix<double, 3, 2> U, mat(values);
    math::Matrix<double, 2, 2> S, V;
    math::matrix_svd<double, 3, 2>(mat, &U, &S, &V, 1e-12);
    EXPECT_GT(S(0,0), S(1,1));
}

#if 0
namespace
{
    double
    getrand (void)
    {
        return std::rand() / static_cast<double>(RAND_MAX);
    }

} // namespace

TEST(MatrixSVDTest, ForbiddenRandomTestLargeMatrix)
{
#define TEST_ROWS 10000
#define TEST_COLS 10
    std::srand(0);
    math::Matrix<double, TEST_ROWS, TEST_COLS> A;
    for (int i = 0; i < TEST_ROWS * TEST_COLS; ++i)
        A[i] = getrand();

    math::Matrix<double, TEST_ROWS, TEST_COLS> U;
    math::Matrix<double, TEST_COLS, TEST_COLS> S;
    math::Matrix<double, TEST_COLS, TEST_COLS> V;
    math::matrix_svd(A, &U, &S, &V, 1e-10);

    math::Matrix<double, TEST_ROWS, TEST_COLS> X = U * S * V.transposed();
    for (int i = 0; i < TEST_ROWS * TEST_COLS; ++i)
        EXPECT_NEAR(A[i], X[i], 1e-10);
}

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

        math::Matrix<double, 3, 2> U;
        math::Matrix<double, 2, 2> S;
        math::Matrix<double, 2, 2> V;
        math::matrix_svd(A, &U, &S, &V, 1e-12);

        EXPECT_TRUE(A.is_similar(U * S * V.transposed(), 1e-12));
        EXPECT_GT(S(0,0), S(1,1));
    }
}
#endif

TEST(MatrixSVDTest, TestPseudoInverse)
{
    math::Matrix<double, 3, 2> mat;
    mat[0] = 1.0;  mat[1] = 2.0;
    mat[2] = 3.0;  mat[3] = 4.0;
    mat[4] = 5.0;  mat[5] = 6.0;

    math::Matrix<double, 2, 3> pinv;
    math::matrix_pseudo_inverse(mat, &pinv, 1e-10);

    math::Matrix<double, 3, 2> mat2 = mat * pinv * mat;
    for (int i = 0; i < 6; ++i)
        EXPECT_NEAR(mat2[i], mat[i], 1e-14);

    math::Matrix<double, 2, 3> pinv2 = pinv * mat * pinv;
    for (int i = 0; i < 6; ++i)
        EXPECT_NEAR(pinv2[i], pinv[i], 1e-14);
}

TEST(MatrixSVDTest, TestPseudoInverseGoldenData1)
{
    double a_values[] = { 2, -4,  5, 6, 0, 3, 2, -4, 5, 6, 0, 3 };
    double a_inv_values[] = { -2, 6, -2, 6, -5, 3, -5, 3, 4, 0, 4, 0 };
    math::Matrix<double, 4, 3> A(a_values);
    math::Matrix<double, 3, 4> Ainv(a_inv_values);
    Ainv /= 72.0;

    math::Matrix<double, 3, 4> result;
    math::matrix_pseudo_inverse(A, &result, 1e-10);

    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 4; ++c)
            EXPECT_NEAR(Ainv(r, c), result(r, c), 1e-16);
}

TEST(MatrixSVDTest, TestPseudoInverseGoldenData2)
{
    double a_values[] = { 1, 1, 1, 1, 5, 7, 7, 9 };
    double a_inv_values[] = { 2, -0.25, 0.25, 0, 0.25, 0, -1.5, 0.25 };
    math::Matrix<double, 2, 4> A(a_values);
    math::Matrix<double, 4, 2> Ainv(a_inv_values);

    math::Matrix<double, 4, 2> result;
    math::matrix_pseudo_inverse(A, &result, 1e-10);

    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 2; ++c)
            EXPECT_NEAR(Ainv(r, c), result(r, c), 1e-13);
}
