/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef FSSR_HERMITE_HEADER
#define FSSR_HERMITE_HEADER

#include "fssr/defines.h"

FSSR_NAMESPACE_BEGIN

enum InterpolationType
{
    INTERPOLATION_LINEAR,
    INTERPOLATION_SCALING,
    INTERPOLATION_LSDERIV,
    INTERPOLATION_CUBIC
};

/**
 * Finds the root of a linear function f(x) = a0 + a1(x).
 * The method produces unstable results if a1 ~ 0.
 */
double
find_root_linear (double a0, double a1);

/**
 * Finds the root of a quadratic function f(x) = a0 + a1 * x + a2 * x^2.
 * The code assumes a root in [0, 1] and the root closer to 0.5 is returned.
 */
double
find_root_square (double a0, double a1, double a2);

/**
 * Finds the root of a cubic function f(x) = a0 + a1 * x + a2 * x^2 + a3 * x^3.
 * The code assumes a single root in [0, 1]. If [0, 1] contains more than one
 * root, linear interpolation is used. In the case with two distinct roots
 * (one single, one double root), the double root is ignored.
 */
double
find_root_cubic (double a0, double a1, double a2, double a3);

/**
 * Interpolates the root of an unknown function f(x) given value and
 * derivative constraints: f(0) = v0, f(1) = v1, f'(0) = d0, f'(1) = d1.
 * First, a polynomial function is fit to the constraints, then the root
 * in the interval [0, 1] is determined.
 */
double
interpolate_root (double v0, double v1, double d0, double d1,
    InterpolationType type = INTERPOLATION_CUBIC);

FSSR_NAMESPACE_END

#endif /* FSSR_HERMITE_HEADER */
