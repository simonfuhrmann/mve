/*
 * Mathematical helpers.
 * Written by Simon Fuhrmann.
 */

#ifndef MATH_ALGO_HEADER
#define MATH_ALGO_HEADER

#include <cmath>
#include <vector>
#include <iterator>
#include <stdexcept>

#include "defines.h"

MATH_NAMESPACE_BEGIN
MATH_ALGO_NAMESPACE_BEGIN

/* ---------------------------- Algorithms ------------------------ */

/**
 * Algorithm that returns the ID (starting from zero at element 'first')
 * of the smallest element in range [first, last[.
 */
template <typename FwdIter>
std::size_t
min_element_id (FwdIter first, FwdIter last)
{
    if (first == last)
        throw std::invalid_argument("Invalid range");

    FwdIter lowest = first;
    std::size_t lowest_id = 0;

    for (std::size_t cnt = 1; ++first != last; ++cnt)
    {
        if (*first < *lowest)
        {
            lowest_id = cnt;
            lowest = first;
        }
    }

    return lowest_id;
}

/**
 * Algorithm that returns the ID (starting from zero at element 'first')
 * of the largest element in range [first, last[.
 */
template <typename FwdIter>
std::size_t
max_element_id (FwdIter first, FwdIter last)
{
    if (first == last)
        throw std::invalid_argument("Invalid range");

    FwdIter largest = first;
    std::size_t largest_id = 0;

    for (std::size_t cnt = 1; ++first != last; ++cnt)
    {
        if (*largest < *first)
        {
            largest_id = cnt;
            largest = first;
        }
    }

    return largest_id;
}

/* ---------------------- Generator functors ---------------------- */

template <typename T>
struct IncrementGenerator
{
    T state;

    IncrementGenerator (T const& init = T(0)) : state(init)
    {}

    T operator() (void)
    { return this->state++; }
};

/* ------------------- Misc: predicates, iterators, ... ----------- */

/** Squared sum accumulator. */
template <typename T>
inline T
accum_squared_sum (T const& init, T const& next)
{
    return init + next * next;
}

/** Absolute sum accumulator. */
template <typename T>
inline T
accum_absolute_sum (T const& init, T const& next)
{
    return init + std::abs(next);
}

/** Epsilon comparator predicate. */
template <typename T>
struct predicate_epsilon_equal
{
    T eps;
    predicate_epsilon_equal (T const& eps) : eps(eps) {}
    bool operator() (T const& v1, T const& v2)
    { return (v1 - eps <= v2 && v2 <= v1 + eps); }
};

/** Iterator that advances 'S' elements of type T. */
template <typename T, int S>
struct InterleavedIter : public std::iterator<std::input_iterator_tag, T>
{
    T const* pos;
    InterleavedIter (T const* pos) : pos(pos) {}
    InterleavedIter& operator++ (void) { pos += S; return *this; }
    T const& operator* (void) const { return *pos; }
    bool operator!= (InterleavedIter<T,S> const& other) const
    { return pos != other.pos; }
};

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

/* ------------------------- Gaussion function -------------------- */

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
 * Gaussian function that expects x to squared.
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

/* --------------------------- Vector tools ----------------------- */

/**
 * Erases all elements from 'vec' that are marked with 'true' in 'dlist'.
 * The remaining elements are kept in order but relocated to an earlier
 * position in the vector. Each element moved over a distance that is
 * equal to the amount of deleted vertices earlier in the vector.
 */
template <typename T>
void
vector_clean (std::vector<T>& vec, std::vector<bool> const& dlist)
{
    typename std::vector<T>::iterator vr = vec.begin();
    typename std::vector<T>::iterator vw = vec.begin();
    typename std::vector<bool>::const_iterator dr = dlist.begin();

    while (vr != vec.end() && dr != dlist.end())
    {
        if (*dr++)
        {
            vr++;
            continue;
        }
        if (vw != vr)
            *vw = *vr;
        vw++;
        vr++;
    }

    vec.erase(vw, vec.end());
}

