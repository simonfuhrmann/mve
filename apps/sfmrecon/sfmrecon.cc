/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the GPL 3 license. See the LICENSE.txt file for details.
 */

#include <fstream>
#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>

#include "util/system.h"
#include "util/timer.h"
#include "util/arguments.h"
#include "util/file_system.h"
#include "util/tokenizer.h"
#include "mve/scene.h"
#include "mve/bundle.h"
#include "mve/bundle_io.h"
#include "mve/image.h"
#include "mve/image_tools.h"
#include "sfm/nearest_neighbor.h"
#include "sfm/feature_set.h"
#include "sfm/bundler_common.h"
#include "sfm/bundler_features.h"
#include "sfm/bundler_matching.h"
#include "sfm/bundler_tracks.h"
#include "sfm/bundler_init_pair.h"
#include "sfm/bundler_intrinsics.h"
#include "sfm/bundler_incremental.h"

#define RAND_SEED_MATCHING 0
#define RAND_SEED_SFM 0

struct AppSettings
{
    std::string scene_path;
    std::string original_name;
    std::string undistorted_name;
    std::string exif_name;
    std::string prebundle_file;
    std::string log_file;
    int max_image_size;
    bool lowres_matching;
    bool normalize_scene;
    bool skip_sfm;
    bool always_full_ba;
    bool fixed_intrinsics;
    bool shared_intrinsics;
    bool intrinsics_from_views;
    int video_matching;
    float track_error_thres_factor;
    float new_track_error_thres;
    int initial_pair_1;
    int initial_pair_2;
    int min_views_per_track;
};

void
log_message (AppSettings const& conf, std::string const& message)
{
    if (conf.log_file.empty())
        return;
    std::string fname = util::fs::join_path(conf.scene_path, conf.log_file);
    std::ofstream out(fname.c_str(), std::ios::app);
    if (!out.good())
        return;

    time_t rawtime;
    std::time(&rawtime);
    struct std::tm* timeinfo;
    timeinfo = std::localtime(&rawtime);
    char timestr[20];
    std::strftime(timestr, 20, "%Y-%m-%d %H:%M:%S", timeinfo);

    out << timestr << "  " << message << std::endl;
    out.close();
}

