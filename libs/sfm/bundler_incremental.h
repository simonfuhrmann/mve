/*
 * Bundler component that incrementally adds views to reconstruct a scene.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_BUNDLER_INCREMENTAL_HEADER
#define SFM_BUNDLER_INCREMENTAL_HEADER

#include "mve/bundle.h"
#include "sfm/fundamental.h"
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

        int initial_pair_view_1_id;
        int initial_pair_view_2_id;
    };

public:
    Incremental (Options const& options);
    mve::Bundle::Ptr compute (ViewportList const& viewports,
        TrackList const& tracks);

private:
    void compute_initial_pair_fundamental (int view_1_id, int view_2_id,
        ViewportList const& viewports, TrackList const& tracks,
        FundamentalMatrix* result);

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
