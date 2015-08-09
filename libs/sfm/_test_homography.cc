/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>

#include "util/aligned_memory.h"
#include "mve/image.h"
#include "mve/image_io.h"
#include "mve/image_tools.h"
#include "sfm/sift.h"
#include "sfm/matching.h"
#include "sfm/correspondence.h"
#include "sfm/ransac_fundamental.h"
#include "sfm/ransac_homography.h"
#include "sfm/visualizer.h"

#define MAX_PIXELS 1000000

int
main (int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Syntax: " << argv[0] << " <img1> <img2>" << std::endl;
        return 1;
    }

    /* Load images. */
    std::string fname1(argv[1]);
    std::string fname2(argv[2]);
    std::cout << "Loading " << fname1 << "..." << std::endl;
    mve::ByteImage::Ptr img1 = mve::image::load_file(fname1);
    std::cout << "Loading " << fname2 << "..." << std::endl;
    mve::ByteImage::Ptr img2 = mve::image::load_file(fname2);

    /* Limit size of the images. */
    while (img1->get_pixel_amount() > MAX_PIXELS)
        img1 = mve::image::rescale_half_size<uint8_t>(img1);
    while (img2->get_pixel_amount() > MAX_PIXELS)
        img2 = mve::image::rescale_half_size<uint8_t>(img2);

    /* Compute SIFT descriptors. */
    sfm::Sift::Descriptors img1_desc, img2_desc;
    {
        sfm::Sift::Options sift_opts;
        sift_opts.verbose_output = true;
        sfm::Sift sift(sift_opts);

        sift.set_image(img1);
        sift.process();
        img1_desc = sift.get_descriptors();

        sift.set_image(img2);
        sift.process();
        img2_desc = sift.get_descriptors();
    }

    std::cout << "Image 1 (" << img1->width() << "x" << img1->height() << ") "
        << img1_desc.size() << " descriptors." << std::endl;
    std::cout << "Image 2 (" << img2->width() << "x" << img2->height() << ") "
        << img2_desc.size() << " descriptors." << std::endl;

    /* Prepare matching data. */
    util::AlignedMemory<float> descr1, descr2;
    descr1.allocate(128 * sizeof(float) * img1_desc.size());
    descr2.allocate(128 * sizeof(float) * img2_desc.size());
    float* data_ptr = descr1.begin();
    for (std::size_t i = 0; i < img1_desc.size(); ++i, data_ptr += 128)
    {
        sfm::Sift::Descriptor const& d = img1_desc[i];
        std::copy(d.data.begin(), d.data.end(), data_ptr);
    }
    data_ptr = descr2.begin();
    for (std::size_t i = 0; i < img2_desc.size(); ++i, data_ptr += 128)
    {
        sfm::Sift::Descriptor const& d = img2_desc[i];
        std::copy(d.data.begin(), d.data.end(), data_ptr);
    }

    /* Compute matching. */
    sfm::Matching::Options matching_opts;
    matching_opts.descriptor_length = 128;
    matching_opts.distance_threshold = 1.0f;
    matching_opts.lowe_ratio_threshold = 0.8f;
    sfm::Matching::Result matching_result;
    sfm::Matching::twoway_match(matching_opts,
        descr1.begin(), img1_desc.size(),
        descr2.begin(), img2_desc.size(), &matching_result);
    sfm::Matching::remove_inconsistent_matches(&matching_result);

    std::cout << "Found " << sfm::Matching::count_consistent_matches
        (matching_result) << " consistent matches." << std::endl;

    /* Setup correspondences. */
    sfm::Correspondences corr_all;
    std::vector<int> const& m12 = matching_result.matches_1_2;
    for (std::size_t i = 0; i < m12.size(); ++i)
    {
        if (m12[i] < 0)
            continue;

        sfm::Sift::Descriptor const& d1 = img1_desc[i];
        sfm::Sift::Descriptor const& d2 = img2_desc[m12[i]];
        sfm::Correspondence c2d2d;
        c2d2d.p1[0] = d1.x; c2d2d.p1[1] = d1.y;
        c2d2d.p2[0] = d2.x; c2d2d.p2[1] = d2.y;
        corr_all.push_back(c2d2d);
    }

    /* Fundamental RANSAC. */
    std::cout << "RANSAC for fundamental matrix..." << std::endl;
    sfm::RansacFundamental::Options ransac_fundamental_opts;
    ransac_fundamental_opts.max_iterations = 1000;
    ransac_fundamental_opts.threshold = 2.0f;
    ransac_fundamental_opts.verbose_output = true;
    sfm::RansacFundamental ransac_fundamental(ransac_fundamental_opts);
    sfm::RansacFundamental::Result ransac_fundamental_result;
    ransac_fundamental.estimate(corr_all, &ransac_fundamental_result);

    /* Setup filtered correspondences. */
    sfm::Correspondences corr_f;
    for (std::size_t i = 0; i < ransac_fundamental_result.inliers.size(); ++i)
        corr_f.push_back(corr_all[ransac_fundamental_result.inliers[i]]);

    /* Homography RANSAC. */
    std::cout << "RANSAC for homography matrix..." << std::endl;
    sfm::RansacHomography::Options ransac_homography_opts;
    ransac_homography_opts.max_iterations = 1000;
    ransac_homography_opts.threshold = 2.0f;
    ransac_homography_opts.verbose_output = true;
    sfm::RansacHomography ransac_homography(ransac_homography_opts);
    sfm::RansacHomography::Result ransac_homography_result;
    ransac_homography.estimate(corr_all, &ransac_homography_result);

    /* Setup filtered correspondences. */
    sfm::Correspondences corr_h;
    for (std::size_t i = 0; i < ransac_homography_result.inliers.size(); ++i)
        corr_h.push_back(corr_all[ransac_homography_result.inliers[i]]);

    /* Visualize matches. */
    mve::ByteImage::Ptr vis_img;
    vis_img = sfm::Visualizer::draw_matches(img1, img2, corr_all);
    mve::image::save_png_file(vis_img, "/tmp/matches_unfiltered.png");
    vis_img = sfm::Visualizer::draw_matches(img1, img2, corr_f);
    mve::image::save_png_file(vis_img, "/tmp/matches_fundamental.png");
    vis_img = sfm::Visualizer::draw_matches(img1, img2, corr_h);
    mve::image::save_png_file(vis_img, "/tmp/matches_homography.png");

    return 0;
}
