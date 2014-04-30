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

    void reconstruct_initial_pair (int view_1_id, int view_2_id);
    int find_next_view (void) const;
    void reconstruct_next_view (int view_id);
    void triangulate_new_tracks (void);
    void bundle_adjustment_full (void);
    void bundle_adjustment_single_cam (int view_id);

    /**
     * Computes a bundle from all viewports and reconstructed tracks.
     */
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
