/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 *
 * App to create features (which are usually created by SfM) for scenes
 * with views with known camera paramters (both intrinsics and extrinsics)
 * but without features.
 */

#include <algorithm>
#include <iostream>
#include <string>

#include "math/vector.h"
#include "mve/bundle_io.h"
#include "mve/bundle.h"
#include "mve/camera.h"
#include "mve/scene.h"
#include "mve/view.h"
#include "sfm/bundler_common.h"
#include "sfm/bundler_features.h"
#include "sfm/bundler_matching.h"
#include "sfm/bundler_tracks.h"
#include "sfm/triangulate.h"
#include "util/arguments.h"
#include "util/file_system.h"
#include "util/system.h"
#include "util/timer.h"

#define RAND_SEED_MATCHING 0

struct AppSettings
{
    std::string scene_path;
    std::string prebundle_file = "prebundle.sfm";
    std::string original_name = "original";
    int max_image_size = 6000000;
};

void
features_and_matching (mve::Scene::Ptr scene, AppSettings const& conf,
    sfm::bundler::ViewportList* viewports,
    sfm::bundler::PairwiseMatching* pairwise_matching)
{
    /* Feature computation for the scene. */
    std::cout << "Computing image features..." << std::endl;
    sfm::bundler::Features::Options feature_opts;
    feature_opts.image_embedding = conf.original_name;
    feature_opts.max_image_size = conf.max_image_size;
    feature_opts.feature_options.feature_types = sfm::FeatureSet::FEATURE_ALL;
    {
        util::WallTimer timer;
        sfm::bundler::Features bundler_features(feature_opts);
        bundler_features.compute(scene, viewports);
        std::cout << "Computing features took " << timer.get_elapsed()
            << " ms." << std::endl;
    }

    /* Exhaustive matching between all pairs of views. */
    std::cout << "Performing feature matching..." << std::endl;
    sfm::bundler::Matching::Options matching_opts;
    matching_opts.ransac_opts.verbose_output = false;
    matching_opts.use_lowres_matching = true;
    {
        util::WallTimer timer;
        sfm::bundler::Matching bundler_matching(matching_opts);
        bundler_matching.init(viewports);
        bundler_matching.compute(pairwise_matching);
        std::cout << "Matching took " << timer.get_elapsed()
            << " ms." << std::endl;
    }
  }

sfm::CameraPose
view_pose_to_sfm_pose(mve::View::Ptr view)
{
    mve::CameraInfo const& cam = view->get_camera();
    sfm::CameraPose pose;
    std::copy_n(cam.trans, 3, pose.t.begin());
    std::copy_n(cam.rot, 9, pose.R.begin());
    pose.K.fill(0.0);
    // FIXME: This code ignores the pixel aspect ratio!
    pose.K[0] = cam.flen;
    pose.K[4] = cam.flen;
    pose.K[2] = cam.ppoint[0] - 0.5;
    pose.K[5] = cam.ppoint[1] - 0.5;
    pose.K[8] = 1.0;
    return pose;
}

