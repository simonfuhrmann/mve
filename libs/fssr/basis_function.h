/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef FSSR_BASIS_FUNCTION_HEADER
#define FSSR_BASIS_FUNCTION_HEADER

#include "math/vector.h"
#include "math/matrix.h"
#include "fssr/defines.h"
#include "fssr/sample.h"

FSSR_NAMESPACE_BEGIN

/* ---------------------- Gaussian functions ---------------------- */

/** The Gaussian function in 3D. */
template <typename T>
T
gaussian (T const& sigma, math::Vector<T, 3> const& x);

/** The normalized Gaussian function in 3D. */
template <typename T>
T
gaussian_normalized (T const& sigma, math::Vector<T, 3> const& x);

/* ---------------------- FSSR basis function --------------------- */

/**
 * Evaluates the FSSR basis function and directional derivatives (optional).
 * The basis function is a Gaussian derivative in the direction of the normal
 * and a regular Gaussian orthogonal to the normal. Here, the normal direction
 * is defined to be the positive x-axis, thus 'pos' must be translated and
 * rotated into the sample's LCS. The function takes positive values in front
 * and negative values behind the sample.
 */
template <typename T>
T
fssr_basis (T const& scale, math::Vector<T, 3> const& pos,
    math::Vector<T, 3>* deriv = nullptr);

/* --------------------- FSSR weight function --------------------- */

/**
 * Evaluates the FSSR weight function and directional derivatives (optional).
 * The weight function is composed of polynomials up to the forth degree.
 * The function is one in the center, and falls of to zero at 3 * scale.
 * Similar to fssr_basis(), it expects 'pos' to be in the sample's LCS.
 */
template <typename T>
T
fssr_weight (T const& scale, math::Vector<T, 3> const& pos,
    math::Vector<T, 3>* deriv = nullptr);

/* -------------------------- Helper functions --------------------------- */

/**
 * Rotates the given point in the LCS of the sample, evaluates the basis
 * and weight functions and their derivatives, and rotates the derivatives
 * back to the global coordinate system.
 */
void
evaluate (math::Vec3f const& pos, Sample const& sample,
    double* value, double* weight,
    math::Vector<double, 3>* value_deriv,
    math::Vector<double, 3>* weight_deriv);

/** Transforms 'pos' according to the samples position and normal. */
math::Vec3f
transform_position (math::Vec3f const& pos, Sample const& sample);

/** Generates a rotation matrix that transforms in the FSSR LCS. */
void
rotation_from_normal (math::Vec3f const& normal, math::Matrix3f* rot);

/** Generates a rotation matrix that transforms in the FSSR LCS. */
void
rotation_from_normal (math::Vec2f const& normal, math::Matrix2f* rot);

FSSR_NAMESPACE_END

/*
 * ========================= Implementation ============================
 */

FSSR_NAMESPACE_BEGIN

template <typename T>
inline T
gaussian (T const& sigma, math::Vector<T, 3> const& x)
{
    return std::exp(-x.dot(x) / (T(2) * MATH_POW2(sigma)));
}

template <typename T>
inline T
gaussian_normalized (T const& sigma, math::Vector<T, 3> const& x)
{
    return gaussian(sigma, x) / (sigma * MATH_SQRT_2PI);
}

/* -------------------------------------------------------------------- */

template <typename T>
inline T
fssr_basis (T const& scale, math::Vector<T, 3> const& pos,
    math::Vector<T, 3>* deriv)
{
    double const gaussian_value = gaussian(scale, pos);
    double const value_norm = T(2) * MATH_PI * MATH_POW4(scale);

    if (deriv != nullptr)
    {
        double const deriv_norm = value_norm * T(2) * MATH_POW2(scale);
        (*deriv)[0] = T(2) * (MATH_POW2(scale) - MATH_POW2(pos[0]))
            * gaussian_value / deriv_norm;
        (*deriv)[1] = (-T(2) * pos[0] * pos[1]) * gaussian_value / deriv_norm;
        (*deriv)[2] = (-T(2) * pos[0] * pos[2]) * gaussian_value / deriv_norm;
    }

    return pos[0] * gaussian_value / value_norm;
}

/* -------------------------------------------------------------------- */

#if FSSR_NEW_WEIGHT_FUNCTION

template <typename T>
T
fssr_weight (T const& scale, math::Vector<T, 3> const& pos,
    math::Vector<T, 3>* deriv)
{
    T const square_radius = pos.square_norm() / MATH_POW2(scale);
    if (square_radius >= T(9))
    {
        if (deriv != nullptr)
            deriv->fill(T(0));
        return T(0);
    }

    if (deriv != nullptr)
    {
        T const deriv_factor = -T(4) / T(3)
            + T(48) / T(54) * std::sqrt(square_radius)
            - T(4) / T(27) * square_radius;

        (*deriv)[0] = deriv_factor * pos[0] / scale;
        (*deriv)[1] = deriv_factor * pos[1] / scale;
        (*deriv)[2] = deriv_factor * pos[2] / scale;
    }

    /*
     * w(r > 0) = 1.0 - 2.0/3.0 * r**2 + 8.0/27.0 * r**3 - 1.0/27.0 * r**4
     * w(r < 0) = 1.0 - 2.0/3.0 * r**2 - 8.0/27.0 * r**3 - 1.0/27.0 * r**4
     */
    return T(1) - T(2) / T(3) * square_radius
        + T(8) / T(27) * std::pow(square_radius, T(1.5))
        - T(1) / T(27) * MATH_POW2(square_radius);
}

