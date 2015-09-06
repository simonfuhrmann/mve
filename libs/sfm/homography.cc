/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <set>
#include <stdexcept>
#include <vector>

#include "math/matrix_tools.h"
#include "math/matrix_svd.h"
#include "sfm/homography.h"

SFM_NAMESPACE_BEGIN

bool
homography_dlt (Correspondences2D2D const& points, HomographyMatrix* result)
{
    if (points.size() < 4)
        throw std::invalid_argument("At least 4 matches required");

    /* Create 2Nx9 matrix A. Each correspondence creates two rows in A. */
    std::vector<double> A(2 * points.size() * 9);
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        std::size_t const row1 = 9 * i;
        std::size_t const row2 = 9 * (i + points.size());
        Correspondence2D2D const& match = points[i];
        A[row1 + 0] =  0.0;
        A[row1 + 1] =  0.0;
        A[row1 + 2] =  0.0;
        A[row1 + 3] =  match.p1[0];
        A[row1 + 4] =  match.p1[1];
        A[row1 + 5] =  1.0;
        A[row1 + 6] = -match.p1[0] * match.p2[1];
        A[row1 + 7] = -match.p1[1] * match.p2[1];
        A[row1 + 8] = -match.p2[1];
        A[row2 + 0] = -match.p1[0];
        A[row2 + 1] = -match.p1[1];
        A[row2 + 2] = -1.0;
        A[row2 + 3] =  0.0;
        A[row2 + 4] =  0.0;
        A[row2 + 5] =  0.0;
        A[row2 + 6] =  match.p1[0] * match.p2[0];
        A[row2 + 7] =  match.p1[1] * match.p2[0];
        A[row2 + 8] =  match.p2[0];
    }

    /* Compute homography matrix using SVD. */
    math::Matrix<double, 9, 9> V;
    math::matrix_svd<double>(&A[0], 2 * points.size(),
        9, nullptr, nullptr, V.begin());

    /* Only consider the last column of V as the solution. */
    for (int i = 0; i < 9; ++i)
        (*result)[i] = V[i * 9 + 8];

    return true;
}

double
symmetric_transfer_error(HomographyMatrix const& homography,
    Correspondence2D2D const& match)
{
    /*
     * Computes the symmetric transfer error for a given match and homography
     * matrix. The error is computed as [Sect 4.2.2, Hartley, Zisserman]:
     *
     *   e = d(x, (H^-1)x')^2 + d(x', Hx)^2
     */
    math::Vec3d p1(match.p1[0], match.p1[1], 1.0);
    math::Vec3d p2(match.p2[0], match.p2[1], 1.0);

    math::Matrix3d invH = math::matrix_inverse(homography);
    math::Vec3d result = invH * p2;
    result /= result[2];
    double error = (p1 - result).square_norm();

    result = homography * p1;
    result /= result[2];
    error += (result - p2).square_norm();

    return 0.5 * error;
}

SFM_NAMESPACE_END
