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

/* Whether to use floating point or 8-bit descriptors for matching. */
#define DISCRETIZE_DESCRIPTORS 1

SFM_NAMESPACE_BEGIN

/**
 * The FeatureSet holds per-feature information for a single view, and
 * allows to transparently compute and match multiple feature types.
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
    FeatureSet (void);
    explicit FeatureSet (Options const& options);
    void set_options (Options const& options);

    /** Computes the features specified in the options. */
    void compute_features (mve::ByteImage::Ptr image);

    /** Matches all feature types yielding a single matching result. */
    void match (FeatureSet const& other, Matching::Result* result) const;

    /**
     * Matches the N lowest resolution features and returns the number of
     * matches. Can be used as a guess for full matchability. Useful values
     * are at most 3 matches for 500 features, or 2 matches with 300 features.
     */
    int match_lowres (FeatureSet const& other, int num_features) const;

    /** Clear descriptor data. */
    void clear_descriptors (void);

public:
    /** Image dimension used for feature computation. */
    int width, height;
    /** Per-feature image position. */
    std::vector<math::Vec2f> positions;
    /** Per-feature image color. */
    std::vector<math::Vec3uc> colors;

private:
    void compute_sift (mve::ByteImage::ConstPtr image);
    void compute_surf (mve::ByteImage::ConstPtr image);

private:
    Options opts;
    int num_sift_descriptors;
    int num_surf_descriptors;
#if DISCRETIZE_DESCRIPTORS
    util::AlignedMemory<unsigned short, 16> sift_descr;
    util::AlignedMemory<signed short, 16> surf_descr;
#else
    util::AlignedMemory<float, 16> sift_descr;
    util::AlignedMemory<float, 16> surf_descr;
#endif
};

/* ------------------------ Implementation ------------------------ */

inline
FeatureSet::Options::Options (void)
    : feature_types(FEATURE_SIFT)
{
    this->sift_matching_opts.lowe_ratio_threshold = 0.8f;
    this->sift_matching_opts.descriptor_length = 128;
    this->surf_matching_opts.lowe_ratio_threshold = 0.7f;
    this->surf_matching_opts.descriptor_length = 64;
}

inline
FeatureSet::FeatureSet (void)
    : num_sift_descriptors(0)
    , num_surf_descriptors(0)
{
}

inline
FeatureSet::FeatureSet (Options const& options)
    : opts(options)
    , num_sift_descriptors(0)
    , num_surf_descriptors(0)
{
}

inline void
FeatureSet::set_options (Options const& options)
{
    this->opts = options;
}

SFM_NAMESPACE_END

#endif /* SFM_FEATURE_SET_HEADER */
