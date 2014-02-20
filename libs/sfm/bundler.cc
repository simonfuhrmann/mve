/*
 * High-level tool to create a bundle.
 * Written by Simon Fuhrmann.
 *
 * The tool has three stages:
 * 1. Feature detection
 * 2. Feature matching
 * 3. Bundling
 *
 * 1. Strategy for feature detection
 * - Do feature computation in parallel for all views.
 * - Where to store per-view features, in view or in extra file?
 *   - In view: Requires to save ALL views again
 *   - In extra file: Tedious file handling
 * - Feature Statistic
 *   - Matching with 3MP results in 10k features
 *   - Mathcing with 1MP results in 4k features
 *
 * 2. Strategy for feature matching
 * - Exhaustive matching (complexity of n^2 for n views)
 * - Memory consumption of 1k views with 10k features each:
 *   128 * sizeof(float) * 10000 * 1000 = 4882 MB -> Too large
 *   128 * sizeof(uint8) * 10000 * 1000 = 1220 MB -> Large but OK
 * - Keep all in memory or load subsets of views from file?
 * - How and where to store matching? Memory for one huge file:
 * - Matching statistics:
 *   - Matching between good views of 10k features: 2.5k matches
 *   - Mathcing between good views of 4k features: 800 matches
 *
 * 3. Strategy for bundling
 */

#include <iostream>
#include <algorithm>
#include <vector>

#include "util/timer.h"
#include "util/file_system.h"
#include "mve/view.h"
#include "mve/mesh.h"
#include "mve/mesh_tools.h"
#include "mve/mesh_io.h"
#include "mve/bundle.h"
#include "mve/bundle_io.h"
#include "mve/image.h"
#include "mve/image_io.h"
#include "mve/image_tools.h"
#include "sfm/bundler.h"
#include "sfm/triangulate.h"
#include "sfm/visualizer.h"

SFM_NAMESPACE_BEGIN

void
Bundler::add_image (mve::ByteImage::ConstPtr image, double focal_length)
{
    this->viewports.push_back(Viewport());
    this->viewports.back().image = image;
    this->viewports.back().focal_length = focal_length;

    // TODO: Downscaling of the input image
}

void
Bundler::create_bundle (void)
{
    /* Mark all viewports as remaining (to be bundled). */
    for (std::size_t i = 0; i < this->viewports.size(); ++i)
    {
        mve::ByteImage::ConstPtr image = this->viewports[i].image;
        std::cout << "Bundler: Adding image ID " << i
            << " (" << image->width() << "x" << image->height() << ")"
            << std::endl;
        this->remaining.insert(i);
    }

    /* Bundle initial pair. */
    this->add_initial_pair_to_bundle();

    /* DEBUG: Save initial tracks to mesh. */
    std::cout << "Bundle: Saving tracks after initial pair..." << std::endl;
    this->save_tracks_to_mesh("/tmp/initialpair.ply");

    /* DEBUG: Create a MVE scene from the bundle. */
    //std::cout << "Bundler: Generating output scene..." << std::endl;
    //generate_scene_from_bundle("/tmp/bundler_scene/");

    /* Incrementally bundle remaining views. */
    while (!this->remaining.empty())
        this->add_next_view_to_bundle();

    /* DEBUG: Create a MVE scene from the bundle. */
    std::cout << "Bundler: Generating output scene..." << std::endl;
    generate_scene_from_bundle("/tmp/bundler_scene/");
}

void
Bundler::add_initial_pair_to_bundle (void)
{
    ImagePair initial_pair = this->select_initial_pair();
    this->remaining.erase(initial_pair.first);
    this->remaining.erase(initial_pair.second);
    std::cout << "Processing initial pair " << initial_pair.first
        << "," << initial_pair.second << "..." << std::endl;

    this->compute_sift_descriptors(initial_pair.first);
    this->compute_sift_descriptors(initial_pair.second);
    this->compute_fundamental_for_pair(&initial_pair);
    this->compute_pose_from_fundamental(initial_pair);
    this->triangulate_initial_pair(initial_pair);
}

