/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

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

mve::ByteImage::Ptr
visualize_matching (sfm::Matching::Result const& matching,
    mve::ByteImage::Ptr image1, mve::ByteImage::Ptr image2,
    std::vector<math::Vec2f> const& pos1, std::vector<math::Vec2f> const& pos2)
{
    /* Visualize keypoints. */
    sfm::Correspondences2D2D vis_matches;
    for (std::size_t i = 0; i < matching.matches_1_2.size(); ++i)
    {
        if (matching.matches_1_2[i] < 0)
            continue;
        int const j = matching.matches_1_2[i];

        sfm::Correspondence2D2D match;
        std::copy(pos1[i].begin(), pos1[i].end(), match.p1);
        std::copy(pos2[j].begin(), pos2[j].end(), match.p2);
        vis_matches.push_back(match);
    }

    std::cout << "Drawing " << vis_matches.size() << " matches..." << std::endl;
    mve::ByteImage::Ptr match_image = sfm::Visualizer::draw_matches
        (image1, image2, vis_matches);
    return match_image;
}

#define DISCRETIZE_DESCRIPTORS 0
template <typename T>
void
convert_sift_descriptors(sfm::Sift::Descriptors const& sift_descr,
    util::AlignedMemory<math::Vector<T, 128> >* aligned_descr)
{
    aligned_descr->resize(sift_descr.size());
    T* data_ptr = aligned_descr->data()->begin();
    for (std::size_t i = 0; i < sift_descr.size(); ++i, data_ptr += 128)
    {
        sfm::Sift::Descriptor const& d = sift_descr[i];
#if DISCRETIZE_DESCRIPTORS
        for (int j = 0; j < 128; ++j)
        {
            float value = d.data[j];
            value = math::clamp(value, 0.0f, 1.0f);
            value = math::round(value * 255.0f);
            data_ptr[j] = static_cast<unsigned char>(value);
        }
#else
        std::copy(d.data.begin(), d.data.end(), data_ptr);
#endif
    }
}

template <typename T>
void
convert_surf_descriptors(sfm::Surf::Descriptors const& surf_descr,
    util::AlignedMemory<math::Vector<T, 64> >* aligned_descr)
{
    aligned_descr->resize(surf_descr.size());
    T* data_ptr = aligned_descr->data()->begin();
    for (std::size_t i = 0; i < surf_descr.size(); ++i, data_ptr += 64)
    {
        sfm::Surf::Descriptor const& d = surf_descr[i];
#if DISCRETIZE_DESCRIPTORS
        for (int j = 0; j < 64; ++j)
        {
            float value = d.data[j];
            value = math::clamp(value, -1.0f, 1.0f);
            value = math::round(value * 127.0f);
            data_ptr[j] = static_cast<signed char>(value);
        }
#else
        std::copy(d.data.begin(), d.data.end(), data_ptr);
#endif
    }
}

