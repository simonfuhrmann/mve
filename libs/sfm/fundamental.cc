/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <limits>
#include <stdexcept>

#include "math/matrix_tools.h"
#include "math/matrix_svd.h"
#include "math/functions.h"
#include "sfm/fundamental.h"

SFM_NAMESPACE_BEGIN

namespace
{
    /**
     * Creates the cross product matrix [x] for x. With this matrix,
     * the cross product cross(x, y) can be expressed using matrix
     * multiplication [x] y.
     */
    template <typename T>
    void
    cross_product_matrix (math::Vector<T, 3> const& v,
        math::Matrix<T, 3, 3>* result)
    {
        result->fill(T(0));
        (*result)(0, 1) = -v[2];
        (*result)(0, 2) = v[1];
        (*result)(1, 0) = v[2];
        (*result)(1, 2) = -v[0];
        (*result)(2, 0) = -v[1];
        (*result)(2, 1) = v[0];
    }

}  // namespace

bool
fundamental_least_squares (Correspondences2D2D const& points,
    FundamentalMatrix* result)
{
    if (points.size() < 8)
        throw std::invalid_argument("At least 8 points required");

    /* Create Nx9 matrix A. Each correspondence creates on row in A. */
    std::vector<double> A(points.size() * 9);
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        Correspondence2D2D const& p = points[i];
        A[i * 9 + 0] = p.p2[0] * p.p1[0];
        A[i * 9 + 1] = p.p2[0] * p.p1[1];
        A[i * 9 + 2] = p.p2[0] * 1.0;
        A[i * 9 + 3] = p.p2[1] * p.p1[0];
        A[i * 9 + 4] = p.p2[1] * p.p1[1];
        A[i * 9 + 5] = p.p2[1] * 1.0;
        A[i * 9 + 6] = 1.0     * p.p1[0];
        A[i * 9 + 7] = 1.0     * p.p1[1];
        A[i * 9 + 8] = 1.0     * 1.0;
    }

    /* Compute fundamental matrix using SVD. */
    std::vector<double> V(9 * 9);
    math::matrix_svd<double>(&A[0], points.size(), 9, nullptr, nullptr, &V[0]);

    /* Use last column of V as solution. */
    for (int i = 0; i < 9; ++i)
        (*result)[i] = V[i * 9 + 8];

    return true;
}

bool
fundamental_8_point (Eight2DPoints const& points_view_1,
    Eight2DPoints const& points_view_2, FundamentalMatrix* result)
{
    /*
     * Create 8x9 matrix A. Each pair of input points creates on row in A.
     */
    math::Matrix<double, 8, 9> A;
    for (int i = 0; i < 8; ++i)
    {
        math::Vector<double, 3> p1 = points_view_1.col(i);
        math::Vector<double, 3> p2 = points_view_2.col(i);
        A(i, 0) = p2[0] * p1[0];
        A(i, 1) = p2[0] * p1[1];
        A(i, 2) = p2[0] * 1.0;
        A(i, 3) = p2[1] * p1[0];
        A(i, 4) = p2[1] * p1[1];
        A(i, 5) = p2[1] * 1.0;
        A(i, 6) = 1.0   * p1[0];
        A(i, 7) = 1.0   * p1[1];
        A(i, 8) = 1.0   * 1.0;
    }

    /*
     * The fundamental matrix F is created from the singular
     * vector corresponding to the smallest eigenvalue of A.
     */
    math::Matrix<double, 9, 9> V;
    math::matrix_svd<double, 8, 9>(A, nullptr, nullptr, &V);
    math::Vector<double, 9> f = V.col(8);
    std::copy(*f, *f + 9, **result);

    return true;
}

void
enforce_fundamental_constraints (FundamentalMatrix* matrix)
{
    /*
     * Constraint enforcement. The fundamental matrix F has rank 2 and 7
     * degrees of freedom. However, F' computed from point correspondences may
     * not have rank 2 and it needs to be enforced. To this end, the SVD is
     * used: F' = USV*, F = UDV* where D = diag(s1, s2, 0) and s1 and s2
     * are the largest and second largest eigenvalues of F.
     */
    math::Matrix<double, 3, 3> U, S, V;
    math::matrix_svd(*matrix, &U, &S, &V);
    S(2, 2) = 0.0;
    *matrix = U * S * V.transposed();
}

void
enforce_essential_constraints (EssentialMatrix* matrix)
{
    /*
     * Constraint enforcement. The essential matrix E has rank 2 and 5 degrees
     * of freedom. However, E' computed from normalized image correspondences
     * may not have rank 2 and it needs to be enforced. To this end, the SVD is
     * used: F' = USV*, F = UDV* where D = diag(s, s, 0), s = (s1 + s2) / 2
     * and s1 and s2 are the largest and second largest eigenvalues of F.
     */
    math::Matrix<double, 3, 3> U, S, V;
    math::matrix_svd(*matrix, &U, &S, &V);
    double avg = (S(0, 0) + S(0, 1)) / 2.0;
    S(0, 0) = avg;
    S(1, 1) = avg;
    S(2, 2) = 0.0;
    *matrix = U * S * V.transposed();
}

