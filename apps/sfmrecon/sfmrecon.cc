/*
 * Structure from Motion reconstruction for MVE scenes.
 * Written by Simon Fuhrmann.
 */

#include <fstream>
#include <iostream>
#include <string>
#include <ctime>

#include "util/system.h"
#include "util/timer.h"
#include "util/arguments.h"
#include "util/file_system.h"
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
#include "sfm/bundler_incremental.h"

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
    bool skip_sfm;
};

void
log_message (AppSettings const& conf, std::string const& message)
{
    if (conf.log_file.empty())
        return;
    std::string fname = util::fs::join_path(conf.scene_path, conf.log_file);
    std::ofstream out(fname.c_str(), std::ios::app);
    out << message << std::endl;
    out.close();
}

void
log_start_of_sfm (AppSettings const& conf)
{
    if (conf.log_file.empty())
        return;

    time_t rawtime;
    std::time(&rawtime);
    struct std::tm* timeinfo;
    timeinfo = std::localtime(&rawtime);
    char timestr[20];
    std::strftime(timestr, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
    log_message(conf, "Starting SfM (" + std::string(timestr) + ")");
}

void
features_and_matching (mve::Scene::Ptr scene, AppSettings const& conf,
    sfm::bundler::ViewportList* viewports,
    sfm::bundler::PairwiseMatching* pairwise_matching)
{
    /* Feature computation for the scene. */
    sfm::bundler::Features::Options feature_opts;
    feature_opts.image_embedding = conf.original_name;
    feature_opts.exif_embedding = conf.exif_name;
    feature_opts.max_image_size = conf.max_image_size;
    feature_opts.feature_options.feature_types = sfm::FeatureSet::FEATURE_ALL;

    std::cout << "Computing image features..." << std::endl;
    {
        util::WallTimer timer;
        sfm::bundler::Features bundler_features(feature_opts);
        bundler_features.compute(scene, viewports);

        std::cout << "Viewport statistics:" << std::endl;
        for (std::size_t i = 0; i < viewports->size(); ++i)
        {
            sfm::bundler::Viewport const& view = viewports->at(i);
            std::cout << "  View " << i << ": "
                << view.width << "x" << view.height << ", "
                << view.features.positions.size() << " features, "
                << "focal length: " << view.focal_length << std::endl;
        }

        std::cout << "Computing features took " << timer.get_elapsed()
            << " ms." << std::endl;
        log_message(conf, "Feature detection took "
            + util::string::get(timer.get_elapsed()) + "ms.");
    }

    /* Exhaustive matching between all pairs of views. */
    sfm::bundler::Matching::Options matching_opts;
    matching_opts.ransac_opts.already_normalized = false;
    matching_opts.ransac_opts.threshold = 3.0f;
    matching_opts.ransac_opts.verbose_output = false;
    matching_opts.use_lowres_matching = conf.lowres_matching;

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
        std::cerr << "No matching image pairs. Exiting." << std::endl;
        std::exit(1);
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
    mve::Scene::Ptr scene = mve::Scene::create(conf.scene_path);
    std::string const prebundle_path
        = util::fs::join_path(scene->get_path(), conf.prebundle_file);

    /* Log time and date if a log file is specified. */
    log_start_of_sfm(conf);

    sfm::bundler::ViewportList viewports;
    sfm::bundler::PairwiseMatching pairwise_matching;
    if (!util::fs::file_exists(prebundle_path.c_str()))
    {
        features_and_matching(scene, conf, &viewports, &pairwise_matching);
        std::cout << "Saving pre-bundle to file..." << std::endl;
        sfm::bundler::save_prebundle_to_file(viewports, pairwise_matching, prebundle_path);
    }
    else if (!conf.skip_sfm)
    {
        std::cout << "Loading pre-bundle from file..." << std::endl;
        sfm::bundler::load_prebundle_from_file(prebundle_path,
            &viewports, &pairwise_matching);
    }

    if (conf.skip_sfm)
    {
        std::cout << "Prebundle finished, skipping SfM. Exiting." << std::endl;
        std::exit(0);
    }

    /* For every viewport drop descriptor information to save memory. */
    for (std::size_t i = 0; i < viewports.size(); ++i)
        viewports[i].features.clean_descriptors();

    /* Remove unused embeddings. */
    scene->cache_cleanup();

    /* Start timer for incremental SfM. */
    util::WallTimer timer;

    /* Compute connected feature components, i.e. feature tracks. */
    sfm::bundler::Tracks::Options tracks_options;
    tracks_options.verbose_output = true;

    sfm::bundler::Tracks bundler_tracks(tracks_options);
    sfm::bundler::TrackList tracks;
    std::cout << "Computing feature tracks..." << std::endl;
    bundler_tracks.compute(pairwise_matching, &viewports, &tracks);
    std::cout << "Created a total of " << tracks.size()
        << " tracks." << std::endl;

    /* Remove unused color data to save memory. */
    for (std::size_t i = 0; i < viewports.size(); ++i)
        viewports[i].features.colors.clear();

    /* Find initial pair. */
    sfm::bundler::InitialPair::Options init_pair_opts;
    init_pair_opts.homography_opts.max_iterations = 1000;
    init_pair_opts.homography_opts.already_normalized = false;
    init_pair_opts.homography_opts.threshold = 3.0f;
    init_pair_opts.homography_opts.verbose_output = false;
    init_pair_opts.max_homography_inliers = 0.5f;
    init_pair_opts.verbose_output = true;

    sfm::bundler::InitialPair::Result init_pair_result;
    sfm::bundler::InitialPair init_pair(init_pair_opts);
    init_pair.compute(viewports, pairwise_matching, &init_pair_result);

    if (init_pair_result.view_1_id < 0 || init_pair_result.view_2_id < 0)
    {
        std::cerr << "Error finding initial pair, exiting!" << std::endl;
        std::exit(1);
    }

    std::cout << "Using views " << init_pair_result.view_1_id
        << " and " << init_pair_result.view_2_id
        << " as initial pair." << std::endl;

    /* Clear pairwise matching to save memeory. */
    pairwise_matching.clear();

    /* Incrementally compute full bundle. */
    sfm::bundler::Incremental::Options incremental_opts;
    incremental_opts.fundamental_opts.already_normalized = false;
    incremental_opts.fundamental_opts.threshold = 3.0f;
    //incremental_opts.fundamental_opts.max_iterations = 1000;
    incremental_opts.fundamental_opts.verbose_output = true;
    incremental_opts.pose_opts.threshold = 4.0f;
    //incremental_opts.pose_opts.max_iterations = 1000;
    incremental_opts.pose_opts.verbose_output = true;
    incremental_opts.pose_p3p_opts.threshold = 10.0f;
    //incremental_opts.pose_p3p_opts.max_iterations = 1000;
    incremental_opts.pose_p3p_opts.verbose_output = false;
    //incremental_opts.track_error_threshold_factor = 50.0;
    //incremental_opts.new_track_error_threshold = 10.0;
    //incremental_opts.min_triangulation_angle = MATH_DEG2RAD(1.0);
    incremental_opts.verbose_output = true;

    sfm::bundler::Incremental incremental(incremental_opts);
    incremental.initialize(&viewports, &tracks);

    /* Reconstruct pose for the initial pair. */
    std::cout << "Computing pose for initial pair..." << std::endl;
    incremental.reconstruct_initial_pair(init_pair_result.view_1_id,
        init_pair_result.view_2_id);

    /* Reconstruct track positions with the intial pair. */
    incremental.triangulate_new_tracks();

    /* Remove tracks with large errors. */
    incremental.delete_large_error_tracks();

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
        incremental.triangulate_new_tracks();
        incremental.delete_large_error_tracks();
        num_cameras_reconstructed += 1;

        /* Run full bundle adjustment only after a couple of views. */
        int const full_ba_skip_views
            = std::min(5, num_cameras_reconstructed / 15);
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
        + util::string::get(timer.get_elapsed()) + "ms.\n");

    std::cout << "Normalizing scene..." << std::endl;
    incremental.normalize_scene();

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
        std::exit(1);
    }

