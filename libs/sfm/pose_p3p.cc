/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <complex>
#include <algorithm>

#include "math/matrix.h"
#include "math/vector.h"
#include "sfm/pose_p3p.h"

SFM_NAMESPACE_BEGIN

namespace
{
    void
    solve_quartic_roots (math::Vec5d const& factors, math::Vec4d* real_roots)
    {
        double const A = factors[0];
        double const B = factors[1];
        double const C = factors[2];
        double const D = factors[3];
        double const E = factors[4];

        double const A2 = A * A;
        double const B2 = B * B;
        double const A3 = A2 * A;
        double const B3 = B2 * B;
        double const A4 = A3 * A;
        double const B4 = B3 * B;

        double const alpha = -3.0 * B2 / (8.0 * A2) + C / A;
        double const beta = B3 / (8.0 * A3)- B * C / (2.0 * A2) + D / A;
        double const gamma = -3.0 * B4 / (256.0 * A4) + B2 * C / (16.0 * A3)
            - B * D / (4.0 * A2) + E / A;

        double const alpha2 = alpha * alpha;
        double const alpha3 = alpha2 * alpha;
        double const beta2 = beta * beta;

        std::complex<double> P(-alpha2 / 12.0 - gamma, 0.0);
        std::complex<double> Q(-alpha3 / 108.0
            + alpha * gamma / 3.0 - beta2 / 8.0, 0.0);
        std::complex<double> R = -Q / 2.0
            + std::sqrt(Q * Q / 4.0 + P * P * P / 27.0);

        std::complex<double> U = std::pow(R, 1.0 / 3.0);
        std::complex<double> y = (U.real() == 0.0)
            ? -5.0 * alpha / 6.0 - std::pow(Q, 1.0 / 3.0)
            : -5.0 * alpha / 6.0 - P / (3.0 * U) + U;

        std::complex<double> w = std::sqrt(alpha + 2.0 * y);
        std::complex<double> part1 = -B / (4.0 * A);
        std::complex<double> part2 = 3.0 * alpha + 2.0 * y;
        std::complex<double> part3 = 2.0 * beta / w;

        std::complex<double> complex_roots[4];
        complex_roots[0] = part1 + 0.5 * (w + std::sqrt(-(part2 + part3)));
        complex_roots[1] = part1 + 0.5 * (w - std::sqrt(-(part2 + part3)));
        complex_roots[2] = part1 + 0.5 * (-w + std::sqrt(-(part2 - part3)));
        complex_roots[3] = part1 + 0.5 * (-w - std::sqrt(-(part2 - part3)));

        for (int i = 0; i < 4; ++i)
            (*real_roots)[i] = complex_roots[i].real();
    }
}

