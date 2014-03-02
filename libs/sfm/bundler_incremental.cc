#include "sfm/bundler_incremental.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

mve::Bundle::Ptr
Incremental::compute (ViewportList const& viewports,
    TrackList const& tracks)
{
    /*
     * Overall process:
     * - Reconstruct fundamental for initial pair
     * - Extract pose for initial pair
     * - Run bundle adjustment
     * - Repeat
     *   - Find next view for reconstruction
     *   - Recover next camera pose
     *   - Reconstruct new tracks
     *   - Run bundle adjustment
     *
     */


    this->cameras.clear();
    this->cameras.resize(viewports.size());

    /* Reconstruct fundamental matrix for initial pair. */
    FundamentalMatrix fundamental;
    compute_initial_pair_fundamental(this->opts.initial_pair_view_1_id,
        this->opts.initial_pair_view_2_id, viewports, tracks, &fundamental);


    // TODO
    return mve::Bundle::Ptr();
}

/* ---------------------------------------------------------------- */

void
Incremental::compute_initial_pair_fundamental (int view_1_id, int view_2_id,
    ViewportList const& viewports, TrackList const& tracks,
    FundamentalMatrix* result)
{
    Viewport const& view_1 = viewports[view_1_id];
    Viewport const& view_2 = viewports[view_2_id];

    /*
     * Interate over all tracks and find correspondences between the initial
     * pair. Use the correspondences to compute the fundamental matrix.
     */
    // TODO

}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END
