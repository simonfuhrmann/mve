#include "math/matrix_svd.h"
#include "sfm/triangulate.h"

SFM_NAMESPACE_BEGIN

math::Vector<double, 3>
triangulate_match (Correspondence const& match,
    CameraPose const& pose1, CameraPose const& pose2)
{
    /* The algorithm is described in HZ 12.2, page 312. */
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

    math::Matrix<double, 4, 4> V;
    math::matrix_svd<double, 4, 4>(A, NULL, NULL, &V);
    math::Vector<double, 4> x = V.col(3);
    return math::Vector<double, 3>(x[0] / x[3], x[1] / x[3], x[2] / x[3]);
}

math::Vector<double, 3>
triangulate_track (std::vector<math::Vec2f> const& pos,
    std::vector<CameraPose const*> poses)
{
    if (pos.size() != poses.size() || pos.size() < 2)
        throw std::invalid_argument("Invalid number of positions/poses");

    std::vector<double> A(4 * 2 * poses.size(), 0.0);
    for (std::size_t i = 0; i < poses.size(); ++i)
    {
        CameraPose const& pose = *poses[i];
        math::Vec2d p = pos[i];
        math::Matrix<double, 3, 4> p_mat;
        pose.fill_p_matrix(&p_mat);

        for (int j = 0; j < 4; ++j)
        {
            A[(2 * i + 0) * 4 + j] = p[0] * p_mat(2, j) - p_mat(0, j);
            A[(2 * i + 1) * 4 + j] = p[1] * p_mat(2, j) - p_mat(1, j);
        }
    }

    /* Compute SVD. */
    math::Matrix<double, 4, 4> mat_v;
    math::matrix_svd<double>(&A[0], 2 * poses.size(), 4,
        NULL, NULL, mat_v.begin());

    /* Consider the last column of V and extract 3D point. */
    math::Vector<double, 4> x = mat_v.col(3);
    return math::Vector<double, 3>(x[0] / x[3], x[1] / x[3], x[2] / x[3]);
}

bool
is_consistent_pose (Correspondence const& match,
    CameraPose const& pose1, CameraPose const& pose2)
{
    math::Vector<double, 3> x = triangulate_match(match, pose1, pose2);
    math::Vector<double, 3> x1 = pose1.R * x + pose1.t;
    math::Vector<double, 3> x2 = pose2.R * x + pose2.t;
    return x1[2] > 0.0f && x2[2] > 0.0f;
}

SFM_NAMESPACE_END
