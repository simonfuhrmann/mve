#include <iostream>
#include <fstream>

#include "util/aligned_memory.h"
#include "util/timer.h"
#include "mve/image.h"
#include "mve/image_tools.h"
#include "mve/image_io.h"

#include "sfm/surf.h"
#include "sfm/sift.h"
#include "sfm/feature_set.h"
#include "sfm/matching.h"
#include "sfm/visualizer.h"

uint8_t const color_black[] = { 0, 0, 0 };

template <typename DESCR, int DIM>
sfm::Matching::Result
twoview_matching (std::vector<DESCR> const& descr1,
    std::vector<DESCR> const& descr2)
{
    /* Convert the descriptors to aligned float arrays. */
    util::AlignedMemory<float, 16> matchset_1(DIM * descr1.size());
    float* out_ptr1 = matchset_1.begin();
    for (std::size_t i = 0; i < descr1.size(); ++i, out_ptr1 += DIM)
        std::copy(descr1[i].data.begin(), descr1[i].data.end(), out_ptr1);

    util::AlignedMemory<float, 16> matchset_2(DIM * descr2.size());
    float* out_ptr2 = matchset_2.begin();
    for (std::size_t i = 0; i < descr2.size(); ++i, out_ptr2 += DIM)
        std::copy(descr2[i].data.begin(), descr2[i].data.end(), out_ptr2);

    /* Perform matching. */
    sfm::Matching::Options matchopts;
    matchopts.descriptor_length = DIM;
    matchopts.lowe_ratio_threshold = 0.8f;
    matchopts.distance_threshold = std::numeric_limits<float>::max();

    util::WallTimer timer;
    sfm::Matching::Result matching;
    sfm::Matching::twoway_match(matchopts, matchset_1.begin(), descr1.size(),
        matchset_2.begin(), descr2.size(), &matching);
    sfm::Matching::remove_inconsistent_matches(&matching);
    std::cout << "Two-view matching took " << timer.get_elapsed() << "ms." << std::endl;

    std::cout << "Kept " << sfm::Matching::count_consistent_matches(matching)
        << " good matches." << std::endl;

    return matching;
}

template <typename DESCR>
mve::ByteImage::Ptr
visualize_matching (sfm::Matching::Result const& matching,
    mve::ByteImage::Ptr image1, mve::ByteImage::Ptr image2,
    std::vector<DESCR> const& descr1, std::vector<DESCR> const& descr2)
{
#if 0
    /* Draw features. */
    std::vector<sfm::Visualizer::Keypoint> features1;
    for (std::size_t i = 0; i < descr1.size(); ++i)
    {
        sfm::Visualizer::Keypoint kp;
        kp.orientation = descr1[i].orientation;
        kp.radius = descr1[i].scale * 3.0f;
        kp.x = descr1[i].x;
        kp.y = descr1[i].y;
        features1.push_back(kp);
    }
    image1 = sfm::Visualizer::draw_keypoints(image1,
        features1, sfm::Visualizer::RADIUS_BOX_ORIENTATION);

    std::vector<sfm::Visualizer::Keypoint> features2;
    for (std::size_t i = 0; i < descr2.size(); ++i)
    {
        sfm::Visualizer::Keypoint kp;
        kp.orientation = descr2[i].orientation;
        kp.radius = descr2[i].scale * 3.0f;
        kp.x = descr2[i].x;
        kp.y = descr2[i].y;
        features2.push_back(kp);
    }
    image2 = sfm::Visualizer::draw_keypoints(image2,
        features2, sfm::Visualizer::RADIUS_BOX_ORIENTATION);
#endif

    /* Visualize matches. */
    sfm::Correspondences vis_matches;
    for (std::size_t i = 0; i < matching.matches_1_2.size(); ++i)
    {
        if (matching.matches_1_2[i] < 0)
            continue;
        int j = matching.matches_1_2[i];

        sfm::Correspondence match;
        match.p1[0] = descr1[i].x;
        match.p1[1] = descr1[i].y;
        match.p2[0] = descr2[j].x;
        match.p2[1] = descr2[j].y;
        vis_matches.push_back(match);
    }

    std::cout << "Drawing " << vis_matches.size() << " matches..." << std::endl;
    mve::ByteImage::Ptr match_image = sfm::Visualizer::draw_matches
        (image1, image2, vis_matches);
    return match_image;
}

