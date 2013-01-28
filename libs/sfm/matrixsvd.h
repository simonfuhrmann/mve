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

MATH_NAMESPACE_END

#endif // MATH_MATRIX_SVD_HEADER