void
feature_recon (mve::Scene::Ptr scene, AppSettings const& conf)
{
    /* Try to load the pairwise matching from the prebundle. */
    std::string const prebundle_path
        = util::fs::join_path(scene->get_path(), conf.prebundle_file);

    sfm::bundler::ViewportList viewports;
    sfm::bundler::PairwiseMatching pairwise_matching;
    if (!util::fs::file_exists(prebundle_path.c_str()))
    {
        std::cout << "Starting feature matching..." << std::endl;
        util::system::rand_seed(RAND_SEED_MATCHING);
        features_and_matching(scene, conf, &viewports, &pairwise_matching);

        std::cout << "Saving pre-bundle to file..." << std::endl;
        sfm::bundler::save_prebundle_to_file(
            viewports, pairwise_matching, prebundle_path);
    }
    else
    {
        std::cout << "Loading pairwise matching from file..." << std::endl;
        sfm::bundler::load_prebundle_from_file(
            prebundle_path, &viewports, &pairwise_matching);
    }

    /* If there are no matching features, exit. */
    if (pairwise_matching.empty())
    {
        std::cerr << "Error: No matching image pairs. Exiting." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    /* Compute connected feature components, i.e. feature tracks. */
    std::cout << "Computing feature tracks..." << std::endl;
    sfm::bundler::TrackList tracks;
    {
        sfm::bundler::Tracks::Options tracks_options;
        tracks_options.verbose_output = true;
        sfm::bundler::Tracks bundler_tracks(tracks_options);
        bundler_tracks.compute(pairwise_matching, &viewports, &tracks);
        std::cout << "Created a total of " << tracks.size()
            << " tracks." << std::endl;
    }

    /* Triangulate tracks. */
    mve::Scene::ViewList const& views = scene->get_views();
    for (std::size_t i = 0; i < tracks.size(); ++i)
    {
        std::vector<sfm::CameraPose> pose_storage;
        std::vector<math::Vec2f> positions_2d;
        std::vector<sfm::CameraPose const*> cam_poses;
        sfm::bundler::FeatureReferenceList const& flist = tracks[i].features;
        for (std::size_t j = 0; j < flist.size(); ++j)
        {
            std::size_t const view_id = flist[j].view_id;
            std::size_t const feat_id = flist[j].feature_id;
            mve::View::Ptr view = views[view_id];
            if (view == nullptr || !view->is_camera_valid())
                continue;

            pose_storage.push_back(view_pose_to_sfm_pose(view));
            cam_poses.push_back(&pose_storage.back());
            positions_2d.push_back(viewports[view_id].features.positions[feat_id]);
        }

        sfm::Triangulate::Options triangulate_options;
        triangulate_options.error_threshold = 0.005f;
        triangulate_options.angle_threshold = 2.0f * MATH_PI / 180.0f;
        triangulate_options.min_num_views = 2;
        sfm::Triangulate triangulator(triangulate_options);
        math::Vec3d track_pos_3d;
        if (triangulator.triangulate(cam_poses, positions_2d, &track_pos_3d))
          tracks[i].pos = track_pos_3d;
        else
          tracks[i].invalidate();
    }

    /* Create bundle and intialize cameras. */
    mve::Bundle::Ptr bundle = mve::Bundle::create();
    mve::Bundle::Cameras& cameras = bundle->get_cameras();
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        cameras.push_back(mve::CameraInfo());
        if (views[i] != nullptr)
            cameras.back() = views[i]->get_camera();
    }

    /* Add tracks to bundle. */
    mve::Bundle::Features& features = bundle->get_features();
    for (std::size_t i = 0; i < tracks.size(); ++i)
    {
        if (!tracks[i].is_valid())
            continue;

        mve::Bundle::Feature3D feature_3d;
        feature_3d.color[0] = tracks[i].color[0] / 255.0f;
        feature_3d.color[1] = tracks[i].color[1] / 255.0f;
        feature_3d.color[2] = tracks[i].color[2] / 255.0f;
        std::copy_n(tracks[i].pos.begin(), 3, feature_3d.pos);

        sfm::bundler::FeatureReferenceList const& flist = tracks[i].features;
        for (std::size_t j = 0; j < flist.size(); ++j)
        {
            std::size_t const view_id = flist[j].view_id;
            std::size_t const feat_id = flist[j].feature_id;
            math::Vec2f const& feat_pos =
                viewports[view_id].features.positions[feat_id];

            mve::Bundle::Feature2D feature_2d;
            feature_2d.view_id = view_id;
            feature_2d.feature_id = feat_id;
            std::copy_n(feat_pos.begin(), 2, feature_2d.pos);
            feature_3d.refs.push_back(feature_2d);
        }
        features.push_back(feature_3d);
    }

    /* Save bundle file. */
    mve::save_mve_bundle(bundle, scene->get_path() + "/synth_0.out");
}

int
main (int argc, char** argv)
{
    util::system::register_segfault_handler();
    util::system::print_build_timestamp("MVE Feature Reconstruction");

    /* Setup argument parser. */
    util::Arguments args;
    args.set_usage(argv[0], "[ OPTIONS ] SCENE_PATH");
    args.set_exit_on_error(true);
    args.set_nonopt_maxnum(1);
    args.set_nonopt_minnum(1);
    args.set_helptext_indent(22);
    args.set_description("Creates features for scenes with know camera "
        "parameters. It performs feature detection and matching, and track "
        "triangulation.");
    args.add_option('o', "original", true,
        "Image embedding for feature detection [original]");
    args.add_option('m', "max-pixels", true,
        "Limit image size by iterative half-sizing [6000000]");
    args.add_option('\0', "prebundle", true,
        "Load/store pre-bundle file [prebundle.sfm]");
    args.parse(argc, argv);

    /* Setup defaults. */
    AppSettings conf;
    conf.scene_path = util::fs::sanitize_path(args.get_nth_nonopt(0));

    /* Read arguments. */
    for (util::ArgResult const* i = args.next_option();
        i != nullptr; i = args.next_option())
    {
        if (i->opt->lopt == "original")
            conf.original_name = i->arg;
        else if (i->opt->lopt == "max-pixels")
            conf.max_image_size = i->get_arg<int>();
        else if (i->opt->lopt == "prebundle")
            conf.prebundle_file = i->arg;
    }

    /* Check command line arguments. */
    if (conf.scene_path.empty())
    {
        args.generate_helptext(std::cerr);
        return EXIT_FAILURE;
    }

    /* Load scene. */
    mve::Scene::Ptr scene;
    try
    {
        scene = mve::Scene::create(conf.scene_path);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error loading scene: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    /* Reconstruct features. */
    feature_recon(scene, conf);
    return EXIT_SUCCESS;
}
