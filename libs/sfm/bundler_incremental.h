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

        /** ID of the first initial pair view. */
        int initial_pair_view_1_id;
        /** ID of the second initial pair view. */
        int initial_pair_view_2_id;
        /** Options for computing initial pair fundamental matrix. */
        RansacFundamental::Options fundamental_opts;
        /** Options for computing the pose from 2D-3D correspondences. */
        RansacPose::Options pose_opts;

        /** Produce status messages on the console. */
        bool verbose_output;
    };

public:
    Incremental (Options const& options);

    /**
     * Computes a bundle from all viewports and reconstructed tracks.
     * Requires per-view colors, positions, track IDs and focal length.
     * Positions of the tracks are modified during reconstruction.
     */
    mve::Bundle::Ptr compute (ViewportList const& viewports, TrackList* tracks);

private:
    void compute_pose_for_initial_pair (int view_1_id, int view_2_id,
        ViewportList const& viewports, TrackList const& tracks);
    void triangulate_new_tracks (ViewportList const& viewports,
        TrackList* tracks);
    void bundle_adjustment (ViewportList const& viewports,
        TrackList* tracks);
    int find_next_view (TrackList const& tracks);
    void add_next_view (int view_id, ViewportList const& viewports,
        TrackList const& tracks);

private:
    std::vector<CameraPose> cameras;
    Options opts;
};

/* ------------------------ Implementation ------------------------ */

inline
Incremental::Options::Options (void)
    : initial_pair_view_1_id(-1)
    , initial_pair_view_2_id(-1)
{
}

inline
Incremental::Incremental (Options const& options)
    : opts(options)
{
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

#endif /* SFM_BUNDLER_INCREMENTAL_HEADER */
