#ifndef MATH_MATRIX_QR_DECOMP_HEADER
#define MATH_MATRIX_QR_DECOMP_HEADER

#include "Eigen/Dense"

#include "math/defines.h"
#include "math/matrix.h"

MATH_NAMESPACE_BEGIN

template <typename T, int N>
void
matrix_qr_decomp (Matrix<T, N, N> const& mat_a,
    Matrix<T, N, N>* mat_q, Matrix<T, N, N>* mat_r);

MATH_NAMESPACE_END

/* ------------------------ Implementation ------------------------ */

#include "Eigen/Dense"

MATH_NAMESPACE_BEGIN

template <typename T, int N>
void
matrix_qr_decomp (Matrix<T, N, N> const& mat_a,
    Matrix<T, N, N>* mat_q, Matrix<T, N, N>* mat_r)
{
    typedef Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic,
        Eigen::RowMajor> RowMatrixXd;

    Eigen::Map<RowMatrixXd const> A(*mat_a, N, N);
    Eigen::HouseholderQR<RowMatrixXd> qr(A);
    RowMatrixXd Q = qr.householderQ();
    RowMatrixXd R = qr.matrixQR();
    for (int r = 0, i = 0; r < N; ++r)
        for (int c = 0; c < N; ++c, ++i)
            (*mat_q)[i] = Q(r, c);
    mat_r->fill(T(0));
    for (int r = 0, i = 0; r < N; ++r)
        for (int c = r; c < N; ++c, ++i)
            (*mat_r)(r, c) = R(r, c);
}

MATH_NAMESPACE_END

#endif /* MATH_MATRIX_QR_DECOMP_HEADER */
