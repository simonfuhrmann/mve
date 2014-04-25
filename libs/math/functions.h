#ifndef MATH_FUNCTIONS_HEADER
#define MATH_FUNCTIONS_HEADER

#include <cmath>

#include "math/defines.h"
#include "math/vector.h"
#include "math/matrix.h"

MATH_NAMESPACE_BEGIN

/* ------------------------ Gaussian functions -------------------- */

/**
 * Gaussian function g(x) = exp( -1/2 * (x/sigma)^2 ).
 * Gaussian with bell height y=1, bell center x=0 and bell "width" sigma.
 * Useful for at least float and double types.
 */
template <typename T>
inline T
gaussian (T const& x, T const& sigma)
{
    return std::exp(-((x * x) / (T(2) * sigma * sigma)));
}

/**
 * Gaussian function that expects x to be squared.
 * g(x) = exp( -1/2 * xx / sigma^2 ).
 * Gaussian with bell height y=1, bell center x=0 and bell "width" sigma.
 * Useful for at least float and double types.
 */
template <typename T>
inline T
gaussian_xx (T const& xx, T const& sigma)
{
    return std::exp(-(xx / (T(2) * sigma * sigma)));
}

/**
 * Gaussian function in 2D.
 */
template <typename T>
inline T
gaussian_2d (T const& x, T const& y, T const& sigma_x, T const& sigma_y)
{
    return std::exp(-(x * x) / (T(2) * sigma_x * sigma_x)
        - (y * y) / (T(2) * sigma_y * sigma_y));
}

/**
 * Gaussian function in N dimensions with bell height 1
 * and center (0,...,0). The covariance matrix needs to
 * be inverted beforehand.
 */
template <typename T, int N>
inline T
gaussian_nd (Vector<T,N> const& x, Matrix<T,N,N> const& covariance_inv)
{
    return std::exp(-x.dot(covariance_inv * x) / T(2));
}

/* ------------------------- Interpolation ------------------------ */

/** Generic interpolation (weighting) of a single value. */
template <typename T>
inline T
interpolate (T const& v1, float w1)
{
    return v1 * w1;
}

/** Specific interpolation (weighting) of a value for unsigned char. */
template <>
inline unsigned char
interpolate (unsigned char const& v1, float w1)
{
    return (unsigned char)((float)v1 * w1 + 0.5f);
}

/** Generic interpolation between two values. */
template <typename T>
inline T
interpolate (T const& v1, T const& v2, float w1, float w2)
{
    return v1 * w1 + v2 * w2;
}

/** Specific interpolation between two unsigned char values. */
template <>
inline unsigned char
interpolate (unsigned char const& v1, unsigned char const& v2,
    float w1, float w2)
{
    return (unsigned char)((float)v1 * w1 + (float)v2 * w2 + 0.5f);
}

/** Generic interpolation between three values. */
template <typename T>
inline T
interpolate (T const& v1, T const& v2, T const& v3,
    float w1, float w2, float w3)
{
    return v1 * w1 + v2 * w2 + v3 * w3;
}

/** Specific interpolation between three unsigned char values. */
template <>
inline unsigned char
interpolate (unsigned char const& v1, unsigned char const& v2,
    unsigned char const& v3, float w1, float w2, float w3)
{
    return (unsigned char)((float)v1 * w1 + (float)v2 * w2
        + (float)v3 * w3 + 0.5f);
}

/** Generic interpolation between four values. */
template <typename T>
inline T
interpolate (T const& v1, T const& v2, T const& v3, T const& v4,
    float w1, float w2, float w3, float w4)
{
    return v1 * w1 + v2 * w2 + v3 * w3 + v4 * w4;
}

/** Specific interpolation between four unsigned char values. */
template <>
inline unsigned char
interpolate (unsigned char const& v1, unsigned char const& v2,
    unsigned char const& v3, unsigned char const& v4,
    float w1, float w2, float w3, float w4)
{
    return (unsigned char)((float)v1 * w1 + (float)v2 * w2
        + (float)v3 * w3 + (float)v4 * w4 + 0.5f);
}

/* --------------------------- Special functions ------------------ */

/**
 * Sinc function.
 */
template <typename T>
inline T
sinc (T const& x)
{
    return (x == T(0) ? T(1) : std::sin(x) / x);
}

/* ------------------------------ Misc ---------------------------- */

/**
 * Returns value 'v' clamped to the interval specified by 'min' and 'max'.
 */
template <typename T>
T const&
clamp (T const& v, T const& min = T(0), T const& max = T(1))
{
    return (v < min ? min : (v > max ? max : v));
}

template <typename T>
T
bound_mirror (T const& v, T const& min, T const& max)
{
    return (v < min ? min - v : (v > max ? max + max - v : v));
}

/** Returns the minimum value of three arguments. */
template <typename T>
T const&
min (T const& a, T const& b, T const& c)
{
    return std::min(a, std::min(b, c));
}

/** Returns the maximum value of three arguments. */
template <typename T>
T const&
max (T const& a, T const& b, T const& c)
{
    return std::max(a, std::max(b, c));
}

/** Takes base to the integer power of 'exp'. */
template <typename T>
T
fastpow (T const& base, unsigned int exp)
{
    T ret = (exp == 0 ? T(1) : base);
    unsigned int const exp2 = exp / 2;
    unsigned int i = 1;
    for (; i <= exp2; i = i * 2)
        ret = ret * ret;
    for (; i < exp; ++i)
        ret = ret * base;
    return ret;
}

inline int
to_gray_code (int bin)
{
    return bin ^ (bin >> 1);
}

inline int
from_gray_code (int gc)
{
    int b = 0, ret = 0;
    for (int i = 31; i >= 0; --i)
    {
        b = (b + ((gc & (1 << i)) >> i)) % 2;
        ret |= b << i;
    }
    return ret;
}

MATH_NAMESPACE_END

#endif // MATH_FUNCTIONS_HEADER
