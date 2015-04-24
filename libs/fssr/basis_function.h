/*
 * This file is part of the Floating Scale Surface Reconstruction software.
 * Written by Simon Fuhrmann.
 *
 * GNUplot basis functions:
 * nw(r) = r < 0 ? 1.0 - 2.0/3.0 * r**2 - 8.0/27.0 * r**3 - 1.0/27.0 * r**4 : 1.0 - 2.0/3.0 * r**2 + 8.0/27.0 * r**3 - 1.0/27.0 * r**4
 * ow(r) = r < 0 ? 1.0 + 2.0/3.0 * r + 1.0/9.0 * r**2 : 1 - 1.0/3.0 * r**2 + 2.0/27.0 * r**3
 */

#ifndef FSSR_BASIS_FUNCTION_HEADER
#define FSSR_BASIS_FUNCTION_HEADER

#include "math/vector.h"
#include "math/matrix.h"
#include "fssr/defines.h"
#include "fssr/sample.h"

/* Use new weighting function with continuous derivative. */
#define FSSR_NEW_WEIGHT 0

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
 * Evaluates the FSSR basis function which is a Gaussian derivative in the
 * direction of the normal and a regular Gaussian orthogonal to the normal.
 * Here, the normal direction is defined to be the positive x-axis, thus 'pos'
 * must be translated and rotated into the sample's LCS. The function
 * takes positive values in front and negative values behind the sample.
 */
template <typename T>
T
fssr_basis (T const& scale, math::Vector<T, 3> const& pos);

/**
 * Evaluates the FSSR basis function and its directional derivatives.
 * Similar to fssr_basis(), it expects 'pos' to be in the sample's LCS.
 */
template <typename T>
math::Vector<T, 4>
fssr_basis_deriv (T const& scale, math::Vector<T, 3> const& pos);

/* --------------------- FSSR weight function --------------------- */

/**
 * Evaluates the FSSR basis function which is composed of polynomials
 * and second and third degree. The function is one in the center, falls
 * of quickly behind the surface (negative x) and less quickly in front of
 * the surface (positive x), and is radially symmetric in y and z direction.
 * Similar to fssr_basis(), it expects 'pos' to be in the sample's LCS.
 */
template <typename T>
T
fssr_weight (T const& scale, math::Vector<T, 3> const& pos);

/**
 * Evaluates the FSSR weighting function and its directional derivatives.
 * Similar to fssr_basis(), it expects 'pos' to be in the sample's LCS.
 */
template <typename T>
math::Vector<T, 4>
fssr_weight_deriv (T const& scale, math::Vector<T, 3> const& pos);

/* -------------------------- Helper functions --------------------------- */

/**
 * Rotates the given point in the LCS of the sample, wvaluates the basis
 * and weighting functions and their derivatives, and rotates the derivatives
 * back to the global coordinate system.
 */
void
evaluate (math::Vec3f const& pos, Sample const& sample,
    math::Vec4d* value, math::Vec4d* weight);

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
fssr_basis (T const& scale, math::Vector<T, 3> const& pos)
{
    return pos[0] * gaussian(scale, pos) / (MATH_POW4(scale) * 2.0 * MATH_PI);
}

template <typename T>
inline math::Vector<T, 4>
fssr_basis_deriv (T const& scale, math::Vector<T, 3> const& pos)
{
    double const gaussian_value = gaussian(scale, pos);
    double const norm_value = T(2) * MATH_PI * MATH_POW4(scale);
    double const norm_deriv = norm_value * T(2) * MATH_POW2(scale);

    math::Vector<T, 4> ret;
    ret[0] = pos[0] * gaussian_value / norm_value;
    ret[1] = (T(2) * MATH_POW2(scale) - T(2) * MATH_POW2(pos[0]))
        * gaussian_value / norm_deriv;
    ret[2] = (-T(2) * pos[0] * pos[1]) * gaussian_value / norm_deriv;
    ret[3] = (-T(2) * pos[0] * pos[2]) * gaussian_value / norm_deriv;
    return ret;
}

/* -------------------------------------------------------------------- */

#if FSSR_NEW_WEIGHT

template <typename T>
T
fssr_weight (T const& scale, math::Vector<T, 3> const& pos)
{
    T const square_radius = pos_tmp.square_norm() / MATH_POW2(scale);
    return T(1) - T(2) / T(3) * square_radius
        + T(8) / T(27) * std::pow(square_radius, T(1.5))
        - T(1) / T(27) * MATH_POW2(square_radius);
}

