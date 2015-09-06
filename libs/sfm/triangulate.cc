/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <stdexcept>

#include "math/matrix_svd.h"
#include "sfm/triangulate.h"

SFM_NAMESPACE_BEGIN

/* ---------------- Low-level triangulation solver ---------------- */

math::Vector<double, 3>
triangulate_match (Correspondence2D2D const& match,
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
    math::matrix_svd<double, 4, 4>(A, nullptr, nullptr, &V);
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
        nullptr, nullptr, mat_v.begin());

    /* Consider the last column of V and extract 3D point. */
    math::Vector<double, 4> x = mat_v.col(3);
    return math::Vector<double, 3>(x[0] / x[3], x[1] / x[3], x[2] / x[3]);
}

bool
is_consistent_pose (Correspondence2D2D const& match,
    CameraPose const& pose1, CameraPose const& pose2)
{
    math::Vector<double, 3> x = triangulate_match(match, pose1, pose2);
    math::Vector<double, 3> x1 = pose1.R * x + pose1.t;
    math::Vector<double, 3> x2 = pose2.R * x + pose2.t;
    return x1[2] > 0.0f && x2[2] > 0.0f;
}

/* --------------- Higher-level triangulation class --------------- */

bool
Triangulate::triangulate (std::vector<CameraPose const*> const& poses,
    std::vector<math::Vec2f> const& positions,
    math::Vec3d* track_pos,
    Statistics* stats) const
{
    if (poses.size() < 2)
        throw std::invalid_argument("At least two poses required");
    if (poses.size() != positions.size())
        throw std::invalid_argument("Poses and positions size mismatch");

    /* Triangulate track. */
    *track_pos = triangulate_track(positions, poses);

    /* Check if track has small triangulation angle. */
    double smallest_cos_angle = 1.0;
    if (this->opts.angle_threshold > 0.0)
    {
        std::vector<math::Vec3d> rays(poses.size());
        for (std::size_t i = 0; i < poses.size(); ++i)
        {
            math::Vec3d camera_pos;
            poses[i]->fill_camera_pos(&camera_pos);
            rays[i] = (*track_pos - camera_pos).normalized();
        }

        for (std::size_t i = 0; i < rays.size(); ++i)
            for (std::size_t j = 0; j < i; ++j)
            {
                double const cos_angle = rays[i].dot(rays[j]);
                smallest_cos_angle = std::min(smallest_cos_angle, cos_angle);
            }

        if (smallest_cos_angle > this->cos_angle_thres)
        {
            if (stats != nullptr)
                stats->num_too_small_angle += 1;
            return false;
        }
    }

    /* Compute reprojection error of the track. */
    double average_error = 0.0;
    for (std::size_t i = 0; i < poses.size(); ++i)
    {
        math::Vec3d x = poses[i]->R * *track_pos + poses[i]->t;

        /* Reject track if it appears behind the camera. */
        if (x[2] < 0.0)
        {
            if (stats != nullptr)
                stats->num_behind_camera += 1;
            return false;
        }

        x = poses[i]->K * x;
        math::Vec2d x2d(x[0] / x[2], x[1] / x[2]);
        average_error += (positions[i] - x2d).norm();
    }
    average_error /= static_cast<double>(poses.size());

    /* Reject track if the reprojection error is too large. */
    if (average_error > this->opts.error_threshold)
    {
        if (stats != nullptr)
            stats->num_large_error += 1;
        return false;
    }

    if (stats != nullptr)
        stats->num_new_tracks += 1;

    return true;
}

void
Triangulate::print_statistics (Statistics const& stats, std::ostream& out) const
{
    int const num_rejected = stats.num_large_error
        + stats.num_behind_camera
        + stats.num_too_small_angle;

    out << "Triangulated " << stats.num_new_tracks
        << " new tracks, rejected " << num_rejected
        << " bad tracks." << std::endl;
    if (stats.num_large_error > 0)
        out << "  Rejected " << stats.num_large_error
            << " tracks with large error." << std::endl;
    if (stats.num_behind_camera > 0)
        out << "  Rejected " << stats.num_behind_camera
            << " tracks behind cameras." << std::endl;
    if (stats.num_too_small_angle > 0)
        out << "  Rejected " << stats.num_too_small_angle
            << " tracks with unstable angle." << std::endl;
}

SFM_NAMESPACE_END