void
Bundler::add_next_view_to_bundle (void)
{
    std::size_t view_id = this->select_next_view();
    Viewport& viewport = this->viewports[view_id];
    this->remaining.erase(view_id);

    /* Compute descriptors for this view. */
    this->compute_sift_descriptors(view_id);

    /*
     * Match the descriptors for the current viewport with all other
     * viewports. If the other viewports feature corresponds to a track,
     * the 2D-3D correspondence is collected and considered for pose.
     *
     * Since a track was generated from multiple views, this procedure
     * collects the same track multiple times. The track IDs are made
     * unique by using a vector that records "track IDs" to "SIFT IDs".
     *
     * The following inconsistencies can arise:
     * - One track maps to different SIFT IDs. This issue can be resolved
     *   by removing all SIFT IDs to avoid invalid tracks.
     * - One SIFT ID maps to multiple tracks. This issue must be resolved
     *   by merging the tracks (if possible) to create long tracks.
     *   Merging can fail if inconsisntencies arise.
     *
     * Note: Maybe use two-way mapping using vectors for inconsistencies?
     *   std::vector<int> tracks_to_sift(this->tracks.size());
     *   std::vector<int> sift_to_tracks(viewport.descr_info.size());
     */

    typedef std::vector<int> SiftToTracksMapping;
    typedef std::vector<int> TracksToSiftMapping;
    SiftToTracksMapping sift_to_tracks(viewport.descr_info.size(), -1);
    TracksToSiftMapping tracks_to_sift(this->tracks.size(), -1);
    for (std::size_t i = 0; i < this->viewports.size(); ++i)
    {
        if (this->viewports[i].descr_info.empty())
            continue;
        if (i == view_id)
            continue;

        /*
         * Match the current view to the other view and compute the
         * fundamental matrix for geometric correspondence filtering.
         */
        std::cout << "Processing image pair " << view_id
            << "," << i << "..." << std::endl;
        ImagePair image_pair(view_id, i);
        this->compute_fundamental_for_pair(&image_pair);

        /*
         * For every correspondence, check if the other feature ID
         * corresponds to a track and collect 2D-3D correspondences.
         */
        std::cout << "Collecting 2D-3D correspondences..." << std::endl;
        for (std::size_t j = 0; j < image_pair.indices.size(); ++j)
        {
            int this_sift_id = image_pair.indices[j].first;
            int other_sift_id = image_pair.indices[j].second;
            int other_f2d_id = this->viewports[i].descr_info[other_sift_id].feature2d_id;
            if (other_f2d_id < 0)
                continue;
            int other_f3d_id = this->features[other_f2d_id].feature3d_id;

            /* If the SIFT ID saw a different track, they should be merged. */
            if (sift_to_tracks[this_sift_id] != -1
                && sift_to_tracks[this_sift_id] != other_f3d_id)
            {
                // TODO Merge. Skip SIFT ID for now.
                sift_to_tracks[this_sift_id] = -2;
                continue;
            }
            else
            {
                sift_to_tracks[this_sift_id] = other_f3d_id;
            }

            /* If multiple SIFT IDs map to one track, resolve inconsistency. */
            if (tracks_to_sift[other_f3d_id] != -1
                && tracks_to_sift[other_f3d_id] != this_sift_id)
            {
                sift_to_tracks[this_sift_id] = -2;
                sift_to_tracks[tracks_to_sift[other_f3d_id]] = -2;
                tracks_to_sift[other_f3d_id] = -2;
                continue;
            }
            else
            {
                tracks_to_sift[other_f3d_id] = this_sift_id;
            }
        }
    }

    /* Build list of 2D-3D correspondences and update data structure. */
    std::cout << "Processing 2D-3D correspondences..." << std::endl;
    Correspondences2D3D corresp_2d3d;
    for (std::size_t i = 0; i < tracks_to_sift.size(); ++i)
    {
        if (tracks_to_sift[i] < 0)
            continue;
        int const track_id = i;
        int const sift_id = tracks_to_sift[i];

        Feature3D& track = this->tracks[track_id];
        Feature2DRef const& feature_ref = viewport.descr_info[sift_id];

        /* Create new 2D feature, update track. */
        Feature2D feature;
        std::copy(feature_ref.pos, feature_ref.pos + 2, feature.pos);
        feature.view_id = view_id;
        feature.descriptor_id = sift_id;
        feature.feature3d_id = track_id;
        // TODO Color
        this->features.push_back(feature);
        track.feature2d_ids.push_back(this->features.size() - 1);

        track.color[0] = 1.0f;
        track.color[1] = 0.0f;
        track.color[2] = 0.0f;

        /* Create correspondence. */
        Correspondence2D3D corresp;
        std::copy(feature.pos, feature.pos + 2, corresp.p2d);
        std::copy(track.pos, track.pos + 3, corresp.p3d);
        corresp_2d3d.push_back(corresp);
    }

    /* Compute pose given the 2D-3D correspondences. */
    std::cout << "Computing pose from " << corresp_2d3d.size()
        << " 2D-3D correspondences..." << std::endl;
    this->compute_pose_from_2d3d(view_id, corresp_2d3d);

    /* Collect new 2D-2D correspondences and triangulate new tracks. */
    // TODO
}

