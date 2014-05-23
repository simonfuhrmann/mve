/*
 * Detection and matching of multiple feature types.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_FEATURE_SET_HEADER
#define SFM_FEATURE_SET_HEADER

#include <vector>

#include "math/vector.h"
#include "util/aligned_memory.h"
#include "sfm/sift.h"
#include "sfm/surf.h"
#include "sfm/matching.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN

/**
 * TODO
 */
class FeatureSet
{
public:
    /** Bitmask with feature types. */
    enum FeatureTypes
    {
        FEATURE_SIFT = 1 << 0,
        FEATURE_SURF = 1 << 1,
        FEATURE_ALL = 0xFF
    };

    /** Options for feature detection and matching. */
    struct Options
    {
        Options (void);

        FeatureTypes feature_types;
        Sift::Options sift_opts;
        Surf::Options surf_opts;
        Matching::Options sift_matching_opts;
        Matching::Options surf_matching_opts;
    };

public:
    FeatureSet (Options const& options);

    /** Computes the features specified in the options. */
    void compute_features (mve::ByteImage::Ptr image);
    /** Matches all features yielding a single matching result. */
    void match (FeatureSet const& other, Matching::Result* result);

public:
    /** Per-feature image position. */
    std::vector<math::Vec2f> pos;
    /** Per-feature image color. */
    std::vector<math::Vec3uc> color;
    /** Per-feature track ID, -1 if not part of a track. */
    std::vector<int> track_ids;

private:
    void compute_sift (mve::ByteImage::ConstPtr image);
    void compute_surf (mve::ByteImage::ConstPtr image);

private:
    Options opts;
    int num_sift_descriptors;
    int num_surf_descriptors;
    util::AlignedMemory<float, 16> sift_descr;
    util::AlignedMemory<float, 16> surf_descr;
};

/* ------------------------ Implementation ------------------------ */

inline
FeatureSet::Options::Options (void)
    : feature_types(FEATURE_SIFT)
{
}

inline
FeatureSet::FeatureSet (Options const& options)
    : opts(options)
    , num_sift_descriptors(0)
    , num_surf_descriptors(0)
{
}

SFM_NAMESPACE_END

#endif /* SFM_FEATURE_SET_HEADER */
