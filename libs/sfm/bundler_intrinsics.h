/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_BUNDLER_INTRINSICS_HEADER
#define SFM_BUNDLER_INTRINSICS_HEADER

#include <string>
#include <map>

#include "mve/scene.h"
#include "sfm/bundler_common.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

/**
 * Bundler Component: Obtains initial intrinsic paramters for the viewports
 * from either the EXIF embeddings or from the MVE views.
 */
class Intrinsics
{
public:
    /** Data source for camera intrinsic estimates. */
    enum Source
    {
        FROM_EXIF,
        FROM_VIEWS
    };

    struct Options
    {
        Options (void);

        /** Data source for camera intrinsic estimates. */
        Source intrinsics_source;

        /** The embedding name in which EXIF tags are stored. */
        std::string exif_embedding;
    };

public:
    explicit Intrinsics (Options const& options);

    /** Obtains camera intrinsics for all viewports. */
    void compute (mve::Scene::Ptr scene, ViewportList* viewports);

private:
    void init_from_exif (mve::View::Ptr view, Viewport* viewport);
    void init_from_views (mve::View::Ptr view, Viewport* viewport);
    void fallback_focal_length (Viewport* viewport);

private:
    Options opts;
    std::map<std::string, int> unknown_cameras;
};

/* ------------------------ Implementation ------------------------ */

inline
Intrinsics::Options::Options (void)
    : intrinsics_source(FROM_EXIF)
    , exif_embedding("exif")
{
}

inline
Intrinsics::Intrinsics (Options const& options)
    : opts(options)
{
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END

#endif  /* SFM_BUNDLER_INTRINSICS_HEADER */