void
features_and_matching (mve::Scene::Ptr scene, AppSettings const& conf,
    sfm::bundler::ViewportList* viewports,
    sfm::bundler::PairwiseMatching* pairwise_matching)
{
    /* Feature computation for the scene. */
    sfm::bundler::Features::Options feature_opts;
    feature_opts.image_embedding = conf.original_name;
    feature_opts.max_image_size = conf.max_image_size;
    feature_opts.feature_options.feature_types = sfm::FeatureSet::FEATURE_ALL;

    std::cout << "Computing image features..." << std::endl;
    {
        util::WallTimer timer;
        sfm::bundler::Features bundler_features(feature_opts);
        bundler_features.compute(scene, viewports);

        std::cout << "Computing features took " << timer.get_elapsed()
            << " ms." << std::endl;
        log_message(conf, "Feature detection took "
            + util::string::get(timer.get_elapsed()) + "ms.");
    }

    /* Exhaustive matching between all pairs of views. */
    sfm::bundler::Matching::Options matching_opts;
    //matching_opts.ransac_opts.max_iterations = 1000;
    //matching_opts.ransac_opts.threshold = 0.0015;
    matching_opts.ransac_opts.verbose_output = false;
    matching_opts.use_lowres_matching = conf.lowres_matching;
    matching_opts.match_num_previous_frames = conf.video_matching;

    std::cout << "Performing feature matching..." << std::endl;
    {
        util::WallTimer timer;
        sfm::bundler::Matching bundler_matching(matching_opts);
        bundler_matching.compute(*viewports, pairwise_matching);
        std::cout << "Matching took " << timer.get_elapsed()
            << " ms." << std::endl;
        log_message(conf, "Feature matching took "
            + util::string::get(timer.get_elapsed()) + "ms.");
    }

    if (pairwise_matching->empty())
    {
        std::cerr << "Error: No matching image pairs. Exiting." << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void
sfm_reconstruct (AppSettings const& conf)
{
#if ENABLE_SSE2_NN_SEARCH && defined(__SSE2__)
    std::cout << "SSE2 accelerated matching is enabled." << std::endl;
#else
    std::cout << "SSE2 accelerated matching is disabled." << std::endl;
#endif

#if ENABLE_SSE3_NN_SEARCH && defined(__SSE3__)
    std::cout << "SSE3 accelerated matching is enabled." << std::endl;
#else
    std::cout << "SSE3 accelerated matching is disabled." << std::endl;
#endif

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

    /* Try to load the pairwise matching from the prebundle. */
    std::string const prebundle_path
        = util::fs::join_path(scene->get_path(), conf.prebundle_file);
    sfm::bundler::ViewportList viewports;
    sfm::bundler::PairwiseMatching pairwise_matching;
    if (!util::fs::file_exists(prebundle_path.c_str()))
    {
        log_message(conf, "Starting feature matching.");
        util::system::rand_seed(RAND_SEED_MATCHING);
        features_and_matching(scene, conf, &viewports, &pairwise_matching);

        std::cout << "Saving pre-bundle to file..." << std::endl;
        sfm::bundler::save_prebundle_to_file(viewports, pairwise_matching, prebundle_path);
    }
    else if (!conf.skip_sfm)
    {
        log_message(conf, "Loading pairwise matching from file.");
        std::cout << "Loading pairwise matching from file..." << std::endl;
        sfm::bundler::load_prebundle_from_file(prebundle_path,
            &viewports, &pairwise_matching);
    }

    if (conf.skip_sfm)
    {
        std::cout << "Prebundle finished, skipping SfM. Exiting." << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    /* Drop descriptors and embeddings to save memory. */
    scene->cache_cleanup();
    for (std::size_t i = 0; i < viewports.size(); ++i)
        viewports[i].features.clear_descriptors();

    /* Check if there are some matching images. */
    if (pairwise_matching.empty())
    {
        std::cerr << "No matching image pairs. Exiting." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    /*
     * Obtain camera intrinsics from the views or guess them from EXIF.
     * If neither is available, fall back to a default.
     *
     * FIXME: Once width and height in the viewport is gone, this fully
     * initializes the viewports. Thus viewport info does not need to be
     * saved in the prebundle.sfm, and the file becomes matching.sfm.
     * Obtaining EXIF guesses can be moved out of the feature module to here.
     * The following code can become its own module "bundler_intrinsics".
     */
    {
        sfm::bundler::Intrinsics::Options intrinsics_opts;
        if (conf.intrinsics_from_views)
        {
            intrinsics_opts.intrinsics_source
                = sfm::bundler::Intrinsics::FROM_VIEWS;
        }
        std::cout << "Initializing camera intrinsics..." << std::endl;
        sfm::bundler::Intrinsics intrinsics(intrinsics_opts);
        intrinsics.compute(scene, &viewports);
    }

    /* Start incremental SfM. */
    log_message(conf, "Starting incremental SfM.");
    util::WallTimer timer;
    util::system::rand_seed(RAND_SEED_SFM);

    /* Compute connected feature components, i.e. feature tracks. */
    sfm::bundler::TrackList tracks;
    {
        sfm::bundler::Tracks::Options tracks_options;
        tracks_options.verbose_output = true;

        sfm::bundler::Tracks bundler_tracks(tracks_options);
        std::cout << "Computing feature tracks..." << std::endl;
        bundler_tracks.compute(pairwise_matching, &viewports, &tracks);
        std::cout << "Created a total of " << tracks.size()
            << " tracks." << std::endl;
    }

    /* Remove color data and pairwise matching to save memory. */
    for (std::size_t i = 0; i < viewports.size(); ++i)
        viewports[i].features.colors.clear();
    pairwise_matching.clear();

    /* Search for a good initial pair, or use the user-specified one. */
    sfm::bundler::InitialPair::Result init_pair_result;
    sfm::bundler::InitialPair::Options init_pair_opts;
    if (conf.initial_pair_1 < 0 || conf.initial_pair_2 < 0)
    {
        //init_pair_opts.homography_opts.max_iterations = 1000;
        //init_pair_opts.homography_opts.threshold = 0.005f;
        init_pair_opts.homography_opts.verbose_output = false;
        init_pair_opts.max_homography_inliers = 0.6f;
        init_pair_opts.verbose_output = true;

        sfm::bundler::InitialPair init_pair(init_pair_opts);
        init_pair.initialize(viewports, tracks);
        init_pair.compute_pair(&init_pair_result);
    }
    else
    {
        sfm::bundler::InitialPair init_pair(init_pair_opts);
        init_pair.initialize(viewports, tracks);
        init_pair.compute_pair(conf.initial_pair_1, conf.initial_pair_2,
            &init_pair_result);
    }

    if (init_pair_result.view_1_id < 0 || init_pair_result.view_2_id < 0
        || init_pair_result.view_1_id >= static_cast<int>(viewports.size())
        || init_pair_result.view_2_id >= static_cast<int>(viewports.size()))
    {
        std::cerr << "Error finding initial pair, exiting!" << std::endl;
        std::cerr << "Try manually specifying an initial pair." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::cout << "Using views " << init_pair_result.view_1_id
        << " and " << init_pair_result.view_2_id
        << " as initial pair." << std::endl;

    /* Incrementally compute full bundle. */
    sfm::bundler::Incremental::Options incremental_opts;
    //incremental_opts.pose_p3p_opts.max_iterations = 1000;
    //incremental_opts.pose_p3p_opts.threshold = 0.005f;
    incremental_opts.pose_p3p_opts.verbose_output = false;
    incremental_opts.track_error_threshold_factor = conf.track_error_thres_factor;
    incremental_opts.new_track_error_threshold = conf.new_track_error_thres;
    incremental_opts.min_triangulation_angle = MATH_DEG2RAD(1.0);
    incremental_opts.ba_fixed_intrinsics = conf.fixed_intrinsics;
    incremental_opts.ba_shared_intrinsics = conf.shared_intrinsics;
    incremental_opts.verbose_output = true;

    /* Initialize cameras. */
    sfm::CameraPoseList cameras;
    cameras.resize(viewports.size());
    cameras[init_pair_result.view_1_id] = init_pair_result.view_1_pose;
    cameras[init_pair_result.view_2_id] = init_pair_result.view_2_pose;

    sfm::bundler::Incremental incremental(incremental_opts);
    incremental.initialize(&viewports, &tracks, &cameras);

    /* Reconstruct track positions for the intial pair. */
    incremental.triangulate_new_tracks(2);
    incremental.invalidate_large_error_tracks();

    /* Run bundle adjustment. */
    std::cout << "Running full bundle adjustment..." << std::endl;
    incremental.bundle_adjustment_full();

    /* Reconstruct remaining views. */
    int num_cameras_reconstructed = 2;
    int full_ba_num_skipped = 0;
    while (true)
    {
        std::vector<int> next_views;
        incremental.find_next_views(&next_views);

        if (next_views.empty())
        {
            std::cout << "SfM reconstruction finished." << std::endl;
            break;
        }

        int next_view_id = -1;
        for (std::size_t i = 0; i < next_views.size(); ++i)
        {
            std::cout << std::endl;
            std::cout << "Adding next view ID " << next_views[i]
                << " (" << (num_cameras_reconstructed + 1) << " of "
                << viewports.size() << ")..." << std::endl;
            if (incremental.reconstruct_next_view(next_views[i]))
            {
                next_view_id = next_views[i];
                break;
            }
        }

        if (next_view_id < 0)
        {
            std::cout << "No valid next view. Exiting." << std::endl;
            break;
        }

        std::cout << "Running single camera bundle adjustment..." << std::endl;
        incremental.bundle_adjustment_single_cam(next_view_id);
        incremental.triangulate_new_tracks(conf.min_views_per_track);
        incremental.invalidate_large_error_tracks();
        num_cameras_reconstructed += 1;

        /* Run full bundle adjustment only after a couple of views. */
        int const full_ba_skip_views = conf.always_full_ba ? 0
            : std::min(5, num_cameras_reconstructed / 15);
        if (full_ba_num_skipped < full_ba_skip_views)
        {
            std::cout << "Skipping full bundle adjustment (skipping "
                << full_ba_skip_views << " views)." << std::endl;
            full_ba_num_skipped += 1;
        }
        else
        {
            std::cout << "Running full bundle adjustment..." << std::endl;
            incremental.bundle_adjustment_full();
            full_ba_num_skipped = 0;
        }
    }

    if (full_ba_num_skipped > 0)
    {
        std::cout << "Running final bundle adjustment..." << std::endl;
        incremental.bundle_adjustment_full();
    }

    std::cout << "SfM reconstruction took " << timer.get_elapsed()
        << " ms." << std::endl;
    log_message(conf, "SfM reconstruction took "
        + util::string::get(timer.get_elapsed()) + "ms.");

    /* Normalize scene if requested. */
    if (conf.normalize_scene)
    {
        std::cout << "Normalizing scene..." << std::endl;
        incremental.normalize_scene();
    }

    /* Save bundle file to scene. */
    std::cout << "Creating bundle data structure..." << std::endl;
    mve::Bundle::Ptr bundle = incremental.create_bundle();
    mve::save_mve_bundle(bundle, scene->get_path() + "/synth_0.out");

    /* Apply bundle cameras to views. */
    mve::Bundle::Cameras const& bundle_cams = bundle->get_cameras();
    mve::Scene::ViewList const& views = scene->get_views();
    if (bundle_cams.size() != views.size())
    {
        std::cerr << "Error: Invalid number of cameras!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

#pragma omp parallel for schedule(dynamic,1)
    for (std::size_t i = 0; i < bundle_cams.size(); ++i)
    {
        mve::View::Ptr view = views[i];
        mve::CameraInfo const& cam = bundle_cams[i];
        if (view == NULL)
            continue;
        if (view->get_camera().flen == 0.0f && cam.flen == 0.0f)
            continue;

        view->set_camera(cam);

        /* Undistort image. */
        if (!conf.undistorted_name.empty())
        {
            mve::ByteImage::Ptr original
                = view->get_byte_image(conf.original_name);
            if (original == NULL)
                continue;
            mve::ByteImage::Ptr undist
                = mve::image::image_undistort_vsfm<uint8_t>
                (original, cam.flen, cam.dist[0]);
            view->set_image(undist, conf.undistorted_name);
        }

#pragma omp critical
        std::cout << "Saving view " << view->get_directory() << std::endl;
        view->save_view();
        view->cache_cleanup();
    }

    log_message(conf, "SfM reconstruction done.\n");
}

void
check_prebundle (AppSettings const& conf)
{
    std::string const prebundle_path
        = util::fs::join_path(conf.scene_path, conf.prebundle_file);

    if (util::fs::exists(prebundle_path.c_str()))
        return;

    /* Check if the prebundle is writable. */
    std::ofstream out(prebundle_path.c_str());
    if (!out.good())
    {
        out.close();
        std::cerr << "Error: Specified prebundle not writable: "
            << prebundle_path << std::endl;
        std::cerr << "Note: The prebundle is relative to the scene."
            << std::endl;
        std::exit(EXIT_FAILURE);
    }
    out.close();

    /* Looks good. Delete created prebundle. */
    util::fs::unlink(prebundle_path.c_str());
}

int
main (int argc, char** argv)
{
    /* Setup argument parser. */
    util::Arguments args;
    args.set_usage(argv[0], "[ OPTIONS ] SCENE");
    args.set_exit_on_error(true);
    args.set_nonopt_maxnum(1);
    args.set_nonopt_minnum(1);
    args.set_helptext_indent(23);
    args.set_description("Reconstruction of camera parameters "
        "for MVE scenes using Structure from Motion. Note: The "
        "prebundle and the log file are relative to the scene directory.");
    args.add_option('o', "original", true, "Original image embedding [original]");
    args.add_option('e', "exif", true, "EXIF data embedding [exif]");
    args.add_option('m', "max-pixels", true, "Limit image size by iterative half-sizing [6000000]");
    args.add_option('u', "undistorted", true, "Undistorted image embedding [undistorted]");
    args.add_option('\0', "prebundle", true, "Load/store pre-bundle file [prebundle.sfm]");
    args.add_option('\0', "log-file", true, "Log some timings to file []");
    args.add_option('\0', "no-prediction", false, "Disable matchability prediction");
    args.add_option('\0', "normalize", false, "Normalize scene after reconstruction");
    args.add_option('\0', "skip-sfm", false, "Compute prebundle, skip SfM reconstruction");
    args.add_option('\0', "always-full-ba", false, "Run full bundle adjustment after every view");
    args.add_option('\0', "video-matching", true, "Only match to ARG previous frames [0]");
    args.add_option('\0', "fixed-intrinsics", false, "Do not optimize camera intrinsics");
    args.add_option('\0', "shared-intrinsics", false, "Share intrinsics between all cameras");
    args.add_option('\0', "intrinsics-from-views", false, "Use intrinsics from MVE views [use EXIF]");
    args.add_option('\0', "track-error-thres", true, "Error threshold for new tracks [10]");
    args.add_option('\0', "track-thres-factor", true, "Error threshold factor for tracks [25]");
    args.add_option('\0', "use-2cam-tracks", false, "Triangulate tracks from only two cameras");
    args.add_option('\0', "initial-pair", true, "Manually specify initial pair IDs [-1,-1]");
    args.parse(argc, argv);

    /* Setup defaults. */
    AppSettings conf;
    conf.scene_path = args.get_nth_nonopt(0);
    conf.original_name = "original";
    conf.undistorted_name = "undistorted";
    conf.exif_name = "exif";
    conf.prebundle_file = "prebundle.sfm";
    conf.max_image_size = 6000000;
    conf.lowres_matching = true;
    conf.normalize_scene = false;
    conf.skip_sfm = false;
    conf.always_full_ba = false;
    conf.video_matching = 0;
    conf.fixed_intrinsics = false;
    conf.shared_intrinsics = false;
    conf.intrinsics_from_views = false;
    conf.track_error_thres_factor = 25.0f;
    conf.new_track_error_thres = 0.01f;
    conf.min_views_per_track = 3;
    conf.initial_pair_1 = -1;
    conf.initial_pair_2 = -1;

    /* Read arguments. */
    for (util::ArgResult const* i = args.next_option();
        i != NULL; i = args.next_option())
    {
        if (i->opt->lopt == "original")
            conf.original_name = i->arg;
        else if (i->opt->lopt == "exif")
            conf.exif_name = i->arg;
        else if (i->opt->lopt == "undistorted")
            conf.undistorted_name = i->arg;
        else if (i->opt->lopt == "max-pixels")
            conf.max_image_size = i->get_arg<int>();
        else if (i->opt->lopt == "prebundle")
            conf.prebundle_file = i->arg;
        else if (i->opt->lopt == "log-file")
            conf.log_file = i->arg;
        else if (i->opt->lopt == "no-prediction")
            conf.lowres_matching = false;
        else if (i->opt->lopt == "normalize")
            conf.normalize_scene = true;
        else if (i->opt->lopt == "skip-sfm")
            conf.skip_sfm = true;
        else if (i->opt->lopt == "always-full-ba")
            conf.always_full_ba = true;
        else if (i->opt->lopt == "video-matching")
            conf.video_matching = i->get_arg<int>();
        else if (i->opt->lopt == "fixed-intrinsics")
            conf.fixed_intrinsics = true;
        else if (i->opt->lopt == "shared-intrinsics")
            conf.shared_intrinsics = true;
        else if (i->opt->lopt == "intrinsics-from-views")
            conf.intrinsics_from_views = true;
        else if (i->opt->lopt == "track-error-thres")
            conf.new_track_error_thres = i->get_arg<float>();
        else if (i->opt->lopt == "track-thres-factor")
            conf.track_error_thres_factor = i->get_arg<float>();
        else if (i->opt->lopt == "use-2cam-tracks")
            conf.min_views_per_track = 2;
        else if (i->opt->lopt == "initial-pair")
        {
            util::Tokenizer tok;
            tok.split(i->arg, ',');
            if (tok.size() != 2)
            {
                std::cerr << "Error: Cannot parse initial pair." << std::endl;
                std::exit(EXIT_FAILURE);
            }
            conf.initial_pair_1 = tok.get_as<int>(0);
            conf.initial_pair_2 = tok.get_as<int>(1);
            std::cout << "Using initial pair (" << conf.initial_pair_1
                << "," << conf.initial_pair_2 << ")." << std::endl;
        }
        else
            throw std::invalid_argument("Unexpected option");
    }

    check_prebundle(conf);
    sfm_reconstruct(conf);

    return EXIT_SUCCESS;
}
