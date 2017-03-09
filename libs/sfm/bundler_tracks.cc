/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>
#include <set>

#include "mve/image_tools.h"
#include "mve/image_drawing.h"
#include "sfm/bundler_tracks.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

/*
 * Merges tracks and updates viewports accordingly.
 */
void
unify_tracks(int view1_tid, int view2_tid,
    TrackList* tracks, ViewportList* viewports)
{
    /* Unify in larger track. */
    if (tracks->at(view1_tid).features.size()
        < tracks->at(view2_tid).features.size())
        std::swap(view1_tid, view2_tid);

    Track& track1 = tracks->at(view1_tid);
    Track& track2 = tracks->at(view2_tid);

    for (std::size_t k = 0; k < track2.features.size(); ++k)
    {
        int const view_id = track2.features[k].view_id;
        int const feat_id = track2.features[k].feature_id;
        viewports->at(view_id).track_ids[feat_id] = view1_tid;
    }
    track1.features.insert(track1.features.end(),
        track2.features.begin(), track2.features.end());
    /* Free old track's memory. clear() does not work. */
    track2.features = FeatureReferenceList();
}

/* ---------------------------------------------------------------- */

void
Tracks::compute (PairwiseMatching const& matching,
    ViewportList* viewports, TrackList* tracks)
{
    /* Initialize per-viewport track IDs. */
    for (std::size_t i = 0; i < viewports->size(); ++i)
    {
        Viewport& viewport = viewports->at(i);
        viewport.track_ids.resize(viewport.features.positions.size(), -1);
    }

    /* Propagate track IDs. */
    if (this->opts.verbose_output)
        std::cout << "Propagating track IDs..." << std::endl;

    /* Iterate over all pairwise matchings and create tracks. */
    tracks->clear();
    for (std::size_t i = 0; i < matching.size(); ++i)
    {
        TwoViewMatching const& tvm = matching[i];
        Viewport& viewport1 = viewports->at(tvm.view_1_id);
        Viewport& viewport2 = viewports->at(tvm.view_2_id);

        /* Iterate over matches for a pair. */
        for (std::size_t j = 0; j < tvm.matches.size(); ++j)
        {
            CorrespondenceIndex idx = tvm.matches[j];
            int const view1_tid = viewport1.track_ids[idx.first];
            int const view2_tid = viewport2.track_ids[idx.second];
            if (view1_tid == -1 && view2_tid == -1)
            {
                /* No track ID associated with the match. Create track. */
                viewport1.track_ids[idx.first] = tracks->size();
                viewport2.track_ids[idx.second] = tracks->size();
                tracks->push_back(Track());
                tracks->back().features.push_back(
                    FeatureReference(tvm.view_1_id, idx.first));
                tracks->back().features.push_back(
                    FeatureReference(tvm.view_2_id, idx.second));
            }
            else if (view1_tid == -1 && view2_tid != -1)
            {
                /* Propagate track ID from first to second view. */
                viewport1.track_ids[idx.first] = view2_tid;
                tracks->at(view2_tid).features.push_back(
                    FeatureReference(tvm.view_1_id, idx.first));
            }
            else if (view1_tid != -1 && view2_tid == -1)
            {
                /* Propagate track ID from second to first view. */
                viewport2.track_ids[idx.second] = view1_tid;
                tracks->at(view1_tid).features.push_back(
                    FeatureReference(tvm.view_2_id, idx.second));
            }
            else if (view1_tid == view2_tid)
            {
                /* Track ID already propagated. */
            }
            else
            {
                /*
                 * A track ID is already associated with both ends of a match,
                 * however, is not consistent. Unify tracks.
                 */
                unify_tracks(view1_tid, view2_tid, tracks, viewports);
            }
        }
    }

    /* Find and remove invalid tracks or tracks with conflicts. */
    if (this->opts.verbose_output)
        std::cout << "Removing tracks with conflicts..." << std::flush;
    std::size_t const num_invalid_tracks
        = this->remove_invalid_tracks(viewports, tracks);
    if (this->opts.verbose_output)
        std::cout << " deleted " << num_invalid_tracks
            << " tracks." << std::endl;

    /* Compute color for every track. */
    if (this->opts.verbose_output)
        std::cout << "Colorizing tracks..." << std::endl;
    for (std::size_t i = 0; i < tracks->size(); ++i)
    {
        Track& track = tracks->at(i);
        math::Vec4f color(0.0f, 0.0f, 0.0f, 0.0f);
        for (std::size_t j = 0; j < track.features.size(); ++j)
        {
            FeatureReference const& ref = track.features[j];
            FeatureSet const& features = viewports->at(ref.view_id).features;
            math::Vec3f const feature_color(features.colors[ref.feature_id]);
            color += math::Vec4f(feature_color, 1.0f);
        }
        track.color[0] = static_cast<uint8_t>(color[0] / color[3] + 0.5f);
        track.color[1] = static_cast<uint8_t>(color[1] / color[3] + 0.5f);
        track.color[2] = static_cast<uint8_t>(color[2] / color[3] + 0.5f);
    }
}

/* ---------------------------------------------------------------- */

int
Tracks::remove_invalid_tracks (ViewportList* viewports, TrackList* tracks)
{
    /*
     * Detect invalid tracks where a track contains no features, or
     * multiple features from a single view.
     */
    std::vector<bool> delete_tracks(tracks->size());
    int num_invalid_tracks = 0;
    for (std::size_t i = 0; i < tracks->size(); ++i)
    {
        if (tracks->at(i).features.empty())
        {
            delete_tracks[i] = true;
            continue;
        }

        std::set<int> view_ids;
        for (std::size_t j = 0; j < tracks->at(i).features.size(); ++j)
        {
            FeatureReference const& ref = tracks->at(i).features[j];
            if (view_ids.insert(ref.view_id).second == false)
            {
                num_invalid_tracks += 1;
                delete_tracks[i] = true;
                break;
            }
        }
    }

    /* Create a mapping from old to new track IDs. */
    std::vector<int> id_mapping(delete_tracks.size(), -1);
    int valid_track_counter = 0;
    for (std::size_t i = 0; i < delete_tracks.size(); ++i)
    {
        if (delete_tracks[i])
            continue;
        id_mapping[i] = valid_track_counter;
        valid_track_counter += 1;
    }

    /* Fix track IDs stored in the viewports. */
    for (std::size_t i = 0; i < viewports->size(); ++i)
    {
        std::vector<int>& track_ids = viewports->at(i).track_ids;
        for (std::size_t j = 0; j < track_ids.size(); ++j)
            if (track_ids[j] >= 0)
                track_ids[j] = id_mapping[track_ids[j]];
    }

    /* Clean the tracks from the vector. */
    math::algo::vector_clean(delete_tracks, tracks);

    return num_invalid_tracks;
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END
