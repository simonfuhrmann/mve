/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

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
 * As input this component requires all the pairwise matching results.
 * Additionally, to color the tracks, a color for each feature must be set.
 */
class Tracks
{
public:
    struct Options
    {
        Options (void);

        /** Produce status messages on the console. */
        bool verbose_output;
    };

public:
    explicit Tracks (Options const& options);

    /**
     * Computes viewport connectivity information by propagating track IDs.
     * Computation requires feature positions and colors in the viewports.
     * A color for each track is computed as the average color from features.
     * Per-feature track IDs are added to the viewports.
     */
    void compute (PairwiseMatching const& matching,
        ViewportList* viewports, TrackList* tracks);

private:
    int remove_invalid_tracks (ViewportList* viewports, TrackList* tracks);

private:
    Options opts;
};

/* ------------------------ Implementation ------------------------ */

inline
Tracks::Options::Options (void)
    : verbose_output(false)
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
