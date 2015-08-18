/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_BUNDLER_INCREMENTAL_HEADER
#define SFM_BUNDLER_INCREMENTAL_HEADER

#include "mve/bundle.h"
#include "sfm/fundamental.h"
#include "sfm/ransac_fundamental.h"
#include "sfm/ransac_pose_p3p.h"
#include "sfm/bundler_common.h"
#include "sfm/camera_pose.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

/**
 * Bundler Component: Incremental structure-from-motion.
 */
class Incremental
{
public:
    struct Options
    {
        Options (void);

        /** Options for computing pose from 2D-3D correspondences. */
        RansacPoseP3P::Options pose_p3p_opts;
        /** Threshold (factor of the median error) for large error tracks. */
        double track_error_threshold_factor;
        /** Reprojection error threshold for newly triangulated tracks. */
        double new_track_error_threshold;
        /** Minimum angle for track triangulation in RAD. */
        double min_triangulation_angle;
        /** Bundle Adjustment fixed intrinsics. */
        bool ba_fixed_intrinsics;
        /** Bundle Adjustment with shared intrinsics. */
        bool ba_shared_intrinsics;
        /** Produce status messages on the console. */
        bool verbose_output;
    };

public:
    explicit Incremental (Options const& options);

    /**
     * Initializes the incremental bundler and sets required data.
     * - viewports: per-viewport colors, positions, track IDs, and initial
     *   focal length is required. A good initial camera pair is required
     *   so that initial tracks can be triangulated. Radial distortion and
     *   the camera pose is regularly updated.
     * - tracks: The tracks are triangulated and regularly updated.
     */
    void initialize (ViewportList* viewports, TrackList* tracks);
    /** Returns whether the incremental SfM has been initialized. */
    bool is_initialized (void) const;

    /** Returns a list of suitable view ID or emtpy list on failure. */
    void find_next_views (std::vector<int>* next_views);
    /** Incrementally adds the given view to the bundle. */
    bool reconstruct_next_view (int view_id);
    /** Triangulates tracks without 3D position and at least N views. */
    void triangulate_new_tracks (int min_num_views);
    /** Deletes tracks with a large reprojection error. */
    void invalidate_large_error_tracks (void);
    /** Runs bundle adjustment on both, structure and motion. */
    void bundle_adjustment_full (void);
    /** Runs bundle adjustment on a single camera without structure. */
    void bundle_adjustment_single_cam (int view_id);

    /** Transforms the bundle for numerical stability. */
    void normalize_scene (void);
    /** Computes a bundle from all viewports and reconstructed tracks. */
    mve::Bundle::Ptr create_bundle (void) const;

private:
    void bundle_adjustment_intern (int single_camera_ba);

private:
    Options opts;
    ViewportList* viewports;
    TrackList* tracks;
};

/* ------------------------ Implementation ------------------------ */

inline
Incremental::Options::Options (void)
    : track_error_threshold_factor(25.0)
    , new_track_error_threshold(10.0)
    , min_triangulation_angle(MATH_DEG2RAD(1.0))
    , ba_fixed_intrinsics(false)
    , ba_shared_intrinsics(false)
    , verbose_output(false)
{
}

inline
Incremental::Incremental (Options const& options)
    : opts(options)
    , viewports(NULL)
    , tracks(NULL)
{
}

inline bool
Incremental::is_initialized (void) const
{
    return this->viewports != NULL
        && this->tracks != NULL;
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

#endif /* SFM_BUNDLER_INCREMENTAL_HEADER */