Bundler::ImagePair
Bundler::select_initial_pair (void)
{
    // TODO: Replace with proper implementation
    return ImagePair(0, 1);
}

int
Bundler::select_next_view (void)
{
    // TODO: Replace with proper implementation
    return *this->remaining.begin();
}

void
Bundler::compute_sift_descriptors (std::size_t view_id)
{
    Viewport& view = this->viewports[view_id];

    /* Compute SIFT descriptors. */
    sfm::Sift sift(this->options.sift_options);
    sift.set_image(view.image);
    sift.process();

    /* Convert descriptors to matching data structure. */
    sfm::Sift::Descriptors const& descr = sift.get_descriptors();
    view.descr_info.resize(descr.size());
    view.descr_data.allocate(descr.size() * 128);
    float* out_ptr = view.descr_data.begin();
    for (std::size_t i = 0; i < descr.size(); ++i, out_ptr += 128)
    {
        sfm::Sift::Descriptor const& d = descr[i];
        std::copy(d.data.begin(), d.data.end(), out_ptr);
        view.descr_info[i].pos[0] = d.x;
        view.descr_info[i].pos[1] = d.y;
        view.descr_info[i].feature2d_id = -1;
    }

    /* FIXME: How to enforce this properly? */
    this->options.sift_matching_options.descriptor_length = 128;
}

/* Success of this function is indicated if indices are not empty. */
void
Bundler::compute_fundamental_for_pair (ImagePair* image_pair)
{
    Viewport const& view_1 = this->viewports[image_pair->first];
    Viewport const& view_2 = this->viewports[image_pair->second];
    image_pair->indices.clear();

    /* Perform two-view descriptor matching. */
    Matching::Result matching_result;
    std::size_t num_matches = 0;
    {
        util::WallTimer timer;
        sfm::Matching::twoway_match(this->options.sift_matching_options,
            view_1.descr_data.begin(), view_1.descr_info.size(),
            view_2.descr_data.begin(), view_2.descr_info.size(),
            &matching_result);
        sfm::Matching::remove_inconsistent_matches(&matching_result);
        num_matches = sfm::Matching::count_consistent_matches(matching_result);
        std::cout << "Two-view matching took " << timer.get_elapsed() << "ms, "
            << num_matches << " matches." << std::endl;
    }

    /* Require at least 8 matches. (This can be much higher?) */
    if (num_matches < 8)
    {
        return;
    }

    /* Build correspondences from feature matching result. */
    sfm::Correspondences unfiltered_matches;
    CorrespondenceIndices unfiltered_indices;
    {
        std::vector<int> const& m12 = matching_result.matches_1_2;
        for (std::size_t i = 0; i < m12.size(); ++i)
        {
            if (m12[i] < 0)
                continue;

            sfm::Correspondence match;
            match.p1[0] = view_1.descr_info[i].pos[0];
            match.p1[1] = view_1.descr_info[i].pos[1];
            match.p2[0] = view_2.descr_info[m12[i]].pos[0];
            match.p2[1] = view_2.descr_info[m12[i]].pos[1];
            unfiltered_matches.push_back(match);
            unfiltered_indices.push_back(std::make_pair(i, m12[i]));
        }
    }

    /* Pose RANSAC. */
    sfm::RansacFundamental::Result ransac_result;
    {
        sfm::RansacFundamental ransac(this->options.ransac_fundamental_options);
        util::WallTimer timer;
        ransac.estimate(unfiltered_matches, &ransac_result);
        std::cout << "RANSAC took " << timer.get_elapsed() << "ms, "
            << ransac_result.inliers.size() << " inliers." << std::endl;
    }

    /* Require at least 8 inlier matches. */
    if (ransac_result.inliers.size() < 8)
        return;

    /* Build correspondences from inliers only. */
    int const num_inliers = ransac_result.inliers.size();
    sfm::Correspondences inlier_matches(num_inliers);
    image_pair->indices.resize(num_inliers);
    for (int i = 0; i < num_inliers; ++i)
    {
        int const inlier_id = ransac_result.inliers[i];
        inlier_matches[i] = unfiltered_matches[inlier_id];
        image_pair->indices[i] = unfiltered_indices[inlier_id];
    }

#if 0
    /* DEBUG: Draw initial pair matching result. */
    mve::ByteImage::Ptr matching_image = sfm::Visualizer::draw_matches
        (view_1.image, view_2.image, inlier_matches);
    mve::image::save_file(matching_image, "/tmp/matching_init_pair.png");
#endif

    /* Find normalization for inliers and re-compute fundamental. */
    std::cout << "Re-computing fundamental matrix for inliers..." << std::endl;
    {
        math::Matrix3d T1, T2;
        sfm::FundamentalMatrix F;
        sfm::compute_normalization(inlier_matches, &T1, &T2);
        sfm::apply_normalization(T1, T2, &inlier_matches);
        sfm::fundamental_least_squares(inlier_matches, &F);
        sfm::enforce_fundamental_constraints(&F);
        image_pair->fundamental = T2.transposed() * F * T1;
    }
}