void
pose_from_essential (EssentialMatrix const& matrix,
    std::vector<CameraPose>* result)
{
    /*
     * The pose [R|t] for the second camera is extracted from the essential
     * matrix E and the first camera is given in canonical form [I|0].
     * The SVD of E = USV is computed. The scale of S' diagonal entries is
     * irrelevant and S is assumed to be diag(1,1,0).
     * Details can be found in [Hartley, Zisserman, Sect. 9.6.1].
     */

    math::Matrix<double, 3, 3> W(0.0);
    W(0, 1) = -1.0; W(1, 0) = 1.0; W(2, 2) = 1.0;
    math::Matrix<double, 3, 3> Wt(0.0);
    Wt(0, 1) = 1.0; Wt(1, 0) = -1.0; Wt(2, 2) = 1.0;

    math::Matrix<double, 3, 3> U, S, V;
    math::matrix_svd(matrix, &U, &S, &V);

    // This seems to ensure that det(R) = 1 (instead of -1).
    // FIXME: Is this the correct way to do it?
    if (math::matrix_determinant(U) < 0.0)
        for (int i = 0; i < 3; ++i)
            U(i,2) = -U(i,2);
    if (math::matrix_determinant(V) < 0.0)
        for (int i = 0; i < 3; ++i)
            V(i,2) = -V(i,2);

    V.transpose();
    result->clear();
    result->resize(4);
    result->at(0).R = U * W * V;
    result->at(1).R = result->at(0).R;
    result->at(2).R = U * Wt * V;
    result->at(3).R = result->at(2).R;
    result->at(0).t = U.col(2);
    result->at(1).t = -result->at(0).t;
    result->at(2).t = result->at(0).t;
    result->at(3).t = -result->at(0).t;

    // FIXME: Temporary sanity check.
    if (!MATH_EPSILON_EQ(math::matrix_determinant(result->at(0).R), 1.0, 1e-3))
        throw std::runtime_error("Invalid rotation matrix");
}

void
fundamental_from_pose (CameraPose const& cam1, CameraPose const& cam2,
    FundamentalMatrix* result)
{
    /*
     * The fundamental matrix is obtained from camera matrices.
     * See Hartley Zisserman (9.1): F = [e2] P2 P1^.
     * Where P1, P2 are the camera matrices, and P^ is the inverse of P.
     * e2 is the epipole in the second camera and [e2] is the cross product
     * matrix for e2.
     */
    math::Matrix<double, 3, 4> P1, P2;
    cam1.fill_p_matrix(&P1);
    cam2.fill_p_matrix(&P2);

    math::Vec4d c1(cam1.R.transposed() * -cam1.t, 1.0);
    math::Vec3d e2 = P2 * c1;

    math::Matrix3d ex;
    cross_product_matrix(e2, &ex);

    // FIXME: The values in the fundamental matrix can become huge.
    // The input projection matrix should be given in unit coodinates,
    // not pixel coordinates? Test and document that.

    math::Matrix<double, 4, 3> P1inv;
    math::matrix_pseudo_inverse(P1, &P1inv);
    *result = ex * P2 * P1inv;
}


double
sampson_distance (FundamentalMatrix const& F, Correspondence2D2D const& m)
{
    /*
     * Computes the Sampson distance SD for a given match and fundamental
     * matrix. SD is computed as [Sect 11.4.3, Hartley, Zisserman]:
     *
     *   SD = (x'Fx)^2 / ( (Fx)_1^2 + (Fx)_2^2 + (x'F)_1^2 + (x'F)_2^2 )
     */

    double p2_F_p1 = 0.0;
    p2_F_p1 += m.p2[0] * (m.p1[0] * F[0] + m.p1[1] * F[1] + F[2]);
    p2_F_p1 += m.p2[1] * (m.p1[0] * F[3] + m.p1[1] * F[4] + F[5]);
    p2_F_p1 +=     1.0 * (m.p1[0] * F[6] + m.p1[1] * F[7] + F[8]);
    p2_F_p1 *= p2_F_p1;

    double sum = 0.0;
    sum += math::fastpow(m.p1[0] * F[0] + m.p1[1] * F[1] + F[2], 2);
    sum += math::fastpow(m.p1[0] * F[3] + m.p1[1] * F[4] + F[5], 2);
    sum += math::fastpow(m.p2[0] * F[0] + m.p2[1] * F[3] + F[6], 2);
    sum += math::fastpow(m.p2[0] * F[1] + m.p2[1] * F[4] + F[7], 2);

    return p2_F_p1 / sum;
}

SFM_NAMESPACE_END
