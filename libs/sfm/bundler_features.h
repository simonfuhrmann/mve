#ifndef SFM_BUNDLER_FEATURES_HEADER
#define SFM_BUNDLER_FEATURES_HEADER

#include <string>
#include <limits>

#include "mve/image.h"
#include "mve/scene.h"
#include "sfm/sift.h"
#include "sfm/surf.h"
#include "sfm/matching.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

/**
 * Bundler Component: Computation of image features for an MVE scene.
 *
 * For every view in the scene the componenet computes features for a given
 * embedding name for which features have not already been computed. If
 * features already exist, the view is skipped. The computed image descriptors
 * are then stored as embedding in the view. The resulting embedding has the
 * following binary format (signature, header, then all descriptors):
 *
 * MVE_DESCRIPTORS\n
 * <int32:num_descriptors> <int32:image_width> <int32:image_height>
 * <float:x> <float:y> <float:scale> <float:orientation> <data>
 * <...>
 *
 * The <data> field corresponds to the whole descriptor with either
 * 128 unsigned (SIFT) or 64 signed (SURF) floating point values.
 */
class Features
{
public:
    struct Options
    {
        /** The image for which features are to be computed. */
        std::string image_embedding;
        /** The embedding name for saving the features. */
        std::string feature_embedding;
        /** Do not skip views with existing features embedding. */
        bool force_recompute;
        /** Do not save MVE views. */
        bool skip_saving_views;
        /** The maximum image size given in number of pixels. */
        int max_image_size;

        /** Options for SIFT. */
        Sift::Options sift_options;
        /** Options for SURF. */
        Surf::Options surf_options;

        Options (void);
    };

    enum FeatureType
    {
        SIFT_FEATURES,
        SURF_FEATURES
    };

public:
    Features (Options const& options);
    void compute (mve::Scene::Ptr scene, FeatureType type);

private:
    Options opts;
};

/* ---------------------------------------------------------------- */

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

/* ---------------------------------------------------------------- */

inline
Features::Options::Options (void)
    : image_embedding("original")
    , feature_embedding("original-descr")
    , force_recompute(false)
    , skip_saving_views(false)
    , max_image_size(std::numeric_limits<int>::max())
{
}

inline
Features::Features (Options const& options)
    : opts(options)
{
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

#endif /* SFM_BUNDLER_FEATURES_HEADER */