void
feature_set_matching (mve::ByteImage::Ptr image1, mve::ByteImage::Ptr image2)
{
    sfm::FeatureSet::Options feature_set_opts;
    feature_set_opts.feature_types = sfm::FeatureSet::FEATURE_ALL;
    feature_set_opts.sift_opts.verbose_output = true;
    feature_set_opts.surf_opts.verbose_output = true;
    feature_set_opts.surf_opts.contrast_threshold = 500.0f;

    sfm::FeatureSet feat1(feature_set_opts);
    feat1.compute_features(image1);
    sfm::FeatureSet feat2(feature_set_opts);
    feat2.compute_features(image2);

    sfm::Matching::Options sift_matching_opts;
    sift_matching_opts.lowe_ratio_threshold = 0.8f;
    sift_matching_opts.descriptor_length = 128;
    sift_matching_opts.distance_threshold = std::numeric_limits<float>::max();

#if DISCRETIZE_DESCRIPTORS
    util::AlignedMemory<math::Vec128us, 16> sift_descr1, sift_descr2;
#else
    util::AlignedMemory<math::Vec128f, 16> sift_descr1, sift_descr2;
#endif
    convert_sift_descriptors(feat1.sift_descriptors, &sift_descr1);
    convert_sift_descriptors(feat2.sift_descriptors, &sift_descr2);

    sfm::Matching::Result sift_matching;
    sfm::Matching::twoway_match(sift_matching_opts,
        sift_descr1.data()->begin(), sift_descr1.size(),
        sift_descr2.data()->begin(), sift_descr2.size(),
        &sift_matching);
    sfm::Matching::remove_inconsistent_matches(&sift_matching);
    std::cout << "Consistent Sift Matches: "
        << sfm::Matching::count_consistent_matches(sift_matching)
        << std::endl;

    sfm::Matching::Options surf_matching_opts;
    surf_matching_opts.lowe_ratio_threshold = 0.7f;
    surf_matching_opts.descriptor_length = 64;
    surf_matching_opts.distance_threshold = std::numeric_limits<float>::max();

#if DISCRETIZE_DESCRIPTORS
    util::AlignedMemory<math::Vec64s, 16> surf_descr1, surf_descr2;
#else
    util::AlignedMemory<math::Vec64f, 16> surf_descr1, surf_descr2;
#endif
    convert_surf_descriptors(feat1.surf_descriptors, &surf_descr1);
    convert_surf_descriptors(feat2.surf_descriptors, &surf_descr2);

    sfm::Matching::Result surf_matching;
    sfm::Matching::twoway_match(surf_matching_opts,
        surf_descr1.data()->begin(), surf_descr1.size(),
        surf_descr2.data()->begin(), surf_descr2.size(),
        &surf_matching);
    sfm::Matching::remove_inconsistent_matches(&surf_matching);
    std::cout << "Consistent Surf Matches: "
        << sfm::Matching::count_consistent_matches(surf_matching)
        << std::endl;

    sfm::Matching::Result matching;
    sfm::Matching::combine_results(sift_matching, surf_matching, &matching);

    std::cout << "Consistent Matches: "
        << sfm::Matching::count_consistent_matches(matching)
        << std::endl;

#if 1
    /* Draw features. */
    std::vector<sfm::Visualizer::Keypoint> features1;
    for (std::size_t i = 0; i < feat1.sift_descriptors.size(); ++i)
    {
        if (matching.matches_1_2[i] == -1)
            continue;

        sfm::Sift::Descriptor const& descr = feat1.sift_descriptors[i];
        sfm::Visualizer::Keypoint kp;
        kp.orientation = descr.orientation;
        kp.radius = descr.scale * 3.0f;
        kp.x = descr.x;
        kp.y = descr.y;
        features1.push_back(kp);
    }

    std::vector<sfm::Visualizer::Keypoint> features2;
    for (std::size_t i = 0; i < feat2.sift_descriptors.size(); ++i)
    {
        if (matching.matches_2_1[i] == -1)
            continue;

        sfm::Sift::Descriptor const& descr = feat2.sift_descriptors[i];
        sfm::Visualizer::Keypoint kp;
        kp.orientation = descr.orientation;
        kp.radius = descr.scale * 3.0f;
        kp.x = descr.x;
        kp.y = descr.y;
        features2.push_back(kp);
    }

    image1 = sfm::Visualizer::draw_keypoints(image1,
        features1, sfm::Visualizer::RADIUS_BOX_ORIENTATION);
    image2 = sfm::Visualizer::draw_keypoints(image2,
        features2, sfm::Visualizer::RADIUS_BOX_ORIENTATION);
#endif

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
        image1 = mve::image::rescale_half_size<uint8_t>(image1);
        //image1 = mve::image::rescale_half_size<uint8_t>(image1);
        //image1 = mve::image::rotate<uint8_t>(image1, mve::image::ROTATE_CCW);

        std::cout << "Loading " << argv[2] << "..." << std::endl;
        image2 = mve::image::load_file(argv[2]);
        image2 = mve::image::rescale_half_size<uint8_t>(image2);
        //image2 = mve::image::rescale_half_size<uint8_t>(image2);
        //image2 = mve::image::rotate<uint8_t>(image2, mve::image::ROTATE_CCW);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    feature_set_matching(image1, image2);
    return 0;
}