#else

template <typename T>
T
fssr_weight (T const& scale, math::Vector<T, 3> const& pos,
    math::Vector<T, 3>* deriv)
{
    T const x = pos[0] / scale;
    T const y = pos[1] / scale;
    T const z = pos[2] / scale;
    T const square_radius = MATH_POW2(y) + MATH_POW2(z);

    /*
     * wx(x > 0) = 1.0 - 1.0/3.0 * r**2 + 2.0/27.0 * r**3
     * wx(x < 0) = 1.0 + 2.0/3.0 * r + 1.0/9.0 * r**2
     */
    T weight_x = T(0);
    if (x > T(-3) && x < T(0))
    {
        weight_x = T(1)
            + T(2) / T(3) * x
            + T(1) / T(9) * MATH_POW2(x);
    }
    else if (x >= T(0) && x < T(3))
    {
        weight_x = T(1)
            - T(1) / T(3) * MATH_POW2(x)
            + T(2) / T(27) * MATH_POW3(x);
    }

    /*
     * wyz(r) = 1.0 - 1.0/3.0 r**2 / s**2 + 2.0/27.0 r**3 / s**3
     */
    T weight_yz = T(0);
    if (square_radius < T(9))
    {
        weight_yz = T(1)
            - T(1) / T(3) * square_radius
            + T(2) / T(27) * std::pow(square_radius, T(1.5));
    }

    if (deriv != nullptr)
    {
        /*
         * wx'(x < 0) = 2.0/9.0 * x / s^2 + 2.0/3.0 / s
         * wx'(x > 0) = 6.0/27.0 * x^2 / s^3 - 2.0/3.0 * x / s^2
         */
        T deriv_x = T(0);
        if (x > T(-3) && x <= T(0))
        {
            deriv_x = (T(2) / T(3) + T(2) / T(9) * x) / scale;
        }
        else if (x > T(0) && x < T(3))
        {
            deriv_x = (-T(2) / T(3) * x + T(6) / T(27) * MATH_POW2(x)) / scale;
        }

        /*
         * w(y,z) = 2/27 (y^2 + z^2)^(3/2) / s^3 - 1/3 (y^2 + z^2) / s^2 + 1
         * d/dy w'(y,z) = y/s * (12 / (54 s^2) * (y^2 + z^2)^(1/2) - 2 / (3s))
         * d/dz w'(y,z) = z/s * (12 / (54 s^2) * (y^2 + z^2)^(1/2) - 2 / (3s))
         */
        T deriv_y = T(0);
        T deriv_z = T(0);
        if (square_radius < T(9))
        {
            T const factor = (T(12) / (T(54) * scale)
                * std::sqrt(MATH_POW2(pos[1]) + MATH_POW2(pos[2]))
                - T(2) / T(3)) / MATH_POW2(scale);
            deriv_y = factor * pos[1];
            deriv_z = factor * pos[2];
        }

        (*deriv)[0] = deriv_x * weight_yz;
        (*deriv)[1] = deriv_y * weight_x;
        (*deriv)[2] = deriv_z * weight_x;
    }

    return weight_x * weight_yz;
}

#endif

/* -------------------------------------------------------------------- */

inline math::Vec3f
transform_position (math::Vec3f const& pos, Sample const& sample)
{
    math::Matrix3f rot;
    rotation_from_normal(sample.normal, &rot);
    return rot * (pos - sample.pos);
}

/* -------------------------------------------------------------------- */

#if 0  /* Following is dead but potentially still useful code. */
/**
 * Implementation of a linear ramp signed distance function. Expects the
 * sample and the NON-TRANSFORMED voxel position. The SDF is computed
 * using the dot product sdf = < pos - sample.pos | sample.normal >.
 */
template <typename T>
inline T
linear_ramp (Sample const& sample, math::Vector<T, 3> const& pos)
{
    return (pos - sample.pos).dot(sample.normal);
}

/**
 * Radially symmetric weighting function in [-3, 3] from the MPU paper (3)
 * which uses a quadratic B-spline.
 */
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

/**
 * Evaluates the weighting function scaled according to sample scale.
 * Similar to the basis functions, it expects 'pos' to be translated
 * into the sample's LCS but is rotation invariant.
 */
template <typename T>
inline T
weighting_function_mpu (T const& sample_scale, math::Vector<T, 3> const& pos)
{
    return weighting_function_mpu(pos.norm() / sample_scale);
}
#endif

FSSR_NAMESPACE_END

#endif // FSSR_BASIS_FUNCTION_HEADER
