/*
 * High-level bundler component that computes features for every view.
 * Written by Simon Fuhrmann.
 */

#ifndef SFM_BUNDLER_FEATURES_HEADER
#define SFM_BUNDLER_FEATURES_HEADER

#include <string>
#include <limits>

#include "mve/image.h"
#include "mve/scene.h"
#include "sfm/feature_set.h"
#include "sfm/bundler_common.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

/**
 * Bundler Component: Computation of image features for an MVE scene.
 *
 * The component computes features for every view in the scene and stores
 * the features in the viewports. It also estimates the focal length from
 * the EXIF data stored in the views.
 */
class Features
{
public:
    struct Options
    {
        Options (void);

        /** The image for which features are to be computed. */
        std::string image_embedding;
        /** The embedding name in which EXIF tags are stored. */
        std::string exif_embedding;
        /** The maximum image size given in number of pixels. */
        int max_image_size;
        /** Feature set options. */
        FeatureSet::Options feature_options;
    };

public:
    explicit Features (Options const& options);

    /**
     * Computes features for all images in the scene.
     * Optionally, if the viewports argument is not NULL, the viewports
     * are initialized with descriptor data, positions and colors.
     */
    void compute (mve::Scene::Ptr scene, ViewportList* viewports);

private:
    void estimate_focal_length (mve::View::Ptr view, Viewport* viewport) const;
    void fallback_focal_length (mve::View::Ptr view, Viewport* viewport) const;

private:
    Options opts;
};

/* ------------------------ Implementation ------------------------ */

inline
Features::Options::Options (void)
    : image_embedding("original")
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
