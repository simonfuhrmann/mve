/*
 * This file is part of the Floating Scale Surface Reconstruction software.
 * Written by Simon Fuhrmann.
 */

#ifndef FSSR_BASIS_FUNCTION_HEADER
#define FSSR_BASIS_FUNCTION_HEADER

#include "math/vector.h"
#include "math/matrix.h"
#include "fssr/defines.h"
#include "fssr/sample.h"

FSSR_NAMESPACE_BEGIN

/* ------------------------- Gaussian funcitons -------------------------- */

/**
 * The Gaussian function in 3D. This basis function expects 'pos' to be
 * translated into the sample's LCS but is rotation invariant.
 */
template <typename T>
T
gaussian (T const& sigma, math::Vector<T, 3> const& pos);

/**
 * The normalized Gaussian in 3D. This basis function expects 'pos' to be
 * translated into the sample's LCS but is rotation invariant.
 */
template <typename T>
T
gaussian_normalized (T const& sigma, math::Vector<T, 3> const& pos);

/**
 * Evaluates the FSSR basis function which is a Gaussian derivative in the
 * direction of the normal and a regular Gaussian orthogonal to the normal.
 * Here, the normal direction is defined to be the positive x-axis, thus 'pos'
 * must be translated and rotated into the sample's LCS. The function
 * takes positive values in front and negative values behind the sample.
 */
template <typename T>
T
gaussian_fssr (T const& sigma, math::Vector<T, 3> const& pos);

/* ----------------------------- Linear Ramp ----------------------------- */

/**
 * Implementation of a linear ramp signed distance function. Expects the
 * sample and the NON-TRANSFORMED voxel position. The SDF is computed
 * using the dot product sdf = < pos - sample.pos | sample.normal >.
 */
template <typename T>
T
linear_ramp (Sample const& sample, math::Vector<T, 3> const& pos);

/* ------------------------- Weighting function -------------------------- */

/**
 * Weighting function in x-direction in [-3, 3] using polynomials.
 * The function is 1 in the center, falls of quicky behind the surface
 * (negative x) and less quickly in front of the surface (positive x).
 */
template <typename T>
T
weighting_function_x (T const& x);

/**
 * Weighting function in y and z direction in [-3, 3]^2.
 */
template <typename T>
T
weighting_function_yz (T const& y, T const& z);

/**
 * Evaluates the weighting function scaled according to sample scale.
 * Similar to the basis functions, it expects 'pos' to be translated and
 * rotated into the LCS of the sample.
 */
template <typename T>
inline T
weighting_function (T const& sample_scale, math::Vector<T, 3> const& pos);

/* ----------------------- MPU Weighting function ------------------------ */

/**
 * Radially symmetric weighting function in [-3, 3] from the MPU paper (3)
 * which uses a quadratic B-spline.
 */
template <typename T>
T
weighting_function_mpu (T const& x);

/**
 * Evaluates the weighting function scaled according to sample scale.
 * Similar to the basis functions, it expects 'pos' to be translated
 * into the sample's LCS but is rotation invariant.
 */
template <typename T>
inline T
weighting_function_mpu (T const& sample_scale, math::Vector<T, 3> const& pos);

/* -------------------------- Helper functions --------------------------- */

/**
 * Transforms the given position according to the samples position and normal.
 */
math::Vec3f
transform_position (math::Vec3f const& pos, Sample const& sample);

void
rotation_from_normal (math::Vec3f const& normal, math::Matrix3f* rot);

void
rotation_from_normal (math::Vec2f const& normal, math::Matrix2f* rot);

FSSR_NAMESPACE_END

/*
 * ========================= Implementation ============================
 */

FSSR_NAMESPACE_BEGIN

template <typename T>
inline T
gaussian (T const& sigma, math::Vector<T, 3> const& pos)
{
    return std::exp(-pos.dot(pos) / (T(2) * MATH_POW2(sigma)));
}

template <typename T>
inline T
gaussian_normalized (T const& sigma, math::Vector<T, 3> const& pos)
{
    return gaussian(sigma, pos) / (sigma * MATH_SQRT_2PI);
}

template <typename T>
inline T
gaussian_fssr (T const& sigma, math::Vector<T, 3> const& pos)
{
    return pos[0] * gaussian(sigma, pos) / (MATH_POW4(sigma) * 2.0 * MATH_PI);
}

/* -------------------------------------------------------------------- */

template <typename T>
T
linear_ramp (Sample const& sample, math::Vector<T, 3> const& pos)
{
    return (pos - sample.pos).dot(sample.normal);
}

/* -------------------------------------------------------------------- */

template <typename T>
T
weighting_function_x (T const& x)
{
    if (x <= -T(3) || x >= T(3))
        return T(0);

    if (x > T(0))
    {
        T const a_o = T(2) / T(27);
        T const b_o = -T(1) / T(3);
        T const d_o = T(1);
        T const value = a_o * MATH_POW3(x) + b_o * MATH_POW2(x) + d_o;
        return value;
    }

    T const a_i = T(1) / T(9);
    T const b_i = T(2) / T(3);
    T const c_i = T(1);
    T const value = a_i * MATH_POW2(x) + b_i * x + c_i;
    return value;
}

template <typename T>
T
weighting_function_yz (T const& y, T const& z)
{
    if (y * y + z * z > T(9))
        return T(0);

    T const a_o = T(2) / T(27);
    T const b_o = -T(1) / T(3);
    T const d_o = T(1);
    T const value = a_o * std::pow(MATH_POW2(y) + MATH_POW2(z), T(1.5))
        + b_o * (MATH_POW2(y) + MATH_POW2(z)) + d_o;
    return value;
}

template <typename T>
inline T
weighting_function (T const& sample_scale, math::Vector<T, 3> const& pos)
{
    return weighting_function_x(pos[0] / sample_scale)
        * weighting_function_yz(pos[1] / sample_scale, pos[2] / sample_scale);
}

/* -------------------------------------------------------------------- */

template <typename T>
T
weighting_function_mpu (T const& x)
{
    if (x <= T(-3) || x >= T(3))
        return T(0);

    T xf = (x + T(3)) / T(2);
    if (xf <= T(1))
        return MATH_POW2(xf) / T(2);
    if (xf <= T(2))
        return (T(-2) * MATH_POW2(xf) + T(6) * xf - T(3)) / T(2);
    return MATH_POW2(T(3) - xf) / T(2);
}

template <typename T>
inline T
weighting_function_mpu (T const& sample_scale, math::Vector<T, 3> const& pos)
{
    return weighting_function_mpu(pos.norm() / sample_scale);
}

/* -------------------------------------------------------------------- */

inline math::Vec3f
transform_position (math::Vec3f const& pos, Sample const& sample)
{
    math::Matrix3f rot;
    rotation_from_normal(sample.normal, &rot);
    return rot * (pos - sample.pos);
}

FSSR_NAMESPACE_END

#endif // FSSR_BASIS_FUNCTION_HEADER