template <typename T>
math::Vector<T, 4>
fssr_weight_deriv (T const& scale, math::Vector<T, 3> const& pos)
{
    T const square_radius = pos.square_norm() / MATH_POW2(scale);
    T const deriv_factor = -T(4) / T(3)
        + T(48) / T(54) * std::sqrt(square_radius)
        - T(4) / T(27) * square_radius;

    math::Vector<T, 4> ret;
    ret[0] = T(1) - T(2) / T(3) * square_radius
        + T(8) / T(27) * std::pow(square_radius, T(1.5))
        - T(1) / T(27) * MATH_POW2(square_radius);
    ret[1] = pos[0] / scale * deriv_factor;
    ret[2] = pos[1] / scale * deriv_factor;
    ret[3] = pos[2] / scale * deriv_factor;
    return ret;
}

#else

template <typename T>
T
fssr_weight_x (T const& scale, math::Vector<T, 3> const& pos)
{
    T const x = pos[0] / scale;
    if (x > T(-3) && x < T(0))
    {
        // w(x) = 1/9 x^2 / s^2 + 2/3 x / s + 1
        T const a0 = T(1);
        T const a1 = T(2) / T(3);
        T const a2 = T(1) / T(9);
        return a0 + a1 * x + a2 * MATH_POW2(x);
    }
    if (x >= T(0) && x < T(3))
    {
        // 2/27 x^3 / s^3 - 1/3 x^2 / s^2 + 1
        T const a0 = T(1);
        T const a2 = -T(1) / T(3);
        T const a3 = T(2) / T(27);
        return a0 + a2 * MATH_POW2(x) + a3 * MATH_POW3(x);
    }
    return T(0);
}

template <typename T>
T
fssr_weight_yz (T const& scale, math::Vector<T, 3> const& pos)
{
    T const y = pos[1] / scale;
    T const z = pos[2] / scale;
    if (y * y + z * z < T(9))
    {
        // 2/27 r^3 / s^3 - 1/3 r^2 / s^2 + 1
        T const a0 = T(1);
        T const a2 = -T(1) / T(3);
        T const a3 = T(2) / T(27);
        return a0 + a2 * (MATH_POW2(y) + MATH_POW2(z))
            + a3 * std::pow(MATH_POW2(y) + MATH_POW2(z), T(1.5));
    }
    return T(0);
}

template <typename T>
T
fssr_weight (T const& scale, math::Vector<T, 3> const& pos)
{
    T const weight_x = fssr_weight_x(scale, pos);
    T const weight_yz = fssr_weight_yz(scale, pos);
    return weight_x * weight_yz;
}

template <typename T>
math::Vector<T, 4>
fssr_weight_deriv (T const& scale, math::Vector<T, 3> const& pos)
{
    T const weight_x = fssr_weight_x(scale, pos);
    T const weight_yz = fssr_weight_yz(scale, pos);
    T const x = pos[0] / scale;
    T const y = pos[1] / scale;
    T const z = pos[2] / scale;

    T deriv_x = T(0);
    if (x > T(-3) && x <= T(0))
    {
        // w'(x) = 2/9 x / s^2 + 2/3 / s
        T const a0 = T(2) / T(3);
        T const a1 = T(2) / T(9);
        deriv_x = (a0 + a1 * x) / scale;
    }
    else if (x > T(0) && x < T(3))
    {
        // w'(x) = 6/27 x^2 / s^3 - 2/3 x / s^2
        T const a1 = -T(2) / T(3);
        T const a2 = T(6) / T(27);
        deriv_x = (a1 * x + a2 * MATH_POW2(x)) / scale;
    }

    /*
     * w(y, z) = 2/27 (y^2 + z^2)^(3/2) / s^3 - 1/3 (y^2 + z^2) / s^2 + 1
     * d/dy w'(y, z) = y/s * (12 / (54 s^2) * (y^2 + z^2)^(1/2) - 2 / (3s))
     * d/dz w'(y, z) = z/s * (12 / (54 s^2) * (y^2 + z^2)^(1/2) - 2 / (3s))
     */
    T deriv_y = T(0);
    T deriv_z = T(0);
    if (y * y + z * z < T(9))
    {
        T const factor = (T(12) / (T(54) * scale)
            * std::sqrt(MATH_POW2(pos[1]) + MATH_POW2(pos[2]))
            - T(2) / T(3)) / MATH_POW2(scale);
        deriv_y = factor * pos[1];
        deriv_z = factor * pos[2];
    }

    math::Vector<T, 4> ret;
    ret[0] = weight_x * weight_yz;
    ret[1] = deriv_x * weight_yz;
    ret[2] = weight_x * deriv_y;
    ret[3] = weight_x * deriv_z;
    return ret;
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
