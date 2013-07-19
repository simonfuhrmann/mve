/*
 * Matrix tools.
 * Written by Simon Fuhrmann.
 */

#ifndef MATH_MATRIXTOOLS_HEADER
#define MATH_MATRIXTOOLS_HEADER

#include "math/defines.h"
#include "math/matrix.h"
#include "math/vector.h"

MATH_NAMESPACE_BEGIN

/**
 * Creates a symmetric projection matrix as used in OpenGL.
 * Values in the frustum are mapped to the unit cube. The frustum
 * near plane is defined by z-near, right and top values, the far-plane
 * is located at z-far.
 */
template <typename T>
Matrix<T,4,4>
matrix_gl_projection (T const& znear, T const& zfar,
    T const& top, T const& right);

/**
 * Creates a symmetric inverse projection matrix as used in OpenGL.
 */
template <typename T>
Matrix<T,4,4>
matrix_inverse_gl_projection (T const& znear, T const& zfar,
    T const& top, T const& right);

/**
 * Creates a view transformation matrix for camera parameters given
 * as camera position, normalized viewing direction, and normalized
 * up-vector.
 */
template <typename T>
Matrix<T,4,4>
matrix_viewtrans (Vector<T,3> const& campos,
    Vector<T,3> const& viewdir, Vector<T,3> const& upvec);

/**
 * Creates an inverse view transformation matrix.
 */
template <typename T>
Matrix<T,4,4>
matrix_inverse_viewtrans (Vector<T,3> const& campos,
    Vector<T,3> const& viewdir, Vector<T,3> const& upvec);

/**
 * Inverts a transformation matrix.
 */
template <typename T>
Matrix<T,4,4>
matrix_invert_trans (Matrix<T,4,4> const& mat);

/**
 * Sets the given square matrix to the identity matrix.
 * The function returns a reference to the given matrix.
 */
template <typename T, int N>
Matrix<T,N,N>&
matrix_set_identity (Matrix<T,N,N>* mat);

/**
 * Sets the given square matrix of dimension 'n' to the identity matrix.
 * The function returns the argument pointer.
 */
template <typename T>
T*
matrix_set_identity (T* mat, int n);

/**
 * Returns true if and only if the given matrix is the identity matrix.
 */
template <typename T, int N>
bool
matrix_is_identity (Matrix<T,N,N> const& mat);

/**
 * Returns a diagonal matrix from the given vector.
 */
template <typename T, int N>
Matrix<T,N,N>
matrix_diagonal (math::Vector<T,N> const& v);

/**
 * Calculates the determinant of the given matrix.
 * This is specialized for 1x1, 2x2, 3x3 and 4x4 matrices only.
 */
template <typename T, int N>
T
matrix_determinant (Matrix<T,N,N> const& mat);

/**
 * Calculates the inverse of the given matrix.
 * This is specialized for 1x1, 2x2, 3x3 and 4x4 matrices only.
 */
template <typename T, int N>
Matrix<T,N,N>
matrix_inverse (Matrix<T,N,N> const& mat);

/**
 * Calculates the inverse of the given matrix given its determinant.
 * This is specialized for 1x1, 2x2 and 3x3 matrices only.
 */
template <typename T, int N>
Matrix<T,N,N>
matrix_inverse (Matrix<T,N,N> const& mat, T const& det);

MATH_NAMESPACE_END

/* ------------------------ Implementation ------------------------ */

MATH_NAMESPACE_BEGIN

template <typename T>
Matrix<T,4,4>
matrix_gl_projection (T const& znear, T const& zfar,
    T const& top, T const& right)
{
    Matrix<T,4,4> proj(0.0f);
    proj(0,0) = znear / right;
    proj(1,1) = znear / top;
    proj(2,2) = -(zfar + znear) / (zfar - znear);
    proj(2,3) = T(-2) * zfar * znear / (zfar - znear);
    proj(3,2) = -1;

    return proj;
}

template <typename T>
Matrix<T,4,4>
matrix_inverse_gl_projection (T const& znear, T const& zfar,
    T const& top, T const& right)
{
    Matrix<T,4,4> iproj(0.0f);
    iproj(0,0) = right / znear;
    iproj(1,1) = top / znear;
    iproj(2,3) = -1;
    iproj(3,2) = (zfar - znear) / (T(-2) * zfar * znear);
    iproj(3,3) = -(zfar + znear) / (T(-2) * zfar * znear);

    return iproj;
}

