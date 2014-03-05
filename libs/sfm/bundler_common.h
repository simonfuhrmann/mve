/*
 * Common data structures and functions for the bundler tool.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_BUNDLER_COMMON_HEADER
#define SFM_BUNDLER_COMMON_HEADER

#include <string>
#include <vector>

#include "math/vector.h"
#include "util/aligned_memory.h"
#include "mve/image.h"
#include "sfm/sift.h"
#include "sfm/surf.h"
#include "sfm/correspondence.h"
#include "sfm/defines.h"

#define DESCR_SIGNATURE "MVE_DESCRIPTORS\n"
#define DESCR_SIGNATURE_LEN 16

#define MATCHING_SIGNATURE "MVE_MATCHING\n"
#define MATCHING_SIGNATURE_LEN 13

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

    /** Image dimension used for feature computation. */
    int width, height;
    /** Estimated focal length of the image. */
    float focal_length;
    /** Radial distortion parameter. */
    float radial_distortion;
    /** Tightly packed data for the descriptors. */
    util::AlignedMemory<float, 16> descr_data;
    /** Per-feature image position. */
    std::vector<math::Vec2f> positions;
    /** Per-feature image color. */
    std::vector<math::Vec3uc> colors;
    /** Per-feature track ID, -1 if not part of a track. */
    std::vector<int> track_ids;
};

/** The list of all viewports considered for bundling. */
typedef std::vector<Viewport> ViewportList;

/* --------------- Data Structure for Feature Tracks -------------- */

/** References a 2D feature in a specific view. */
struct FeatureReference
{
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
Viewport::Viewport (void)
    : width(0)
    , height(0)
    , focal_length(0.0f)
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
