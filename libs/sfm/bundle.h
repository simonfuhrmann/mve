/*
 * High-level tool to create a bundle.
 * Written by Simon Fuhrmann.
 */
#ifndef SFM_BUNDLE_HEADER
#define SFM_BUNDLE_HEADER

#include <vector>
#include <map>

#include "util/alignedmemory.h"
#include "mve/image.h"
#include "sfm/defines.h"
#include "sfm/matching.h"
#include "sfm/poseransac.h"
#include "sfm/sift.h"
#include "sfm/surf.h"

SFM_NAMESPACE_BEGIN

// TODO Baustelle

class Bundle
{
public:
    struct Options
    {
        Surf::Options surf_options;
        Sift::Options sift_options;
        MatchingOptions matching_options;
        PoseRansac2D2D::Options ransac2d2d_options;
        PoseRansac2D3D::Options ransac2d3d_options;

        int max_image_size;
    };

    /** Represents a 2D point in the image. */
    struct Feature2D
    {
        float pos[2];
        float color[3];
        int view_id;
        int feature3d_id;
    };

    /** A reference to a Feature2D. ID can be -1 (not part of a track). */
    struct Feature2DRef
    {
        float pos[2];
        int feature2d_id;
    };

    /** Working data per viewport. */
    struct Viewport
    {
        /** The input image data. */
        mve::ByteImage::ConstPtr image;
        /** Tightly packed data for the descriptors. */
        util::AlignedMemory<float, 16> descr_data;
        /** Per-descriptor information. */
        std::vector<Feature2DRef> descr_info;
    };

    /* Represents a 3D point in space: a feature track. */
    struct Feature3D
    {
        double pos[3];
        std::vector<int> feature2d_ids;
    };

    struct ImagePair : public std::pair<int, int>
    {
    };

public:
    explicit Bundle (Options const& options);

    void add_image (mve::ByteImage::ConstPtr image);
    void create_bundle (void);

protected:
    ImagePair select_initial_pair (void);
    int select_next_view (void);
    void compute_descriptors (int view_id);

private:
    // Thread Pool?
};

SFM_NAMESPACE_END

#endif /* SFM_BUNDLE_HEADER */
