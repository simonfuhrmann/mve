/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_BUNDLER_FEATURES_HEADER
#define SFM_BUNDLER_FEATURES_HEADER

#include <string>
#include <limits>

#include "mve/scene.h"
#include "sfm/feature_set.h"
#include "sfm/bundler_common.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

/**
 * Bundler Component: Computes image features for every view in the scene
 * and stores the features in the viewports.
 */
class Features
{
public:
    struct Options
    {
        Options (void);

        /** The image for which features are to be computed. */
        std::string image_embedding;
        /** The maximum image size given in number of pixels. */
        int max_image_size;
        /** Feature set options. */
        FeatureSet::Options feature_options;
    };

public:
    explicit Features (Options const& options);

    /** Computes features for all images in the scene. */
    void compute (mve::Scene::Ptr scene, ViewportList* viewports);

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