void
pose_p3p_kneip (
    math::Vec3d p1, math::Vec3d p2, math::Vec3d p3,
    math::Vec3d f1, math::Vec3d f2, math::Vec3d f3,
    std::vector<math::Matrix<double, 3, 4> >* solutions)
{
    /* Check if points are co-linear. In this case return no solution. */
    double const colinear_threshold = 1e-10;
    if ((p2 - p1).cross(p3 - p1).square_norm() < colinear_threshold)
    {
        solutions->clear();
        return;
    }

    /* Normalize directions if necessary. */
    double const normalize_epsilon = 1e-10;
    if (!MATH_EPSILON_EQ(f1.square_norm(), 1.0, normalize_epsilon))
        f1.normalize();
    if (!MATH_EPSILON_EQ(f2.square_norm(), 1.0, normalize_epsilon))
        f2.normalize();
    if (!MATH_EPSILON_EQ(f3.square_norm(), 1.0, normalize_epsilon))
        f3.normalize();

    /* Create camera frame. */
    math::Matrix3d T;
    {
        math::Vec3d e1 = f1;
        math::Vec3d e3 = f1.cross(f2).normalized();
        math::Vec3d e2 = e3.cross(e1);
        std::copy(e1.begin(), e1.end(), T.begin() + 0);
        std::copy(e2.begin(), e2.end(), T.begin() + 3);
        std::copy(e3.begin(), e3.end(), T.begin() + 6);
        f3 = T * f3;
    }

    /* Change camera frame and point order if f3[2] > 0. */
    if (f3[2] > 0.0)
    {
        std::swap(p1, p2);
        std::swap(f1, f2);

        math::Vec3d e1 = f1;
        math::Vec3d e3 = f1.cross(f2).normalized();
        math::Vec3d e2 = e3.cross(e1);
        std::copy(e1.begin(), e1.end(), T.begin() + 0);
        std::copy(e2.begin(), e2.end(), T.begin() + 3);
        std::copy(e3.begin(), e3.end(), T.begin() + 6);
        f3 = T * f3;
    }

    /* Create world frame. */
    math::Matrix3d N;
    {
        math::Vec3d n1 = (p2 - p1).normalized();
        math::Vec3d n3 = n1.cross(p3 - p1).normalized();
        math::Vec3d n2 = n3.cross(n1);
        std::copy(n1.begin(), n1.end(), N.begin() + 0);
        std::copy(n2.begin(), n2.end(), N.begin() + 3);
        std::copy(n3.begin(), n3.end(), N.begin() + 6);
    }
    p3 = N * (p3 - p1);

    /* Extraction of known parameters. */
    double d_12 = (p2 - p1).norm();
    double f_1 = f3[0] / f3[2];
    double f_2 = f3[1] / f3[2];
    double p_1 = p3[0];
    double p_2 = p3[1];

    double cos_beta = f1.dot(f2);
    double b = 1.0 / (1.0 - MATH_POW2(cos_beta)) - 1;

    if (cos_beta < 0.0)
        b = -std::sqrt(b);
    else
        b = std::sqrt(b);

    /* Temporary pre-computed variables. */
    double f_1_pw2 = MATH_POW2(f_1);
    double f_2_pw2 = MATH_POW2(f_2);
    double p_1_pw2 = MATH_POW2(p_1);
    double p_1_pw3 = p_1_pw2 * p_1;
    double p_1_pw4 = p_1_pw3 * p_1;
    double p_2_pw2 = MATH_POW2(p_2);
    double p_2_pw3 = p_2_pw2 * p_2;
    double p_2_pw4 = p_2_pw3 * p_2;
    double d_12_pw2 = MATH_POW2(d_12);
    double b_pw2 = MATH_POW2(b);

    /* Factors of the 4th degree polynomial. */
    math::Vec5d factors;
    factors[0] = - f_2_pw2 * p_2_pw4 - p_2_pw4 * f_1_pw2 - p_2_pw4;

    factors[1] = 2.0 * p_2_pw3 * d_12 * b
        + 2.0 * f_2_pw2 * p_2_pw3 * d_12 * b
        - 2.0 * f_2 * p_2_pw3 * f_1 * d_12;

    factors[2] = - f_2_pw2 * p_2_pw2 * p_1_pw2
        - f_2_pw2 * p_2_pw2 * d_12_pw2 * b_pw2
        - f_2_pw2 * p_2_pw2 * d_12_pw2
        + f_2_pw2 * p_2_pw4
        + p_2_pw4 * f_1_pw2
        + 2.0 * p_1 * p_2_pw2 * d_12
        + 2.0 * f_1 * f_2 * p_1 * p_2_pw2 * d_12 * b
        - p_2_pw2 * p_1_pw2 * f_1_pw2
        + 2.0 * p_1 * p_2_pw2 * f_2_pw2 * d_12
        - p_2_pw2 * d_12_pw2 * b_pw2
        - 2.0 * p_1_pw2 * p_2_pw2;

    factors[3] = 2.0 * p_1_pw2 * p_2 * d_12 * b
        + 2.0 * f_2 * p_2_pw3 * f_1 * d_12
        - 2.0 * f_2_pw2 * p_2_pw3 * d_12 * b
        - 2.0 * p_1 * p_2 * d_12_pw2 * b;

    factors[4] = -2.0 * f_2 * p_2_pw2 * f_1 * p_1 * d_12 * b
        + f_2_pw2 * p_2_pw2 * d_12_pw2
        + 2.0 * p_1_pw3 * d_12
        - p_1_pw2 * d_12_pw2
        + f_2_pw2 * p_2_pw2 * p_1_pw2
        - p_1_pw4
        - 2.0 * f_2_pw2 * p_2_pw2 * p_1 * d_12
        + p_2_pw2 * f_1_pw2 * p_1_pw2
        + f_2_pw2 * p_2_pw2 * d_12_pw2 * b_pw2;

    /* Solve for the roots of the polynomial. */
    math::Vec4d real_roots;
    solve_quartic_roots(factors, &real_roots);

    /* Back-substitution of each solution. */
    solutions->clear();
    solutions->resize(4);
    for (int i = 0; i < 4; ++i)
    {
        double cot_alpha = (-f_1 * p_1 / f_2 - real_roots[i] * p_2 + d_12 * b)
            / (-f_1 * real_roots[i] * p_2 / f_2 + p_1 - d_12);

        double cos_theta = real_roots[i];
        double sin_theta = std::sqrt(1.0 - MATH_POW2(real_roots[i]));
        double sin_alpha = std::sqrt(1.0 / (MATH_POW2(cot_alpha) + 1));
        double cos_alpha = std::sqrt(1.0 - MATH_POW2(sin_alpha));

        if (cot_alpha < 0.0)
            cos_alpha = -cos_alpha;

        math::Vec3d C(
            d_12 * cos_alpha * (sin_alpha * b + cos_alpha),
            cos_theta * d_12 * sin_alpha * (sin_alpha * b + cos_alpha),
            sin_theta * d_12 * sin_alpha * (sin_alpha * b + cos_alpha));

        C = p1 + N.transposed() * C;

        math::Matrix3d R;
        R[0] = -cos_alpha; R[1] = -sin_alpha * cos_theta; R[2] = -sin_alpha * sin_theta;
        R[3] = sin_alpha;  R[4] = -cos_alpha * cos_theta; R[5] = -cos_alpha * sin_theta;
        R[6] = 0.0;        R[7] = -sin_theta;             R[8] = cos_theta;

        R = N.transposed().mult(R.transposed()).mult(T);

        /* Convert camera position and cam-to-world rotation to pose. */
        R = R.transposed();
        C = -R * C;

        solutions->at(i) = R.hstack(C);
    }
}

SFM_NAMESPACE_END
