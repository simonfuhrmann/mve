#include "sfm/matrixsvd.h"
#include "sfm/triangulate.h"

SFM_NAMESPACE_BEGIN

math::Vector<double, 3>
triangulate_match (Correspondence const& match,
    CameraPose const& pose1, CameraPose const& pose2)
{
    math::Matrix<double, 3, 4> P1, P2;
    pose1.fill_p_matrix(&P1);
    pose2.fill_p_matrix(&P2);

    math::Matrix<double, 4, 4> A;
    for (int i = 0; i < 4; ++i)
    {
        A(0, i) = match.p1[0] * P1(2, i) - P1(0, i);
        A(1, i) = match.p1[1] * P1(2, i) - P1(1, i);
        A(2, i) = match.p2[0] * P2(2, i) - P2(0, i);
        A(3, i) = match.p2[1] * P2(2, i) - P2(1, i);
    }

    math::Matrix<double, 4, 4> U, S, V;
    math::matrix_svd(A, &U, &S, &V);
    math::Vector<double, 4> x = V.col(3);
    return math::Vector<double, 3>(x[0] / x[3], x[1] / x[3], x[2] / x[3]);
}

bool
is_consistent_pose (Correspondence const& match,
    CameraPose const& pose1, CameraPose const& pose2)
{
    math::Vector<double, 3> x = triangulate_match(match, pose1, pose2);
    math::Vector<double, 3> x1 = pose1.R * x + pose1.t;
    math::Vector<double, 3> x2 = pose2.R * x + pose2.t;
    if (x1[2] > 0.0f && x2[2] > 0.0f)
        return true;

    return false;
}

SFM_NAMESPACE_END
