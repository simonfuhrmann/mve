/*
 * Quaternion class for arbitrary types.
 * Written by Simon Fuhrmann.
 */

#ifndef MATH_QUATERNION_HEADER
#define MATH_QUATERNION_HEADER

#include "math/defines.h"
#include "math/vector.h"

MATH_NAMESPACE_BEGIN

template <typename T> class Quaternion;
typedef Quaternion<float> Quat4f;
typedef Quaternion<double> Quat4d;
typedef Quaternion<int> Quat4i;
typedef Quaternion<unsigned int> Quat4ui;
typedef Quaternion<char> Quat4c;
typedef Quaternion<unsigned char> Quat4uc;

/**
 * Quaternion class for arbitrary types (WORK IN PROGRESS).
 */
template <class T>
class Quaternion : public Vector<T,4>
{
public:
    /** Creates a new, uninitialized quaternion. */
    Quaternion (void);

    /** Constructor that initializes ALL values. */
    Quaternion (T const& value);

    /** Constructor that takes all quaternion values. */
    Quaternion (T const& v1, T const& v2, T const& v3, T const& v4);

    /** Creates a quaternion from axis and angle. */
    Quaternion (Vector<T,3> const& axis, T const& angle);

    /** Sets the quaternion from axis and angle. */
    void set (Vector<T,3> const& axis, T const& angle);

    /** Provides the axis and angle of the quaternion. */
    void get_axis_angle (T* axis, T& angle);

    /* ---------------------- Unary operators --------------------- */

    /** Conjugates self and returns reference to self. */
    Quaternion<T>& conjugate (void);

    /** Returns a conjugated copy of self. */
    Quaternion<T> conjugated (void) const;

    /* --------------------- Binary operators --------------------- */

    /** Rotates a vector using the quaternion. */
    Vector<T,3> rotate (Vector<T,3> const& v) const;

    /* Binary operators. */
    //Quaternion<T> plus (T const& f) const
    //{ return Quaternion<T>(r + f, x, y, z); }
    //Quaternion<T> minus (T const& f) const
    //{ return Quaternion<T>(r - f, x, y, z); }

    /* --------------------- Object operators --------------------- */

    /** Quaternion with quaternion multiplication. */
    Quaternion<T> operator* (Quaternion<T> const& rhs) const;

    /** Quaternion self-multiplication. */
    Quaternion<T>& operator*= (Quaternion<T> const& rhs);
};

MATH_NAMESPACE_END

/* ------------------------ Implementation ------------------------ */

#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <numeric>

MATH_NAMESPACE_BEGIN

template <typename T>
inline
Quaternion<T>::Quaternion (void)
{
}

template <typename T>
inline
Quaternion<T>::Quaternion (T const& value)
    : Vector<T,4>(value)
{
}

template <typename T>
inline
Quaternion<T>::Quaternion (T const& v1, T const& v2, T const& v3, T const& v4)
{
    this->v[0] = v1;
    this->v[1] = v2;
    this->v[2] = v3;
    this->v[3] = v4;
}

template <typename T>
inline
Quaternion<T>::Quaternion (Vector<T,3> const& axis, T const& angle)
{
    this->set(axis, angle);
}

template <typename T>
void
Quaternion<T>::set (Vector<T,3> const& axis, T const& angle)
{
    this->v[0] = std::cos(angle / T(2));
    T sa = std::sin(angle / T(2));
    this->v[1] = axis[0] * sa;
    this->v[2] = axis[1] * sa;
    this->v[3] = axis[2] * sa;
}

template <typename T>
void
Quaternion<T>::get_axis_angle (T* axis, T& angle)
{
    T len = this->norm();
    if (len == T(0))
    {
        axis[0] = T(1);
        axis[1] = T(0);
        axis[2] = T(0);
        angle = T(0);
    }
    else
    {
        axis[0] = this->v[1] / len;
        axis[1] = this->v[2] / len;
        axis[2] = this->v[3] / len;
        angle = T(2) * std::acos(this->v[0]);
    }
}

template <typename T>
Vector<T,3>
Quaternion<T>::rotate (Vector<T,3> const& vec) const
{
    T xxzz = this->v[1] * this->v[1] - this->v[3] * this->v[3];
    T rryy = this->v[0] * this->v[0] - this->v[2] * this->v[2];
    T yyrrxxzz = this->v[2] * this->v[2] + this->v[0] * this->v[0]
        - this->v[1] * this->v[1] - this->v[3] * this->v[3];

    T xr2 = this->v[1] * this->v[0] * T(2);
    T xy2 = this->v[1] * this->v[2] * T(2);
    T xz2 = this->v[1] * this->v[3] * T(2);
    T yr2 = this->v[2] * this->v[0] * T(2);
    T yz2 = this->v[2] * this->v[3] * T(2);
    T zr2 = this->v[3] * this->v[0] * T(2);

    Vector<T,3> ret;
    ret[0] = (xxzz+rryy) * vec[0] + (xy2+zr2) * vec[1] + (xz2-yr2) * vec[2];
    ret[1] = (xy2-zr2) * vec[0] + (yyrrxxzz) * vec[1] + (yz2+xr2) * vec[2];
    ret[2] = (xz2+yr2) * vec[0] + (yz2-xr2) * vec[1] + (rryy-xxzz) * vec[2];
    return ret;
}

template <typename T>
inline Quaternion<T>
Quaternion<T>::operator* (Quaternion<T> const& rhs) const
{
      return Quaternion<T>(
          this->v[0] * rhs.v[0] - this->v[1] * rhs.v[1]
          - this->v[2] * rhs.v[2] - this->v[3] * rhs.v[3],
          this->v[0] * rhs.v[1] + this->v[1] * rhs.v[0]
          + this->v[2] * rhs.v[3] - this->v[3] * rhs.v[2],
          this->v[0] * rhs.v[2] - this->v[1] * rhs.v[3]
          + this->v[2] * rhs.v[0] + this->v[3] * rhs.v[1],
          this->v[0] * rhs.v[3] + this->v[1] * rhs.v[2]
          - this->v[2] * rhs.v[1] + this->v[3] * rhs.v[0]);
}

template <typename T>
inline Quaternion<T>&
Quaternion<T>::operator*= (Quaternion<T> const& rhs)
{
    *this = *this * rhs;
    return *this;
}

template <typename T>
inline Quaternion<T>&
Quaternion<T>::conjugate (void)
{
    this->v[1] = -this->v[1];
    this->v[2] = -this->v[2];
    this->v[3] = -this->v[3];
    return *this;
}

template <typename T>
inline Quaternion<T>
Quaternion<T>::conjugated (void) const
{
    return Quaternion<T>(*this).conjugate();
}

MATH_NAMESPACE_END

/* --------------------- Ouput stream adapter --------------------- */

#include <ostream>

MATH_NAMESPACE_BEGIN

/** Serializing a vector to an output stream. */
template <typename T, int N>
inline std::ostream&
operator<< (std::ostream& os, Quaternion<T> const& v)
{
    for (int i = 0; i < 4; ++i)
        os << v[i] << ( i < 3 ? " " : "");
    return os;
}

MATH_NAMESPACE_END

#endif /* MATH_QUATERNION_HEADER */