#pragma omp parallel for
    for (std::size_t i = 0; i < bundle_cams.size(); ++i)
    {
        mve::View::Ptr view = views[i];
        mve::CameraInfo const& cam = bundle_cams[i];
        if (view->get_camera().flen == 0.0f && cam.flen == 0.0f)
            continue;

        view->set_camera(cam);

        /* Undistort image. */
        if (!conf.undistorted_name.empty())
        {
            mve::ByteImage::Ptr original
                = view->get_byte_image(conf.original_name);
            mve::ByteImage::Ptr undist
                = mve::image::image_undistort_vsfm<uint8_t>
                (original, cam.flen, cam.dist[0]);
            view->set_image(conf.undistorted_name, undist);
        }

#pragma omp critical
        std::cout << "Saving MVE view " << view->get_filename() << std::endl;
        view->save_mve_file();
        view->cache_cleanup();
    }
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
    args.set_description("This utility reconstructs camera parameters "
        "for MVE scenes using Structure-from-Motion techniques. Note: the "
        "prebundle and the log file are relative to the scene directory.");
    args.add_option('o', "original", true, "Original image embedding [original]");
    args.add_option('e', "exif", true, "EXIF data embedding [exif]");
    args.add_option('m', "max-pixels", true, "Limit image size by iterative half-sizing [6000000]");
    args.add_option('u', "undistorted", true, "Undistorted image embedding [undistorted]");
    args.add_option('\0', "prebundle", true, "Load/store pre-bundle from file [prebundle.sfm]");
    args.add_option('\0', "log-file", true, "Logs some timings to file []");
    args.add_option('\0', "no-prediction", false, "Disables matchability prediction");
    args.add_option('\0', "skip-sfm", false, "Compute prebundle, skip SfM reconstruction");
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
    conf.skip_sfm = false;

    /* General settings. */
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
        else if (i->opt->lopt == "skip-sfm")
            conf.skip_sfm = true;
        else
            throw std::invalid_argument("Unexpected option");
    }

    util::system::rand_seed(0);
    sfm_reconstruct(conf);

    return 0;
}
