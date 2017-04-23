#ifndef SFM_BA_CHOLESKY_HEADER
#define SFM_BA_CHOLESKY_HEADER

#include <stdexcept>
#include <cmath>

#include "math/defines.h"
#include "math/matrix_tools.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN
SFM_BA_NAMESPACE_BEGIN

/**
 * Invert symmetric, positive definite matrix A using Cholesky decomposition
 * and inversion of the triangular matrix: A^-1 = (L^-1)^T * (L^-1).
 */
template <typename T>
void
cholesky_invert (T const* A, int const cols, T* A_inv);

/**
 * Invert symmetric, positive definite matrix A inplace using Cholesky.
 */
template <typename T>
void
cholesky_invert_inplace (T* A, int const cols);

/**
 * Cholesky decomposition of the symmetric, positive definite matrix
 * A = L * L^T. The resulting matrix L is a lower-triangular matrix.
 * If A and L are the same matrix, the decomposition is performed in-place.
 */
template <typename T>
void
cholesky_decomposition (T const* A, int const cols, T* L);

/**
 * Invert a lower-triangular matrix (e.g. obtained by Cholesky decomposition).
 * The inversion cannot be computed in-place.
 */
template <typename T>
void
invert_lower_diagonal (T const* A, int const cols, T* A_inv);

/* ------------------------ Implementation ------------------------ */

template <typename T>
void
cholesky_invert (T const* A, int const cols, T* A_inv)
{
    cholesky_decomposition(A, cols, A_inv);
    T* tmp = new T[cols * cols];
    invert_lower_diagonal(A_inv, cols, tmp);
    math::matrix_transpose_multiply(tmp, cols, cols, A_inv);
    delete[] tmp;
}

template <typename T>
void
cholesky_invert_inplace (T* A, int const cols)
{
    cholesky_decomposition(A, cols, A);
    T* tmp = new T[cols * cols];
    invert_lower_diagonal(A, cols, tmp);
    math::matrix_transpose_multiply(tmp, cols, cols, A);
    delete[] tmp;
}

template <typename T>
void
cholesky_decomposition (T const* A, int const cols, T* L)
{
    T* out_ptr = L;
    for (int r = 0; r < cols; ++r)
    {
        /* Compute left-of-diagonal entries. */
        for (int c = 0; c < r; ++c)
        {
            T result = T(0);
            for (int ci = 0; ci < c; ++ci)
                result += L[r * cols + ci] * L[c * cols + ci];
            result = A[r * cols + c] - result;
            (*out_ptr++) = result / L[c * cols + c];
        }

        /* Compute diagonal entry. */
        {
            T* L_row_ptr = L + r * cols;
            T result = T(0);
            for (int c = 0; c < r; ++c)
                result += MATH_POW2(L_row_ptr[c]);
            result = std::max(T(0), A[r * cols + r] - result);
            (*out_ptr++) = std::sqrt(result);
        }

        /* Set right-of-diagonal entries zero. */
        for (int c = r + 1; c < cols; ++c)
            (*out_ptr++) = T(0);
    }
}

template <typename T>
void
invert_lower_diagonal (T const* A, int const cols, T* A_inv)
{
    if (A == A_inv)
        throw std::invalid_argument("In-place inversion not possible");

    for (int r = 0; r < cols; ++r)
    {
        T const* A_row_ptr = A + r * cols;
        T* A_inv_row_ptr = A_inv + r * cols;

        /* Compute left-of-diagonal entries. */
        for (int c = 0; c < r; ++c)
        {
            T result = T(0);
            for (int ci = 0; ci < r; ++ci)
                result -= A_row_ptr[ci] * A_inv[ci * cols + c];
            A_inv_row_ptr[c] = result / A_row_ptr[r];
        }

        /* Compute diagonal entry. */
        A_inv_row_ptr[r] = T(1) / A_row_ptr[r];

        /* Set right-of-diagonal entries zero. */
        for (int c = r + 1; c < cols; ++c)
            A_inv_row_ptr[c] = T(0);
    }
}

SFM_BA_NAMESPACE_END
SFM_NAMESPACE_END

#endif // SFM_BA_CHOLESKY_HEADER