mve::ByteImage::Ptr
visualize_matching (sfm::Matching::Result const& matching,
    mve::ByteImage::Ptr image1, mve::ByteImage::Ptr image2,
    std::vector<math::Vec2f> const& pos1, std::vector<math::Vec2f> const& pos2)
{
    /* Visualize keypoints. */
    sfm::Correspondences vis_matches;
    for (std::size_t i = 0; i < matching.matches_1_2.size(); ++i)
    {
        if (matching.matches_1_2[i] < 0)
            continue;
        int const j = matching.matches_1_2[i];

        sfm::Correspondence match;
        std::copy(pos1[i].begin(), pos1[i].end(), match.p1);
        std::copy(pos2[j].begin(), pos2[j].end(), match.p2);
        vis_matches.push_back(match);
    }

    std::cout << "Drawing " << vis_matches.size() << " matches..." << std::endl;
    mve::ByteImage::Ptr match_image = sfm::Visualizer::draw_matches
        (image1, image2, vis_matches);
    return match_image;
}

void
sift_matching (mve::ByteImage::Ptr image1, mve::ByteImage::Ptr image2)
{
    std::cout << std::endl << "SIFT matching..." << std::endl;
    sfm::Sift::Descriptors descr1;
    {
        sfm::Sift::Options sift_options;
        sfm::Sift sift(sift_options);
        sift.set_image(image1);
        sift.process();
        descr1 = sift.get_descriptors();
    }

    sfm::Sift::Descriptors descr2;
    {
        sfm::Sift::Options sift_options;
        sfm::Sift sift(sift_options);
        sift.set_image(image2);
        sift.process();
        descr2 = sift.get_descriptors();
    }

    sfm::Matching::Result matching
        = twoview_matching<sfm::Sift::Descriptor, 128>(descr1, descr2);

    std::cout << "SIFT matching result: " << matching.matches_1_2.size() << " / " << matching.matches_2_1.size() << std::endl;

    mve::ByteImage::Ptr match_image
        = visualize_matching(matching, image1, image2, descr1, descr2);
    std::string output_filename = "/tmp/matching_sift.png";
    std::cout << "Saving visualization to " << output_filename << std::endl;
    mve::image::save_file(match_image, output_filename);
}

void
surf_matching (mve::ByteImage::Ptr image1, mve::ByteImage::Ptr image2)
{
    std::cout << std::endl << "SURF matching..." << std::endl;
    sfm::Surf::Descriptors descr1;
    {
        sfm::Surf::Options surf_opts;
        surf_opts.contrast_threshold = 500.0f;
        sfm::Surf surf(surf_opts);
        surf.set_image(image1);
        surf.process();
        descr1 = surf.get_descriptors();
    }

    sfm::Surf::Descriptors descr2;
    {
        sfm::Surf::Options surf_opts;
        surf_opts.contrast_threshold = 500.0f;
        sfm::Surf surf(surf_opts);
        surf.set_image(image2);
        surf.process();
        descr2 = surf.get_descriptors();
    }

    sfm::Matching::Result matching
        = twoview_matching<sfm::Surf::Descriptor, 64>(descr1, descr2);

    std::cout << "SURF matching result: " << matching.matches_1_2.size() << " / " << matching.matches_2_1.size() << std::endl;

    mve::ByteImage::Ptr match_image
        = visualize_matching(matching, image1, image2, descr1, descr2);
    std::string output_filename = "/tmp/matching_surf.png";
    std::cout << "Saving visualization to " << output_filename << std::endl;
    mve::image::save_file(match_image, output_filename);
}

