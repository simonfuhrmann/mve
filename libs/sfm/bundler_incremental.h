/*
 * Bundler component that incrementally adds views to reconstruct a scene.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_BUNDLER_INCREMENTAL_HEADER
#define SFM_BUNDLER_INCREMENTAL_HEADER

#include "mve/bundle.h"
#include "sfm/fundamental.h"
#include "sfm/ransac_fundamental.h"
#include "sfm/ransac_pose.h"
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
        RansacPose::Options pose_opts;
        /** Options for computing pose from 2D-3D correspondences. */
        RansacPoseP3P::Options pose_p3p_opts;
        /** Threshold for large error tracks. */
        double track_error_threshold;
        /** Produce status messages on the console. */
        bool verbose_output;
    };

public:
    Incremental (Options const& options);

    /**
     * Initializes the incremental bundler and sets required data.
     *
     * This componenent requires per-viewport colors, positions, track IDs,
     * the focal length as well as image width and height. During
     * reconstruction the positions of the tracks are modified.
     */
    void initialize (ViewportList* viewports, TrackList* tracks);

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
    /** Runs bundle adjustment on both, structure and motion. */
    void bundle_adjustment_full (void);
    /** Runs bundle adjustment on a single camera without structure. */
    void bundle_adjustment_single_cam (int view_id);
    /** Computes a bundle from all viewports and reconstructed tracks. */
    mve::Bundle::Ptr create_bundle (void) const;

    /** Deletes tracks with a large reprojection error. */
    void delete_large_error_tracks (void);

private:
    void bundle_adjustment_intern (int single_camera_ba);
    void delete_track (int track_id);

private:
    Options opts;
    ViewportList* viewports;
    TrackList* tracks;
    std::vector<CameraPose> cameras;
};

/* ------------------------ Implementation ------------------------ */

inline
Incremental::Options::Options (void)
    : track_error_threshold(10.0)
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
