/*
 * Representation of a line in 3D.
 * Written by Simon Fuhrmann.
 */

#ifndef MATH_LINE_HEADER
#define MATH_LINE_HEADER

#include "math/defines.h"
#include "math/vector.h"

MATH_NAMESPACE_BEGIN

template <class T> class Line3;
typedef Line3<float> Line3f;
typedef Line3<double> Line3d;

/**
 * Class that represents a line using a point and a vector.
 * The normal is expected to have unit length.
 */
template <class T>
class Line3
{
public:
    typedef math::Vector<T, 3> Vec3T;

public:
    Vec3T d;
    Vec3T p;

public:
    /** Creates an uninitialized plane. */
    Line3 (void);

    /** Creates a plane with normal n and distance d from the origin. */
    Line3 (Vec3T const& d, Vec3T const& p);
};

/* ---------------------------------------------------------------- */

template <class T>
inline
Line3<T>::Line3 (void)
{
}

template <class T>
inline
Line3<T>::Line3 (Vec3T const& d, Vec3T const& p)
    : d(d), p(p)
{
}

MATH_NAMESPACE_END

#endif /* MATH_LINE_HEADER */