void
feature_set_matching (mve::ByteImage::Ptr image1, mve::ByteImage::Ptr image2)
{
    std::cout << std::endl << "Feature set matching..." << std::endl;
    sfm::FeatureSet::Options opts;
    opts.feature_types = sfm::FeatureSet::FEATURE_ALL;
    opts.surf_opts.contrast_threshold = 500.0f;
    opts.sift_matching_opts.lowe_ratio_threshold = 0.9f;
    opts.sift_matching_opts.descriptor_length = 128;
    opts.surf_matching_opts.lowe_ratio_threshold = 0.9f;
    opts.surf_matching_opts.descriptor_length = 64;

    sfm::FeatureSet feat1(opts);
    feat1.compute_features(image1);
    sfm::FeatureSet feat2(opts);
    feat2.compute_features(image2);

    sfm::Matching::Result matching;
    feat1.match(feat2, &matching);

    std::cout << "Consistent Matches: "
        << sfm::Matching::count_consistent_matches(matching)
        << std::endl;

    std::cout << "FS matching result: " << matching.matches_1_2.size() << " / " << matching.matches_2_1.size() << std::endl;

    mve::ByteImage::Ptr match_image = visualize_matching(
        matching, image1, image2, feat1.positions, feat2.positions);
    std::string output_filename = "/tmp/matching_featureset.png";
    std::cout << "Saving visualization to " << output_filename << std::endl;
    mve::image::save_file(match_image, output_filename);
}

int
main (int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Syntax: " << argv[0] << " image1 image2" << std::endl;
        return 1;
    }

#ifdef __SSE2__
    std::cout << "SSE2 is enabled!" << std::endl;
#endif
#ifdef __SSE3__
    std::cout << "SSE3 is enabled!" << std::endl;
#endif

    /* Regular two-view matching. */
    mve::ByteImage::Ptr image1, image2;
    try
    {
        std::cout << "Loading " << argv[1] << "..." << std::endl;
        image1 = mve::image::load_file(argv[1]);
        std::cout << "Loading " << argv[2] << "..." << std::endl;
        image2 = mve::image::load_file(argv[2]);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    sift_matching(image1, image2);
    surf_matching(image1, image2);
    feature_set_matching(image1, image2);

#if 0
    /* Benchmarking. */
    image1 = mve::image::crop<uint8_t>(image1, image1->width() + 200, image1->height() + 200, -100, -100, color_black);
    image2 = mve::image::crop<uint8_t>(image2, image2->width() + 200, image2->height() + 200, -100, -100, color_black);
    mve::image::save_file(image1, "/tmp/testcropped.png");
    std::ofstream gnuplot_data("/tmp/gnuplot-sift2.txt");
    for (int i = 0; i < 360; i += 5)
    {
        mve::ByteImage::Ptr image2b = mve::image::rotate<uint8_t>(image2, i * MATH_PI / 180.0, color_black);

#if 0
        sfm::Surf::Descriptors descr1, descr2;
        sfm::Surf surf;
        surf.set_contrast_threshold(500.0f);
        surf.set_image(image1);
        surf.process();
        descr1 = surf.get_descriptors();
        surf.set_image(image2b);
        surf.process();
        descr2 = surf.get_descriptors();

        sfm::MatchingResult matching
            = twoview_matching<sfm::Surf::Descriptor, 64>(descr1, descr2);
#else
        sfm::Sift::Descriptors descr1, descr2;
        sfm::Sift sift;
        sift.set_min_max_octave(0, 4);
        sift.set_image(image1);
        sift.process();
        descr1 = sift.get_descriptors();
        sift.set_image(image2b);
        sift.process();
        descr2 = sift.get_descriptors();

        sfm::MatchingResult matching
            = twoview_matching<sfm::Sift::Descriptor, 128>(descr1, descr2);
#endif
        //std::string filename = "/tmp/featurematching_" + util::string::get_filled(i, 3, '0') + ".png";
        //mve::image::save_file(visualize_matching(matching, image1, image2b, descr1, descr2), filename);

        int num_matches = 0;
        for (std::size_t i = 0; i < matching.matches_1_2.size(); ++i)
            num_matches += matching.matches_1_2[i] < 0 ? 0 : 1;
        gnuplot_data << i << " " << num_matches << std::endl;
    }
    gnuplot_data.close();

#endif

    return 0;
}
