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
#include "sfm/sift.h"
#include "sfm/surf.h"
#include "sfm/matching.h"
#include "sfm/bundler_common.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

/**
 * Bundler Component: Computation of image features for an MVE scene.
 *
 * The component computes for every view in the scene the features for the
 * given image embedding. If features already exist as embedding, they are
 * not recomputed. Otherwise features are computed and stored. If no feature
 * embedding is given, only viewports are initialized.
 */
class Features
{
public:
    struct Options
    {
        Options (void);

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
    };

    enum FeatureType
    {
        SIFT_FEATURES,
        SURF_FEATURES
    };

public:
    Features (Options const& options);

    /**
     * Computes features for all images in the scene.
     * Optionally, if the viewports argument is not NULL, the viewports
     * are initialized with descriptor data, positions and colors.
     */
    void compute (mve::Scene::Ptr scene, FeatureType type,
        ViewportList* viewports = NULL);

private:
    template <typename FEATURE>
    void compute (mve::View::Ptr view, Viewport* viewport) const;

    template <typename FEATURE>
    FEATURE construct (void) const;

    template <typename FEATURE>
    int descriptor_length (void) const;

private:
    Options opts;
};

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