/* ------------------------------ Misc ---------------------------- */

/**
 * Returns the kernel region (x1,y1) to (x2,y2) for a kernel of size ks
 * for image of size (width, height) and for center pixel (cx,cy).
 * The kernel size ks is the half-size, i.e. 2*ks+1 is full kernel size.
 * Values x2 and y2 are inclusive, i.e. belong to the kernel.
 */
template <typename T>
inline void
kernel_region (T const& cx, T const& cy, T const& ks,
    T const& width, T const& height, T& x1, T& x2, T& y1, T& y2)
{
    x1 = (cx > ks ? cx - ks : T(0));
    x2 = (cx + ks > width - T(1) ? width - T(1) : cx + ks);
    y1 = (cy > ks ? cy - ks : T(0));
    y2 = (cy + ks > height - T(1) ? height - T(1) : cy + ks);
}

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

/** Takes base to the integral power of 'power'. */
template <typename T>
T
fastpow (T const& base, unsigned int exp)
{
    T ret(1);
    for (unsigned int i = 0; i < exp; ++i)
        ret *= base;
    return ret;
}

template <typename T>
inline T
round (T const& x)
{
   return x > T(0) ? std::floor(x + T(0.5)) : std::ceil(x - T(0.5));
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

/* ------------------------ for-each functors --------------------- */

/** for-each functor: multiplies operand with constant factor. */
template <typename T>
struct foreach_multiply_with_const
{
    T value;
    foreach_multiply_with_const (T const& value) : value(value) {}
    void operator() (T& val) { val *= value; }
};

/** for-each functor: divides operand by constant divisor. */
template <typename T>
struct foreach_divide_by_const
{
    T div;
    foreach_divide_by_const (T const& div) : div(div) {}
    void operator() (T& val) { val /= div; }
};

/** for-each functor: adds a constant value to operand. */
template <typename T>
struct foreach_addition_with_const
{
    T value;
    foreach_addition_with_const (T const& value) : value(value) {}
    void operator() (T& val) { val += value; }
};

/** for-each functor: substracts a constant value to operand. */
template <typename T>
struct foreach_substraction_with_const
{
    T value;
    foreach_substraction_with_const (T const& value) : value(value) {}
    void operator() (T& val) { val -= value; }
};

/** for-each functor: raises each operand to the power of constant value. */
template <typename T>
struct foreach_constant_power
{
    T value;
    foreach_constant_power (T const& value) : value(value) {}
    void operator() (T& val) { val = std::pow(val, value); }
};

/** for-each functor: matrix-vector multiplication. */
template <typename M, typename V>
struct foreach_matrix_mult
{
    M mat;
    foreach_matrix_mult (M const& matrix) : mat(matrix) {}
    void operator() (V& vec) { vec = mat * vec; }
};

/** for-each functor: applies absolute value to operand. */
template <typename T>
inline void
foreach_absolute_value (T& val)
{
    val = std::abs(val);
}

/** for-each functor: negates the operand. */
template <typename T>
inline void
foreach_negate_value (T& val)
{
    val = -val;
}

/** for-each functor: inverts floating point values with 1/value. */
template <typename T>
inline void
foreach_invert_value (T& val)
{
    val = T(1) / val;
}

/** for-each functor: applies floor operation to the operand. */
template <typename T>
inline void
foreach_floor (T& val)
{
    val = std::floor(val);
}

/** for-each functor: applies ceil operation to the operand. */
template <typename T>
inline void
foreach_ceil (T& val)
{
    val = std::ceil(val);
}

/** for-each functor: applies rounding to the operand. */
template <typename T>
inline void
foreach_round (T& val)
{
    val = round(val);
}

MATH_ALGO_NAMESPACE_END
MATH_NAMESPACE_END

#endif /* MATH_ALGO_HEADER */