template <typename T>
Matrix<T,4,4>
matrix_viewtrans (Vector<T,3> const& campos,
    Vector<T,3> const& viewdir, Vector<T,3> const& upvec)
{
    /* Normalize x in case upvec is not perpendicular to viewdir. */
    Vector<T,3> z(-viewdir);
    Vector<T,3> x(upvec.cross(z).normalized());
    Vector<T,3> y(z.cross(x));

    Matrix<T,4,4> m;
    m(0,0) = x[0]; m(0,1) = x[1]; m(0,2) = x[2]; m(0,3) = 0.0f;
    m(1,0) = y[0]; m(1,1) = y[1]; m(1,2) = y[2]; m(1,3) = 0.0f;
    m(2,0) = z[0]; m(2,1) = z[1]; m(2,2) = z[2]; m(2,3) = 0.0f;
    m(3,0) = 0.0f; m(3,1) = 0.0f; m(3,2) = 0.0f; m(3,3) = 1.0f;

    Vector<T,3> t(-campos);
    m(0,3) = m(0,0) * t[0] + m(0,1) * t[1] + m(0,2) * t[2];
    m(1,3) = m(1,0) * t[0] + m(1,1) * t[1] + m(1,2) * t[2];
    m(2,3) = m(2,0) * t[0] + m(2,1) * t[1] + m(2,2) * t[2];

    return m;
}

template <typename T>
Matrix<T,4,4>
matrix_inverse_viewtrans (Vector<T,3> const& campos,
    Vector<T,3> const& viewdir, Vector<T,3> const& upvec)
{
    Vector<T,3> z(-viewdir);
    Vector<T,3> x(upvec.cross(z).normalized());
    Vector<T,3> y(z.cross(x));

    Matrix<T,4,4> m;
    m(0,0) = x[0]; m(0,1) = y[0]; m(0,2) = z[0]; m(0,3) = campos[0];
    m(1,0) = x[1]; m(1,1) = y[1]; m(1,2) = z[1]; m(1,3) = campos[1];
    m(2,0) = x[2]; m(2,1) = y[2]; m(2,2) = z[2]; m(2,3) = campos[2];
    m(3,0) = 0.0f; m(3,1) = 0.0f; m(3,2) = 0.0f; m(3,3) = 1.0f;

    return m;
}

template <typename T>
Matrix<T,4,4>
matrix_invert_trans (Matrix<T,4,4> const& mat)
{
    Matrix<T,4,4> ret(0.0f);
    /* Transpose rotation. */
    ret[0] = mat[0]; ret[1] = mat[4]; ret[2] = mat[8];
    ret[4] = mat[1]; ret[5] = mat[5]; ret[6] = mat[9];
    ret[8] = mat[2]; ret[9] = mat[6]; ret[10] = mat[10];
    /* Invert translation. */
    ret[3]  = -(ret[0] * mat[3] + ret[1] * mat[7] + ret[2] * mat[11]);
    ret[7]  = -(ret[4] * mat[3] + ret[5] * mat[7] + ret[6] * mat[11]);
    ret[11] = -(ret[8] * mat[3] + ret[9] * mat[7] + ret[10] * mat[11]);
    ret[15] = 1.0f;

    return ret;
}

template <typename T, int N>
Matrix<T,N,N>&
matrix_set_identity (Matrix<T,N,N>* mat)
{
    mat->fill(T(0));
    for (int i = 0; i < N * N; i += N + 1)
        (*mat)[i] = T(1);
    return *mat;
}

template <typename T>
T*
matrix_set_identity (T* mat, int n)
{
    int len = n * n;
    std::fill(mat, mat + len, T(0));
    for (int i = 0; i < len; i += n + 1)
        mat[i] = T(1);
    return mat;
}

template <typename T, int N>
bool
matrix_is_identity (Matrix<T,N,N> const& mat)
{
    for (int y = 0, i = 0; y < N; ++y)
        for (int x = 0; x < N; ++x, ++i)
            if ((x == y && mat[i] != T(1)) || (x != y && mat[i] != T(0)))
                return false;
    return true;
}

template <typename T, int N>
Matrix<T,N,N>
matrix_from_diagonal (math::Vector<T,N> const& v)
{
    Matrix<T,N,N> mat;
    std::fill(*mat, *mat + N*N, T(0));
    for (int i = 0, j = 0; i < N*N; i += N+1, j += 1)
        mat[i] = v[j];
    return mat;
}

template <typename T, int N>
Matrix<T,N,N>&
matrix_set_diagonal (Matrix<T,N,N>& mat, T const* diag)
{
    for (int i = 0, j = 0; i < N*N; i += N+1, j += 1)
        mat[i] = diag[j];
    return mat;
}

template <typename T, int N>
Vector<T,N>
matrix_get_diagonal (Matrix<T,N,N> const& mat)
{
    Vector<T,N> diag;
    for (int i = 0, j = 0; i < N*N; i += N+1, j += 1)
        diag[j] = mat[i];
    return diag;
}

