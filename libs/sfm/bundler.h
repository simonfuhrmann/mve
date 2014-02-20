/*
 * High-level tool to create a bundle.
 * Written by Simon Fuhrmann.
 *
 * TODO
 * - Add thread pool?
 */

#ifndef SFM_BUNDLE_HEADER
#define SFM_BUNDLE_HEADER

#include <vector>
#include <map>
#include <set>

#include "util/aligned_memory.h"
#include "mve/image.h"
#include "sfm/defines.h"
#include "sfm/correspondence.h"
#include "sfm/matching.h"
#include "sfm/ransac_fundamental.h"
#include "sfm/ransac_pose.h"
#include "sfm/sift.h"
#include "sfm/surf.h"
#include "sfm/fundamental.h"

SFM_NAMESPACE_BEGIN

// TODO Baustelle

class Bundler
{
public:
    struct Options
    {
        Sift::Options sift_options;
        Matching::Options sift_matching_options;
        Surf::Options surf_options;
        Matching::Options surf_matching_options;
        RansacFundamental::Options ransac_fundamental_options;
        RansacPose::Options ransac_pose_options;

        int max_image_size;
        bool use_sift_features; // TODO
        bool use_surf_features; // TODO
    };

    /** Represents a 2D point in the image. */
    struct Feature2D
    {
        float pos[2];
        float color[3];
        int view_id;
        int descriptor_id;
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
        /** The input focal length of the image. */
        double focal_length;

        /** Tightly packed data for the descriptors. */
        util::AlignedMemory<float, 16> descr_data;
        /** Per-descriptor information. */
        std::vector<Feature2DRef> descr_info;

        /** The pose for that view. */
        CameraPose pose;
    };

    /** Represents a 3D point in space: a feature track. */
    struct Feature3D
    {
        double pos[3];
        float color[3];
        std::vector<int> feature2d_ids;
    };

    /** Pair-wise image information. */
    struct ImagePair : public std::pair<int, int>
    {
        ImagePair(int a, int b) : std::pair<int, int>(a, b) {}
        FundamentalMatrix fundamental;
        CorrespondenceIndices indices;
    };

public:
    explicit Bundler (Options const& options);

    /**
     * Adds an image with known focal length to the bundler. The focal
     * length is given in normalized format, e.g. for a photo taken
     * at 40mm with a 35mm sensor, the focal length is 40/35.
     */
    void add_image (mve::ByteImage::ConstPtr image, double focal_length);

    /**
     * Temporary implementation.
     */
    void create_bundle (void);

protected:
    void add_initial_pair_to_bundle (void);
    void add_next_view_to_bundle (void);

    void compute_fundamental_for_pair (ImagePair* image_pair);
    void compute_pose_from_fundamental (ImagePair const& image_pair);
    void compute_pose_from_2d3d (int view_id, Correspondences2D3D const& corresp);
    void compute_sift_descriptors (std::size_t view_id);
    void triangulate_initial_pair (ImagePair const& image_pair);

    ImagePair select_initial_pair (void);
    int select_next_view (void);

    // Debug stuff.
    void save_tracks_to_mesh (std::string const& filename);
    void generate_scene_from_bundle (std::string const& directory);

protected:
    Options options;
    std::set<std::size_t> remaining;
    std::vector<Viewport> viewports;
    std::vector<Feature2D> features;
    std::vector<Feature3D> tracks;
};

/* ---------------------------------------------------------------- */

inline
Bundler::Bundler (Options const& options)
    : options(options)
{
}

SFM_NAMESPACE_END

#endif /* SFM_BUNDLE_HEADER */
