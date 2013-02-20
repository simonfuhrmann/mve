/*
 * Matrix Singular Value Decomposition (SVD) Eigen adaptor for MVE.
 * Written by Simon Fuhrmann.
 *
 * Note: Using Eigen is a temporary solution and should be replaced with
 * a nice and small SVD implementation.
 */

#ifndef MATH_MATRIX_SVD_HEADER
#define MATH_MATRIX_SVD_HEADER

#include "Eigen/Dense"

#include "math/defines.h"
#include "math/matrix.h"

MATH_NAMESPACE_BEGIN

/**
 *
 */
template <typename T, int M, int N>
void
matrix_svd (Matrix<T, M, N> const& mat_a, Matrix<T, M, M>* mat_u,
    Matrix<T, M, N>* mat_s, Matrix<T, N, N>* mat_v);

/**
 * Computes the SVD of the m-by-n matrix 'mat_a' in row-major format.
 * The result is placed in 'mat_u' (m-by-m matrix), 'vec_s' (length min(m,n)),
 * and 'mat_v' (n-by-n matrix). Any of the results may be NULL.
 */
template <typename T>
void
matrix_svd (T const* mat_a, int rows, int cols, T* mat_u, T* vec_s, T* mat_v);

/**
 *
 */
template <typename T, int M, int N>
void
matrix_pseudo_inverse (math::Matrix<double, M, N> const& P,
    math::Matrix<double, N, M>* result);

/* ------------------------ Implementation ------------------------ */

template <typename T, int M, int N>
void
matrix_svd (Matrix<T, M, N> const& mat_a, Matrix<T, M, M>* mat_u,
    Matrix<T, M, N>* mat_s, Matrix<T, N, N>* mat_v)
{
    Eigen::MatrixXd eigen_a(M, N);
    for (int i = 0; i < M; ++i)
        for (int j = 0; j < N; ++j)
            eigen_a(i, j) = mat_a(i, j);

    typedef Eigen::JacobiSVD<Eigen::MatrixXd> EigenSVD;
    EigenSVD svd(eigen_a, Eigen::ComputeFullU | Eigen::ComputeFullV);
    EigenSVD::MatrixUType const& eigen_u = svd.matrixU();
    EigenSVD::MatrixVType const& eigen_v = svd.matrixV();
    EigenSVD::SingularValuesType const& eigen_s = svd.singularValues();

    for (int i = 0; i < M; ++i)
        for (int j = 0; j < M; ++j)
            (*mat_u)(i, j) = eigen_u(i, j);
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            (*mat_v)(i, j) = eigen_v(i, j);
    mat_s->fill(T(0));
    for (int i = 0; i < std::min(N, M); ++i)
        (*mat_s)(i, i) = eigen_s(i);
}

template <typename T>
void
matrix_svd (T const* mat_a, int rows, int cols, T* mat_u, T* vec_s, T* mat_v)
{
    typedef Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic,
        Eigen::RowMajor> RowMatrixXd;
    typedef Eigen::JacobiSVD<RowMatrixXd> EigenSVD;

    Eigen::Map<RowMatrixXd> eigen_a(const_cast<T*>(mat_a), rows, cols);
    unsigned int svd_options = 0;
    svd_options |= mat_u ? Eigen::ComputeFullU : 0;
    svd_options |= mat_v ? Eigen::ComputeFullV : 0;
    EigenSVD svd(eigen_a, svd_options);

    if (mat_u != NULL)
    {
        typename EigenSVD::MatrixUType const& eigen_u = svd.matrixU();
        for (int r = 0, i = 0; r < rows; ++r)
            for (int c = 0; c < rows; ++c, ++i)
                mat_u[i] = eigen_u(r, c);
    }
    if (vec_s != NULL)
    {
        typename EigenSVD::SingularValuesType const& eigen_s
            = svd.singularValues();
        for (int i = 0; i < std::min(rows, cols); ++i)
            vec_s[i] = eigen_s(i);
    }
    if (mat_v != NULL)
    {
        typename EigenSVD::MatrixVType const& eigen_v = svd.matrixV();
        for (int r = 0, i = 0; r < cols; ++r)
            for (int c = 0; c < cols; ++c, ++i)
                mat_v[i] = eigen_v(r, c);
    }
}

template <typename T, int M, int N>
void
matrix_pseudo_inverse (math::Matrix<T, M, N> const& A,
    math::Matrix<T, N, M>* result)
{
    math::Matrix<T, M, M> U;
    math::Matrix<T, M, N> S;
    math::Matrix<T, N, N> V;
    math::matrix_svd(A, &U, &S, &V);
    for (int i = 0; i < std::min(M, N); ++i)
        if (MATH_EPSILON_EQ(S(i, i), 0.0, 1e-12))
            S(i, i) = 0.0;
        else
            S(i, i) = 1.0 / S(i, i);

    *result = V * S.transposed() * U.transposed();
}

MATH_NAMESPACE_END

#endif // MATH_MATRIX_SVD_HEADER
