/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_BUNDLER_COMMON_HEADER
#define SFM_BUNDLER_COMMON_HEADER

#include <string>
#include <vector>

#include "math/vector.h"
#include "util/aligned_memory.h"
#include "mve/image.h"
#include "sfm/camera_pose.h"
#include "sfm/correspondence.h"
#include "sfm/feature_set.h"
#include "sfm/sift.h"
#include "sfm/surf.h"
#include "sfm/defines.h"

#define DESCR_SIGNATURE "MVE_DESCRIPTORS\n"
#define DESCR_SIGNATURE_LEN 16

#define MATCHING_SIGNATURE "MVE_MATCHING\n"
#define MATCHING_SIGNATURE_LEN 13

#define VIEWPORTS_SIGNATURE "MVE_VIEWPORTS\n"
#define VIEWPORTS_SIGNATURE_LEN 14

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

/* -------------------- Common Data Structures -------------------- */

/**
 * Per-viewport information.
 * Not all data is required for every step. It should be populated on demand
 * and cleaned as early as possible to keep memory consumption in bounds.
 */
struct Viewport
{
    Viewport (void);

    /** Initial focal length estimate for the image. */
    float focal_length;
    /** Radial distortion parameter. */
    float radial_distortion;

    /** Camera pose for the viewport. */
    CameraPose pose;

    /** The actual image data for debugging purposes. Usually NULL! */
    mve::ByteImage::Ptr image;
    /** Per-feature information. */
    FeatureSet features;
    /** Per-feature track ID, -1 if not part of a track. */
    std::vector<int> track_ids;
};

/** The list of all viewports considered for bundling. */
typedef std::vector<Viewport> ViewportList;

/* ------------------ Input/Output for Viewports ------------------ */

/**
 * Writes certain per-viewport data to file. Currently only data relevant
 * for bundling is written, i.e.
 * - image width and height,
 * - focal length and radial distortion,
 * - positions, colors and track IDs.
 */
void
save_viewports_data (ViewportList const& viewports,
    std::string const& filename);

/** Reads certain per-viewport data from file. */
void
load_viewports_data (std::string const& filename,
    ViewportList* viewports);

/* --------------- Data Structure for Feature Tracks -------------- */

/** References a 2D feature in a specific view. */
struct FeatureReference
{
    FeatureReference (int view_id, int feature_id);

    int view_id;
    int feature_id;
};

/** The list of all feature references inside a track. */
typedef std::vector<FeatureReference> FeatureReferenceList;

/** Representation of a feature track. */
struct Track
{
    bool is_valid (void) const;
    void invalidate (void);
    void remove_view (int view_id);

    math::Vec3f pos;
    math::Vec3uc color;
    FeatureReferenceList features;
};

/** The list of all tracks. */
typedef std::vector<Track> TrackList;

/* ------------- Data Structures for Feature Matching ------------- */

/** The matching result between two views. */
struct TwoViewMatching
{
    bool operator< (TwoViewMatching const& rhs) const;

    int view_1_id;
    int view_2_id;
    CorrespondenceIndices matches;
};

/** The matching result between several pairs of views. */
typedef std::vector<TwoViewMatching> PairwiseMatching;

/* -------------- Input/Output for Feature Matching --------------- */

void
save_pairwise_matching (PairwiseMatching const& matching,
    std::string const& filename);

void
load_pairwise_matching (std::string const& filename,
    PairwiseMatching* matching);

/* ---------------- Input/Output of the Pre-Bundle ---------------- */

/**
 * Saves the pre-bundle data to file, which records all viewport and
 * matching data necessary for incremental structure-from-motion.
 */
void
save_prebundle_to_file (ViewportList const& viewports,
    PairwiseMatching const& matching, std::string const& filename);

/**
 * Loads the pre-bundle data from file, initializing viewports and matching.
 */
void
load_prebundle_from_file (std::string const& filename,
    ViewportList* viewports, PairwiseMatching* matching);

/* ------------- (De-)Serialization of SIFT and SURF -------------- */

/*
 * The feature embedding has the following binary format:
 *
 * MVE_DESCRIPTORS\n
 * <int32:num_descriptors> <int32:image_width> <int32:image_height>
 * <float:x> <float:y> <float:scale> <float:orientation> <data>
 * <...>
 *
 * The first line is the file signature, the second line is the header.
 * The <data> field corresponds to the whole descriptor with either
 * 128 unsigned (SIFT) or 64 signed (SURF) floating point values.
 */

/** Conversion from SIFT descriptors to binary format. */
mve::ByteImage::Ptr
descriptors_to_embedding (Sift::Descriptors const& descriptors,
    int width, int height);

/** Conversion from binary format to SIFT descriptors. */
void
embedding_to_descriptors (mve::ByteImage::ConstPtr data,
    Sift::Descriptors* descriptors, int* width, int* height);

/** Conversion from SURF descriptors to binary format. */
mve::ByteImage::Ptr
descriptors_to_embedding (Surf::Descriptors const& descriptors,
    int width, int height);

/** Conversion from binary format to SURF descriptors. */
void
embedding_to_descriptors (mve::ByteImage::ConstPtr data,
    Surf::Descriptors* descriptors, int* width, int* height);

/* ------------------------ Implementation ------------------------ */

inline
FeatureReference::FeatureReference (int view_id, int feature_id)
    : view_id(view_id)
    , feature_id(feature_id)
{
}

inline bool
TwoViewMatching::operator< (TwoViewMatching const& rhs) const
{
    return this->view_1_id == rhs.view_1_id
        ? this->view_2_id < rhs.view_2_id
        : this->view_1_id < rhs.view_1_id;
}

inline
Viewport::Viewport (void)
    : focal_length(0.0f)
    , radial_distortion(0.0f)
{
}

inline bool
Track::is_valid (void) const
{
    return !std::isnan(this->pos[0]);
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

#endif /* SFM_BUNDLER_COMMON_HEADER */
