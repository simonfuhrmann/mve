/*
 * Bundler component that incrementally adds views to reconstruct a scene.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_BUNDLER_INCREMENTAL_HEADER
#define SFM_BUNDLER_INCREMENTAL_HEADER

#include "mve/bundle.h"
#include "sfm/fundamental.h"
#include "sfm/ransac_fundamental.h"
#include "sfm/ransac_pose_p3p.h"
#include "sfm/bundler_common.h"
#include "sfm/pose.h"
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

        /** Options for computing initial pair fundamental matrix. */
        RansacFundamental::Options fundamental_opts;
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
        /** Produce status messages on the console. */
        bool verbose_output;
    };

public:
    explicit Incremental (Options const& options);

    /**
     * Initializes the incremental bundler and sets required data.
     *
     * This componenent requires per-viewport colors, positions, track IDs,
     * the focal length as well as image width and height. During
     * reconstruction the positions of the tracks are modified.
     */
    void initialize (ViewportList* viewports, TrackList* tracks);

    /** Returns whether the incremental SfM has been initialized. */
    bool is_initialized (void) const;

    /** Reconstructs the initial pose between the two given views. */
    void reconstruct_initial_pair (int view_1_id, int view_2_id);
    /** Returns a suitable next view ID or -1 on failure. */
    int find_next_view (void) const;
    /** Returns a list of suitable view ID or emtpy list on failure. */
    void find_next_views (std::vector<int>* next_views);
    /** Incrementally adds the given view to the bundle. */
    bool reconstruct_next_view (int view_id);
    /** Triangulates tracks without 3D position and at least 2 views. */
    void triangulate_new_tracks (void);
    /** Deletes tracks with a large reprojection error. */
    void invalidate_large_error_tracks (void);
    /** Runs bundle adjustment on both, structure and motion. */
    void bundle_adjustment_full (void);
    /** Runs bundle adjustment on a single camera without structure. */
    void bundle_adjustment_single_cam (int view_id);
    /** Transforms the bundle for numerical stability. */
    void normalize_scene (void);

    /** Returns the list of computed camera poses. */
    std::vector<CameraPose> const& get_cameras (void) const;
    /** Computes a bundle from all viewports and reconstructed tracks. */
    mve::Bundle::Ptr create_bundle (void) const;

private:
    void bundle_adjustment_intern (int single_camera_ba);

private:
    Options opts;
    ViewportList* viewports;
    TrackList* tracks;
    std::vector<CameraPose> cameras;
};

/* ------------------------ Implementation ------------------------ */

inline
Incremental::Options::Options (void)
    : track_error_threshold_factor(25.0)
    , new_track_error_threshold(10.0)
    , min_triangulation_angle(MATH_DEG2RAD(1.0))
    , ba_fixed_intrinsics(false)
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

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

#endif /* SFM_BUNDLER_INCREMENTAL_HEADER */
