#include <iostream>
#include <string>

#include "util/file_system.h"
#include "mve/scene.h"
#include "mve/image_io.h"
#include "mve/image_tools.h"
#include "sfm/sift.h"
#include "sfm/bundler_common.h"
#include "sfm/bundler_features.h"
#include "sfm/bundler_matching.h"
#include "sfm/bundler_tracks.h"
#include "sfm/bundler_init_pair.h"

int
main (int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Syntax: " << argv[0] << " <scene>" << std::endl;
        return 1;
    }

    std::string const image_embedding = "original";
    std::string const feature_embedding = "original-sift";

    /* Load scene. */
    mve::Scene::Ptr scene = mve::Scene::create(argv[1]);

    /* Feature computation for the scene. */
    sfm::bundler::Features::Options feature_opts;
    feature_opts.image_embedding = image_embedding;
    feature_opts.feature_embedding = feature_embedding;
    feature_opts.max_image_size = 2000000;
    feature_opts.skip_saving_views = false;
    feature_opts.force_recompute = false;

    std::cout << "Computing/loading image features..." << std::endl;
    sfm::bundler::ViewportList viewports;
    sfm::bundler::Features bundler_features(feature_opts);
    bundler_features.compute(scene, sfm::bundler::Features::SIFT_FEATURES,
        &viewports);

    /* TODO Initialize focal length. */

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

    /* Compute connected feature components, i.e. feature tracks. */
    sfm::bundler::Tracks::Options tracks_options;

    std::cout << "Computing feature tracks..." << std::endl;
    sfm::bundler::TrackList tracks;
    sfm::bundler::Tracks bundler_tracks(tracks_options);
    bundler_tracks.compute(pairwise_matching, &viewports, &tracks);

    std::cout << "Created a total of " << tracks.size()
        << " tracks." << std::endl;

    /* Find initial pair. */
    sfm::bundler::InitialPair::Options init_pair_opts;
    init_pair_opts.verbose_output = true;
    sfm::bundler::InitialPair::Result init_pair_result;
    sfm::bundler::InitialPair init_pair(init_pair_opts);
    init_pair.compute(viewports, pairwise_matching, &init_pair_result);

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
