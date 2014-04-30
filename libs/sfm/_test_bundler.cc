#include <iostream>
#include <string>

#include "util/system.h"
#include "util/file_system.h"
#include "mve/scene.h"
#include "mve/bundle.h"
#include "mve/bundle_io.h"
#include "mve/image_io.h"
#include "mve/image_tools.h"
#include "sfm/sift.h"
#include "sfm/bundler_common.h"
#include "sfm/bundler_features.h"
#include "sfm/bundler_matching.h"
#include "sfm/bundler_tracks.h"
#include "sfm/bundler_init_pair.h"
#include "sfm/bundler_incremental.h"

int
main (int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Syntax: " << argv[0] << " <scene>" << std::endl;
        return 1;
    }

    util::system::rand_seed(2);

    std::string const image_embedding = "original";
    std::string const feature_embedding = "original-sift";
    std::string const exif_embedding = "exif";

    /* Load scene. */
    mve::Scene::Ptr scene = mve::Scene::create(argv[1]);

    /* Feature computation for the scene. */
    sfm::bundler::Features::Options feature_opts;
    feature_opts.image_embedding = image_embedding;
    feature_opts.feature_embedding = feature_embedding;
    feature_opts.exif_embedding = exif_embedding;
    feature_opts.max_image_size = 4000000;
    feature_opts.skip_saving_views = false;
    feature_opts.force_recompute = false;

    std::cout << "Computing/loading image features..." << std::endl;
    sfm::bundler::ViewportList viewports;
    sfm::bundler::Features bundler_features(feature_opts);
    bundler_features.compute(scene, sfm::bundler::Features::SIFT_FEATURES,
        &viewports);

    std::cout << "Viewport statistics:" << std::endl;
    for (std::size_t i = 0; i < viewports.size(); ++i)
    {
        sfm::bundler::Viewport const& view = viewports[i];
        std::cout << "  View " << i << ": "
            << view.width << "x" << view.height << ", "
            << view.positions.size() << " features, "
            << "focal length: " << view.focal_length << std::endl;
    }

    /* Exhaustive matching between all pairs of views. */
    sfm::bundler::PairwiseMatching pairwise_matching;
    std::string matching_file = scene->get_path() + "/matching.bin";
    if (util::fs::file_exists(matching_file.c_str()))
    {
        std::cout << "Loading matching result from: " << matching_file << std::endl;
        sfm::bundler::load_pairwise_matching(matching_file, &pairwise_matching);
    }
    else
    {
        sfm::bundler::Matching::Options matching_opts;
        matching_opts.matching_opts.descriptor_length = 128; // TODO
        matching_opts.ransac_opts.already_normalized = false;
        matching_opts.ransac_opts.threshold = 3.0f;
        matching_opts.ransac_opts.verbose_output = false;

        std::cout << "Performing exhaustive feature matching..." << std::endl;
        sfm::bundler::Matching bundler_matching(matching_opts);
        bundler_matching.compute(viewports, &pairwise_matching);

        std::cout << "Saving matching result to: " << matching_file << std::endl;
        sfm::bundler::save_pairwise_matching(pairwise_matching, matching_file);
    }

    /* For every viewport drop descriptor information to save memory. */
    for (std::size_t i = 0; i < viewports.size(); ++i)
        viewports[i].descr_data.deallocate();

    /* Find initial pair. */
    sfm::bundler::InitialPair::Options init_pair_opts;
    init_pair_opts.verbose_output = true;
    init_pair_opts.max_homography_inliers = 0.4f;
    init_pair_opts.homography_opts.max_iterations = 1000;
    init_pair_opts.homography_opts.already_normalized = false;
    init_pair_opts.homography_opts.threshold = 3.0f;
    init_pair_opts.homography_opts.verbose_output = false;
    sfm::bundler::InitialPair::Result init_pair_result;
    sfm::bundler::InitialPair init_pair(init_pair_opts);
    init_pair.compute(viewports, pairwise_matching, &init_pair_result);

    if (init_pair_result.view_1_id < 0 || init_pair_result.view_2_id < 0)
    {
        std::cerr << "Error finding initial pair, exiting!" << std::endl;
        return 1;
    }

    std::cout << "  Using views " << init_pair_result.view_1_id
        << " and " << init_pair_result.view_2_id
        << " as initial pair." << std::endl;

    /* Compute connected feature components, i.e. feature tracks. */
    std::cout << "Computing feature tracks..." << std::endl;
    sfm::bundler::Tracks::Options tracks_options;
    tracks_options.verbose_output = true;
    sfm::bundler::TrackList tracks;
    sfm::bundler::Tracks bundler_tracks(tracks_options);
    bundler_tracks.compute(pairwise_matching, &viewports, &tracks);
    std::cout << "Created a total of " << tracks.size()
        << " tracks." << std::endl;

    /* Clear pairwise matching to save memeory. */
    pairwise_matching.clear();

    /* Incrementally compute full bundle. */
    sfm::bundler::Incremental::Options incremental_opts;
    incremental_opts.fundamental_opts.already_normalized = false;
    incremental_opts.fundamental_opts.threshold = 4.0f;
    incremental_opts.fundamental_opts.max_iterations = 1000;
    incremental_opts.fundamental_opts.verbose_output = false;
    incremental_opts.pose_opts.threshold = 4.0f;
    incremental_opts.pose_opts.max_iterations = 1000;
    incremental_opts.pose_opts.verbose_output = true;
    incremental_opts.verbose_output = true;

    sfm::bundler::Incremental incremental(incremental_opts);
    incremental.initialize(&viewports, &tracks);

    /* Reconstruct pose for the initial pair. */
    std::cout << "Starting incremental bundle adjustment." << std::endl;
    std::cout << "  Computing pose for initial pair..." << std::endl;
    incremental.reconstruct_initial_pair(init_pair_result.view_1_id,
        init_pair_result.view_2_id);

    /* Reconstruct track positions with the intial pair. */
    std::cout << "  Triangulating new tracks..." << std::endl;
    incremental.triangulate_new_tracks();

    /* Run bundle adjustment. */
    std::cout << "  Running full bundle adjustment..." << std::endl;
    incremental.bundle_adjustment_full();

    /* Reconstruct remaining views. */
    while (true)
    {
#if 1
        static int tmp = 0;
        if (tmp == 1)
            break;
        tmp += 1;
#endif

        int next_view_id = incremental.find_next_view();
        if (next_view_id < 0)
            break;

        std::cout << "  Adding next view ID " << next_view_id << "..." << std::endl;
        incremental.reconstruct_next_view(next_view_id);

        //std::cout << "  Running single camera bundle adjustment..." << std::endl;
        //incremental.bundle_adjustment_single_cam(next_view_id);

        std::cout << "  Triangulating new tracks..." << std::endl;
        incremental.triangulate_new_tracks();

        std::cout << "  Running full bundle adjustment..." << std::endl;
        incremental.bundle_adjustment_full();
    }

    mve::Bundle::Ptr bundle = incremental.create_bundle();

    /* Save bundle file to scene. */
    mve::save_mve_bundle(bundle, scene->get_path() + "/synth_0.out");

    /* Apply bundle cameras to views. */
    mve::Bundle::Cameras const& bundle_cams = bundle->get_cameras();
    mve::Scene::ViewList const& views = scene->get_views();
    if (bundle_cams.size() != views.size())
    {
        std::cerr << "Error: Invalid number of cameras!" << std::endl;
        return 1;
    }

    for (std::size_t i = 0; i < bundle_cams.size(); ++i)
    {
        mve::View::Ptr view = views[i];
        mve::CameraInfo const& cam = bundle_cams[i];
        if (view->get_camera().flen == 0.0f && cam.flen == 0.0f)
            continue;

        view->set_camera(cam);
        std::cout << "Saving MVE view " << view->get_filename() << std::endl;
        view->save_mve_file();
    }

#if 0
    /* Visualizing tracks for debugging. */
    for (std::size_t i = 0; i < tracks.size(); ++i)
    {
        if (tracks[i].features.size() < 10)
            continue;

        std::cout << "Visualizing track " << i << "..." << std::endl;
        mve::ByteImage::Ptr img = sfm::bundler::visualize_track(tracks[i],
            viewports, scene, image_embedding, pairwise_matching);
        mve::image::save_file(img, "/tmp/track.png");

        std::cout << "Press ENTER to continue or STRG-C to cancel." << std::endl;
        std::string temp_line;
        std::getline(std::cin, temp_line);
    }
#endif

    return 0;
}
