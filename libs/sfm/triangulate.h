/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_TRIANGULATE_HEADER
#define SFM_TRIANGULATE_HEADER

#include <vector>
#include <ostream>

#include "math/vector.h"
#include "sfm/correspondence.h"
#include "sfm/camera_pose.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN

/* ---------------- Low-level triangulation solver ---------------- */

/**
 * Given an image correspondence in two views and the corresponding poses,
 * this function triangulates the 3D point coordinate using the DLT algorithm.
 */
math::Vector<double, 3>
triangulate_match (Correspondence2D2D const& match,
    CameraPose const& pose1, CameraPose const& pose2);

/**
 * Given any number of 2D image positions and the corresponding camera poses,
 * this function triangulates the 3D point coordinate using the DLT algorithm.
 */
math::Vector<double, 3>
triangulate_track (std::vector<math::Vec2f> const& pos,
    std::vector<CameraPose const*> const& poses);

/**
 * Given a two-view pose configuration and a correspondence, this function
 * returns true if the triangulated point is in front of both cameras.
 */
bool
is_consistent_pose (Correspondence2D2D const& match,
    CameraPose const& pose1, CameraPose const& pose2);

/* --------------- Higher-level triangulation class --------------- */

/**
 * Triangulation routine that triangulates a track from camera poses and
 * 2D image positions while keeping triangulation statistics. In contrast
 * to the low-level functions, this implementation checks for triangulation
 * problems such as large reprojection error, tracks appearing behind the
 * camera, and unstable triangulation angles.
 */
class Triangulate
{
public:
    struct Options
    {
        Options (void);

        /** Threshold on reprojection error for outlier detection. */
        double error_threshold;
        /** Threshold on the triangulation angle (in radians). */
        double angle_threshold;
        /** Minimal number of views with small error (inliers). */
        int min_num_views;
    };

    struct Statistics
    {
        Statistics (void);

        /** The number of successfully triangulated tracks. */
        int num_new_tracks;
        /** Number of tracks with too large reprojection error. */
        int num_large_error;
        /** Number of tracks that appeared behind the camera. */
        int num_behind_camera;
        /** Number of tracks with too small triangulation angle. */
        int num_too_small_angle;
    };

public:
    explicit Triangulate (Options const& options);
    bool triangulate (std::vector<CameraPose const*> const& poses,
        std::vector<math::Vec2f> const& positions,
        math::Vec3d* track_pos, Statistics* stats = nullptr,
        std::vector<std::size_t>* outliers = nullptr) const;
    void print_statistics (Statistics const& stats, std::ostream& out) const;

private:
    Options const opts;
    double const cos_angle_thres;
};

/* ------------------------ Implementation ------------------------ */

inline
Triangulate::Options::Options (void)
    : error_threshold(0.01)
    , angle_threshold(MATH_DEG2RAD(1.0))
    , min_num_views(2)
{
}

inline
Triangulate::Statistics::Statistics (void)
    : num_new_tracks(0)
    , num_large_error(0)
    , num_behind_camera(0)
    , num_too_small_angle(0)
{
}

inline
Triangulate::Triangulate (Options const& options)
    : opts(options)
    , cos_angle_thres(std::cos(options.angle_threshold))
{
}

SFM_NAMESPACE_END

#endif // SFM_TRIANGULATE_HEADER