void
Bundler::compute_pose_from_fundamental (ImagePair const& image_pair)
{
    Viewport& view_1 = this->viewports[image_pair.first];
    Viewport& view_2 = this->viewports[image_pair.second];

    /* Compute pose from fundamental matrix. */
    std::cout << "Computing pose..." << std::endl;
    sfm::CameraPose pose1, pose2;
    {
        /* Populate K-matrices. */
        int const width1 = view_1.image->width();
        int const height1 = view_1.image->height();
        int const width2 = view_2.image->width();
        int const height2 = view_2.image->height();
        double flen1 = view_1.focal_length
            * static_cast<double>(std::max(width1, height1));
        double flen2 = view_2.focal_length
            * static_cast<double>(std::max(width2, height2));
        pose1.set_k_matrix(flen1, width1 / 2.0, height1 / 2.0);
        pose1.init_canonical_form();
        pose2.set_k_matrix(flen2, width2 / 2.0, height2 / 2.0);

        /* Compute essential matrix from fundamental matrix (HZ (9.12)). */
        sfm::EssentialMatrix E = pose2.K.transposed()
            * image_pair.fundamental * pose1.K;

        /* Compute pose from essential. */
        std::vector<sfm::CameraPose> poses;
        sfm::pose_from_essential(E, &poses);

        /* Prepare a single correspondence to test which pose is correct. */
        sfm::Correspondence test_match;
        {
            int index_view1 = image_pair.indices[0].first;
            int index_view2 = image_pair.indices[0].second;
            test_match.p1[0] = view_1.descr_info[index_view1].pos[0];
            test_match.p1[1] = view_1.descr_info[index_view1].pos[1];
            test_match.p2[0] = view_2.descr_info[index_view2].pos[0];
            test_match.p2[1] = view_2.descr_info[index_view2].pos[1];
        }

        /* Find the correct pose using point test (HZ Fig. 9.12). */
        bool found_pose = false;
        for (std::size_t i = 0; i < poses.size(); ++i)
        {
            poses[i].K = pose2.K;
            if (sfm::is_consistent_pose(test_match, pose1, poses[i]))
            {
                pose2 = poses[i];
                found_pose = true;
                break;
            }
        }
        if (!found_pose)
        {
            std::cout << "Could not find valid pose." << std::endl;
            return;
        }
    }

    /* Store recovered pose in viewport. */
    view_1.pose = pose1;
    view_2.pose = pose2;
}

void
Bundler::compute_pose_from_2d3d (int view_id, Correspondences2D3D const& corresp)
{
    Viewport& view = this->viewports[view_id];

    RansacPose::Options opts;
    opts.max_iterations = 1000;
    opts.threshold = 3.0f;

    RansacPose::Result result;
    RansacPose ransac(opts);
    ransac.estimate(corresp, &result);

    math::Matrix<double, 3, 4> p_matrix = result.p_matrix; // fixme

#if 0
    /* Estimate pose given the 2D-3D correspondences. */
    // TODO: Normalization for stability?
    math::Matrix<double, 3, 4> p_matrix;
    pose_from_2d_3d_correspondences(corresp, &p_matrix);
#endif

    /* Build K matrix for the view. */
    double const width = static_cast<double>(view.image->width());
    double const height = static_cast<double>(view.image->height());
    double const flen = view.focal_length * std::max(width, height);

    view.pose.set_k_matrix(flen, width / 2.0, height / 2.0);
    view.pose.set_from_p_and_known_k(p_matrix);
}

