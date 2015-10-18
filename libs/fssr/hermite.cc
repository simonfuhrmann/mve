/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

#include "fssr/hermite.h"

#define EPSILON 1e-8

FSSR_NAMESPACE_BEGIN

double
find_root_linear (double a0, double a1)
{
    return -a0 / a1;
}

double
find_root_square (double a0, double a1, double a2)
{
    if (a2 > -EPSILON && a2 < EPSILON)
        return find_root_linear(a0, a1);

    /* Normalize polynomial. */
    a0 = a0 / a2;
    a1 = a1 / a2;

    /* Solve using PQ formula. */
    double const pq1 = -a1 / 2.0;
    double const pq2 = std::sqrt(std::max(0.0, a1 * a1 / 4.0 - a0));
    double const sol1 = pq1 + pq2;
    double const sol2 = pq1 - pq2;

    /* Select root which is closer to 0.5. */
    if (std::abs(sol1 - 0.5) < std::abs(sol2 - 0.5))
        return sol1;
    else
        return sol2;
}

/*
 * The idea is inspired from:
 * http://tavianator.com/solving-cubic-polynomials/
 */
double
find_root_cubic (double a0, double a1, double a2, double a3)
{
    if (a3 > -EPSILON && a3 < EPSILON)
        return find_root_square(a0, a1, a2);

    /* Normalize polynomial. */
    a0 = a0 / a3;
    a1 = a1 / a3;
    a2 = a2 / a3;

    /* Reduce to depressed cubic t^3 + pt + q using x = t - a2 / (3 a3). */
    double const p = a1 - a2 * a2 / 3.0;
    double const q = a0 - a1 * a2 / 3.0 + a2 * a2 * a2 / 13.5;
    double const discr = 4.0 * p * p * p + 27.0 * q * q;

    if (discr < 0.0)
    {
        /* Three real roots. */
        double const sqrtp3 = std::sqrt(-p / 3.0);
        double const theta = std::acos(3.0 * q / (-2.0 * p * sqrtp3)) / 3.0;
        double root[3];
        root[2] = -2.0 * sqrtp3 * std::cos(theta) - a2 / 3.0;
        root[0] = 2.0 * sqrtp3 * std::cos(4.0 * std::atan(1.0) / 3.0 - theta) - a2 / 3.0;
        root[1] = -(root[0] + root[2] + a2);

        /* Check how many roots are in [0, 1]. */
        double the_root = 0.0;
        int num_roots = 0;
        for (int i = 0; i < 3; ++i)
            if (root[i] >= 0.0 && root[i] <= 1.0)
            {
                the_root = root[i];
                num_roots += 1;
            }

        if (num_roots == 1)
        {
            //std::cout << "One of three roots in range" << std::endl;
            return the_root;
        }
        else
        {
#if 1
            /* Return middle root. */
            return root[1];
#else
            /* Find linear interpolant. */
            double const v0 = a0 * a3;
            double const v1 = (a0 + a1 + a2 + 1.0) * a3;
            return find_root_linear(v0, v1 - v0);
#endif
        }
    }
    else if (discr > 0.0)
    {
        /* One real root. Return it. */
        //double const c = std::cbrt(std::sqrt(disc / 108.0) + std::abs(q) / 2.0);
        double const c = std::pow(std::sqrt(discr / 108.0) + std::abs(q) / 2.0, 1.0 / 3.0);
        double const t = c - p / (3.0 * c);

        if (q >= 0.0)
        {
            //std::cout << "One real root 1" << std::endl;
            return -t - a2 / 3.0;
        }
        else
        {
            //std::cout << "One real root 2" << std::endl;
            return t - a2 / 3.0;
        }
    }
    else
    {
        /* One duplicate and one single root. Extract single root only. */
        if (p > -EPSILON && p < EPSILON)
        {
            //std::cout << "Mixed case 1" << std::endl;
            return a2 / 3.0;
        }
        else
        {
            //std::cout << "Mixed case 1" << std::endl;
            return 3.0 * q / p;
        }
    }

    throw std::runtime_error("Unreachable code");
}

double
interpolate_root (double v0, double v1, double d0, double d1,
    InterpolationType type)
{
    double root = 0.0;
    double a0 = 0.0, a1 = 0.0, a2 = 0.0, a3 = 0.0;
    switch (type)
    {
        case INTERPOLATION_LINEAR:
        {
            a0 = v0;
            a1 = v1 - v0;
            root = find_root_linear(a0, a1);
            break;
        }
        case INTERPOLATION_SCALING:
        {
            double scale = 2.0 * (v1 - v0) / (d0 + d1);
            a0 = v0;
            a1 = d0 * scale;
            a2 = 3.0 * (v1 - v0) - (2.0 * d0 + d1) * scale;
            root = find_root_square(a0, a1, a2);
            break;
        }
        case INTERPOLATION_LSDERIV:
        {
            a0 = v0;
            a1 = (d0 - d1) / 2.0 + v1 - v0;
            a2 = (d1 - d0) / 2.0;
            root = find_root_square(a0, a1, a2);
            break;
        }
        case INTERPOLATION_CUBIC:
        {
            a0 = v0;
            a1 = d0;
            a2 = 3.0 * v1 - 3.0 * v0 - 2.0 * d0 - 1.0 * d1;
            a3 = 2.0 * v0 - 2.0 * v1 + d0 + d1;
            root = find_root_cubic(a0, a1, a2, a3);
            break;
        }
        default:
            throw std::runtime_error("Invalid interpolation type");
    }

    return std::min(1.0, std::max(0.0, root));
}

FSSR_NAMESPACE_END
