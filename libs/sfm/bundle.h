/*
 * High-level view on creating a bundle.
 * Written by Simon Fuhrmann.
 */
#ifndef SFM_BUNDLE_HEADER
#define SFM_BUNDLE_HEADER

#include "sfm/defines.h"
#include "sfm/matching.h"
#include "sfm/poseransac.h"
#include "sfm/sift.h"

SFM_NAMESPACE_BEGIN

// TODO Baustelle

class Bundle
{
    struct Options
    {
        Sift::Options sift_options;
        MatchingOptions matching_options;
        PoseRansac2D2D::Options ransac2d2d_options;
        PoseRansac2D3D::Options ransac2d3d_options;
    };

public:
    explicit Bundle (Options const& options);
};

SFM_NAMESPACE_END

#endif /* SFM_BUNDLE_HEADER */