void
Bundler::triangulate_initial_pair (ImagePair const& image_pair)
{
    Viewport& view1 = this->viewports[image_pair.first];
    Viewport& view2 = this->viewports[image_pair.second];

    /* Triangulate 3D points from pose. */
    this->features.reserve(image_pair.indices.size() * 2);
    this->tracks.reserve(image_pair.indices.size());
    for (std::size_t i = 0; i < image_pair.indices.size(); ++i)
    {
        int index1 = image_pair.indices[i].first;
        int index2 = image_pair.indices[i].second;
        Feature2DRef& f2dr_1 = view1.descr_info[index1];
        Feature2DRef& f2dr_2 = view2.descr_info[index2];

        /* Triangulate match. */
        Correspondence match;
        std::copy(f2dr_1.pos, f2dr_1.pos + 2, match.p1);
        std::copy(f2dr_2.pos, f2dr_2.pos + 2, match.p2);
        math::Vec3d x = sfm::triangulate_match(match, view1.pose, view2.pose);

        /* Register new features. */
        Feature2D f2d_1, f2d_2;
        std::copy(f2dr_1.pos, f2dr_1.pos + 2, f2d_1.pos);
        std::copy(f2dr_2.pos, f2dr_2.pos + 2, f2d_2.pos);
        f2dr_1.feature2d_id = this->features.size();
        f2d_1.descriptor_id = index1;
        f2d_1.feature3d_id = this->tracks.size();
        this->features.push_back(f2d_1);
        f2dr_2.feature2d_id = this->features.size();
        f2d_2.descriptor_id = index2;
        f2d_2.feature3d_id = this->tracks.size();
        this->features.push_back(f2d_2);

        /* Register new track. */
        Feature3D f3d;
        std::copy(*x, *x + 3, f3d.pos);
        std::fill(f3d.color, f3d.color + 3, 1.0f);
        f3d.feature2d_ids.push_back(f2dr_1.feature2d_id);
        f3d.feature2d_ids.push_back(f2dr_2.feature2d_id);
        this->tracks.push_back(f3d);
    }
}

void
Bundler::save_tracks_to_mesh(std::string const& filename)
{
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    for (std::size_t i = 0; i < this->tracks.size(); ++i)
    {
        math::Vec3d x(this->tracks[i].pos);
        verts.push_back(x);
    }
    mve::geom::save_mesh(mesh, filename);
}

void
Bundler::generate_scene_from_bundle (std::string const& directory)
{
    if (!util::fs::dir_exists(directory.c_str())
        && !util::fs::mkdir(directory.c_str()))
    {
        std::cerr << "Error creating scene directory!" << std::endl;
        return;
    }

    std::string views_dir = directory + "/views";
    if (!util::fs::dir_exists(views_dir.c_str())
        && !util::fs::mkdir(views_dir.c_str()))
    {
        std::cerr << "Error creating views directory!" << std::endl;
        return;
    }

    /* Generate bundle file. */
    mve::Bundle::Ptr bundle = mve::Bundle::create();
    mve::Bundle::Cameras& cameras = bundle->get_cameras();
    mve::Bundle::Features& features = bundle->get_features();

    /* Add features to bundle file. */
    for (std::size_t i = 0; i < this->tracks.size(); ++i)
    {
        mve::Bundle::Feature3D feature;
        sfm::Bundler::Feature3D track = this->tracks[i];
        std::copy(track.pos, track.pos + 3, feature.pos);
        std::copy(track.color, track.color + 3, feature.color);
        for (std::size_t j = 0; j < track.feature2d_ids.size(); ++j)
        {
            int const f2d_id = track.feature2d_ids[j];
            sfm::Bundler::Feature2D f2d = this->features[f2d_id];

            mve::Bundle::Feature2D feature2d;
            feature2d.feature_id = f2d.descriptor_id;
            feature2d.view_id = f2d.view_id;
            std::copy(f2d.pos, f2d.pos + 2, feature2d.pos);
            feature.refs.push_back(feature2d);
        }
        features.push_back(feature);
    }

    /* Generate MVE views and add cameras to bundle file. */
    for (std::size_t i = 0; i < this->viewports.size(); ++i)
    {
        Viewport const& viewport = this->viewports[i];
        mve::CameraInfo camera;
        camera.flen = viewport.focal_length;
        std::copy(viewport.pose.t.begin(), viewport.pose.t.end(), camera.trans);
        std::copy(viewport.pose.R.begin(), viewport.pose.R.end(), camera.rot);
        std::string view_name = "view_" + util::string::get_filled(i, 4, '0') + ".mve";

        mve::View::Ptr view = mve::View::create();
        view->set_id(i);
        view->set_camera(camera);
        view->add_image("original", viewport.image->duplicate());
        view->save_mve_file_as(views_dir + "/" + view_name);

        cameras.push_back(camera);
    }

    /* Save bundle file. */
    mve::save_mve_bundle(bundle, directory + "/synth_0.out");
}

SFM_NAMESPACE_END
