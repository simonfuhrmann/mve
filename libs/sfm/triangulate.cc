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
    math::Vec3d* track_pos, Statistics* stats,
    std::vector<std::size_t>* outliers) const
{
    if (poses.size() < 2)
        throw std::invalid_argument("At least two poses required");
    if (poses.size() != positions.size())
        throw std::invalid_argument("Poses and positions size mismatch");

    /* Check all possible pose pairs for successful triangulation */
    std::vector<std::size_t> best_outliers(positions.size());
    math::Vec3f best_pos(0.0f);
    for (std::size_t p1 = 0; p1 < poses.size(); ++p1)
        for (std::size_t p2 = p1 + 1; p2 < poses.size(); ++p2)
        {
            /* Triangulate position from current pair */
            std::vector<CameraPose const*> pose_pair;
            std::vector<math::Vec2f> position_pair;
            pose_pair.push_back(poses[p1]);
            pose_pair.push_back(poses[p2]);
            position_pair.push_back(positions[p1]);
            position_pair.push_back(positions[p2]);
            math::Vec3d tmp_pos = triangulate_track(position_pair, pose_pair);

            /* Check if pair has small triangulation angle. */
            if (this->opts.angle_threshold > 0.0)
            {
                math::Vec3d camera_pos;
                pose_pair[0]->fill_camera_pos(&camera_pos);
                math::Vec3d ray0 = (tmp_pos - camera_pos).normalized();
                pose_pair[1]->fill_camera_pos(&camera_pos);
                math::Vec3d ray1 = (tmp_pos - camera_pos).normalized();
                double const cos_angle = ray0.dot(ray1);
                if (cos_angle > this->cos_angle_thres)
                    continue;
            }

            /* Chek error in all input poses and find outliers */
            std::vector<std::size_t> tmp_outliers;
            for (std::size_t i = 0; i < poses.size(); ++i)
            {
                math::Vec3d x = poses[i]->R * tmp_pos + poses[i]->t;

                /* Reject track if it appears behind the camera. */
                if (x[2] <= 0.0)
                {
                    tmp_outliers.push_back(i);
                    continue;
                }

                x = poses[i]->K * x;
                math::Vec2d x2d(x[0] / x[2], x[1] / x[2]);
                double error = (positions[i] - x2d).norm();
                if (error > this->opts.error_threshold)
                    tmp_outliers.push_back(i);
            }

            /* Select triangulation with lowest amount of outliers */
            if (tmp_outliers.size() < best_outliers.size())
            {
                best_pos = tmp_pos;
                std::swap(best_outliers, tmp_outliers);
            }

        }

    /* If all pairs have small angles pos will be 0 here */
    if (best_pos.norm() == 0.0f)
    {
        if (stats != nullptr)
            stats->num_too_small_angle += 1;
        return false;
    }

    /* Check if required number of inliers is found */
    if (poses.size() < best_outliers.size() + this->opts.min_num_views)
    {
        if (stats != nullptr)
            stats->num_large_error += 1;
        return false;
    }

    /* Return final position and outliers */
    *track_pos = best_pos;
    if (stats != nullptr)
        stats->num_new_tracks += 1;
    if (outliers != nullptr)
        std::swap(*outliers, best_outliers);

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
