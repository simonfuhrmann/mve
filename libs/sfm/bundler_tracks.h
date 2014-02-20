#ifndef SFM_BUNDLER_TRACKS_HEADER
#define SFM_BUNDLER_TRACKS_HEADER

#include "mve/scene.h"
#include "sfm/bundler_matching.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

/**
 * Bundler Component: Generation of tracks from pairwise matching result.
 *
 * As input this component requires all the pairwise matching results and
 * as well as the 2D coordinates for each match in the images.
 */
class Tracks
{
public:
    struct Options
    {
        Options (void);
    };

public:
    Tracks (Options const& options);

    /**
     * Computes viewport connectivity information by propagating track IDs.
     * Computation requires feature positions and colors in the viewports.
     * A color for each track is computed as the average color from features.
     * Viewports are equipped with per-feature track IDs.
     */
    void compute (PairwiseMatching const& matching,
        ViewportList* viewports, TrackList* tracks);

private:
    Options opts;
};

/* ---------------------------------------------------------------- */

mve::ByteImage::Ptr
visualize_track (Track const& track, ViewportList const& viewports,
    mve::Scene::Ptr scene, std::string const& image_embedding,
    PairwiseMatching const& matching);

/* ---------------------------------------------------------------- */

inline
Tracks::Options::Options (void)
{
}

inline
Tracks::Tracks (Options const& options)
    : opts(options)
{
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

#endif /* SFM_BUNDLER_TRACKS_HEADER */
