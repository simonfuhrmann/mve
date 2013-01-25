#include <limits>

#include "pose.h"
#include "eigenwrapper.h"

SFM_NAMESPACE_BEGIN

bool
pose_8_point (Eight2DPoints const& points_view_1,
    Eight2DPoints const& points_view_2,
    FundamentalMatrix* /*result*/)
{
    /*
     * The input points P are normalized such that the mean of the points
     * is zero and the points fit in the unit square. This makes solving
     * for the fundamental matrix numerically stable. The inverse
     * transformation is then applied afterwards.
     */
    math::Matrix<double, 3, 3> T1, T2;
    pose_find_normalization(points_view_1, &T1);
    pose_find_normalization(points_view_2, &T2);

    /*
     * Create 8x9 matrix A. Each pair of input points creates on row in A.
     */
    math::Matrix<double, 8, 9> A;
    for (int i = 0; i < 8; ++i)
    {
        math::Vector<double, 3> p1 = T1 * points_view_1.row(i);
        math::Vector<double, 3> p2 = T2 * points_view_2.row(i);
        A(i, 0) = p2[0] * p1[0];
        A(i, 1) = p2[0] * p1[1];
        A(i, 2) = p2[0];
        A(i, 3) = p2[1] * p1[0];
        A(i, 4) = p2[1] * p1[1];
        A(i, 5) = p2[1];
        A(i, 6) = p1[0];
        A(i, 7) = p1[1];
        A(i, 8) = 1.0;
    }

    /*
     * The fundamental matrix F is created from the singular
     * vector corresponding to the smallest eigenvalue of A.
     */
    FundamentalMatrix F;
    {
        math::Matrix<double, 8, 8> U;
        math::Matrix<double, 8, 9> S;
        math::Matrix<double, 9, 9> V;
        math::matrix_svd(A, &U, &S, &V);
        math::Vector<double, 9> f = V.col(8);
        std::copy(*f, *f + 9, *F);
    }

    /*
     * Constraint enforcement. The fundamental matrix F has rank 2. However,
     * the so far computed F may not have rank 2 and we enforce it by
     * replacing F = USV* with F' = UDV* where D = diag(s1, s2, 0) and
     * s1 and s2 are the largest and second largest eigenvalues of F.
     */
    {
        math::Matrix<double, 3, 3> U;
        math::Matrix<double, 3, 3> S;
        math::Matrix<double, 3, 3> V;
        math::matrix_svd(F, &U, &S, &V);
        S(2, 2) = 0.0;
        F = U * S * V.transposed();
    }

    /*
     * De-normalize fundamental matrix. Points have been normalized with
     * transformation x' = T * x. Since x1' * F * x2 = 0, de-normalization
     * of F is F' = T1* F T2.
     */
    F = T1.transposed() * F * T2;

    return true;
}

SFM_NAMESPACE_END
