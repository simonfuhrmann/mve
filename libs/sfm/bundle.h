/*
 * High-level view on creating a bundle.
 * Written by Simon Fuhrmann.
 */
#ifndef SFM_BUNDLE_HEADER
#define SFM_BUNDLE_HEADER

#include "defines.h"
#include "matching.h"
#include "poseransac.h"

SFM_NAMESPACE_BEGIN

// TODO Baustelle

class Bundle
{
    struct Options
    {
        // SIFT options
        MatchingOptions matching_options;
        PoseRansac2D2D::Options ransac2d2d_options;
        PoseRansac2D3D::Options ransac2d3d_options;
    };

public:
    explicit Bundle (Options const& options);
};

SFM_NAMESPACE_END

#endif /* SFM_BUNDLE_HEADER */