template <typename T>
inline T
matrix_determinant (Matrix<T,1,1> const& mat)
{
    return mat[0];
}

template <typename T>
inline T
matrix_determinant (Matrix<T,2,2> const& mat)
{
    return mat[0] * mat[3] - mat[1] * mat [2];
}

template <typename T>
inline T
matrix_determinant (Matrix<T,3,3> const& m)
{
    return m[0] * m[4] * m[8] + m[1] * m[5] * m[6] + m[2] * m[3] * m[7]
         - m[2] * m[4] * m[6] - m[1] * m[3] * m[8] - m[0] * m[5] * m[7];
}

template <typename T>
T
matrix_determinant (Matrix<T,4,4> const& m)
{
    return m[0] * (m[5] * m[10] * m[15] - m[5] * m[11] * m[14]
        - m[9] * m[6] * m[15] + m[9] * m[7] * m[14]
        + m[13] * m[6] * m[11] - m[13] * m[7] * m[10])
        //
        + m[1] * (-m[4] * m[10] * m[15] + m[4] * m[11] * m[14]
        + m[8] * m[6] * m[15] - m[8] * m[7] * m[14]
        - m[12] * m[6] * m[11] + m[12] * m[7] * m[10])
        //
        + m[2] * (m[4] * m[9] * m[15] - m[4] * m[11] * m[13]
        - m[8] * m[5] * m[15] + m[8] * m[7] * m[13]
        + m[12] * m[5] * m[11] - m[12] * m[7] * m[9])
        //
        + m[3] * (-m[4] * m[9] * m[14] + m[4] * m[10] * m[13]
        + m[8] * m[5] * m[14] - m[8] * m[6] * m[13]
        - m[12] * m[5] * m[10] + m[12] * m[6] * m[9]);
}

template <typename T>
inline Matrix<T,1,1>
matrix_inverse (Matrix<T,1,1> const& mat)
{
    return matrix_inverse(mat, matrix_determinant(mat));
}

template <typename T>
inline Matrix<T,2,2>
matrix_inverse (Matrix<T,2,2> const& mat)
{
    return matrix_inverse(mat, matrix_determinant(mat));
}

template <typename T>
inline Matrix<T,3,3>
matrix_inverse (Matrix<T,3,3> const& m)
{
    return matrix_inverse(m, matrix_determinant(m));
}

template <typename T>
Matrix<T,1,1>
matrix_inverse (Matrix<T,1,1> const& /*mat*/, T const& det)
{
    Matrix<T,1,1> ret(T(1));
    return ret / det;
}

template <typename T>
Matrix<T,2,2>
matrix_inverse (Matrix<T,2,2> const& mat, T const& det)
{
    Matrix<T,2,2> ret;
    ret[0] =  mat[3]; ret[1] = -mat[1];
    ret[2] = -mat[2]; ret[3] =  mat[0];
    return ret / det;
}

template <typename T>
Matrix<T,3,3>
matrix_inverse (Matrix<T,3,3> const& m, T const& det)
{
    Matrix<T,3,3> ret;
    ret[0] = m[4] * m[8] - m[5] * m[7];
    ret[1] = m[2] * m[7] - m[1] * m[8];
    ret[2] = m[1] * m[5] - m[2] * m[4];
    ret[3] = m[5] * m[6] - m[3] * m[8];
    ret[4] = m[0] * m[8] - m[2] * m[6];
    ret[5] = m[2] * m[3] - m[0] * m[5];
    ret[6] = m[3] * m[7] - m[4] * m[6];
    ret[7] = m[1] * m[6] - m[0] * m[7];
    ret[8] = m[0] * m[4] - m[1] * m[3];
    return ret / det;
}

template <typename T>
Matrix<T,4,4>
matrix_inverse (Matrix<T,4,4> const& m)
{
    Matrix<T,4,4> ret;

    ret[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15]
        + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    ret[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15]
        - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    ret[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15]
        + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    ret[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11]
        - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];

    ret[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15]
        - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    ret[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15]
        + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    ret[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15]
        - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    ret[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11]
        + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];

    ret[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15]
        + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    ret[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15]
        - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    ret[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15]
        + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    ret[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11]
        - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];

    ret[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14]
        - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    ret[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14]
        + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    ret[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14]
        - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    ret[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10]
        + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

    T det = m[0] * ret[0] + m[1] * ret[4] + m[2] * ret[8] + m[3] * ret[12];
    return ret / det;
}

MATH_NAMESPACE_END

#endif /* MATH_MATRIXTOOLS_HEADER */
