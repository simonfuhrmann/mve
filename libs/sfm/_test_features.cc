/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>

#include "util/file_system.h"
#include "util/timer.h"
#include "mve/image.h"
#include "mve/image_tools.h"
#include "mve/image_io.h"

#include "sfm/surf.h"
#include "sfm/sift.h"
#include "sfm/visualizer.h"

bool
sift_compare (sfm::Sift::Descriptor const& d1, sfm::Sift::Descriptor const& d2)
{
    return d1.scale > d2.scale;
}

bool
surf_compare (sfm::Surf::Descriptor const& d1, sfm::Surf::Descriptor const& d2)
{
    return d1.scale > d2.scale;
}

int
main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Syntax: " << argv[0] << " <image>" << std::endl;
        return 1;
    }

    /* Load image. */
    mve::ByteImage::Ptr image;
    std::string image_filename = argv[1];
    try
    {
        std::cout << "Loading " << image_filename << "..." << std::endl;
        image = mve::image::load_file(image_filename);
        //image = mve::image::rescale_half_size<uint8_t>(image);
        //image = mve::image::rescale_half_size<uint8_t>(image);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    /* Perform SURF feature detection. */
    sfm::Surf::Descriptors surf_descr;
    sfm::Surf::Keypoints surf_keypoints;
    {
        sfm::Surf::Options surf_options;
        surf_options.verbose_output = true;
        surf_options.debug_output = true;
        sfm::Surf surf(surf_options);
        surf.set_image(image);

        util::WallTimer timer;
        surf.process();
        std::cout << "Computed SURF features in "
            << timer.get_elapsed() << "ms." << std::endl;

        surf_descr = surf.get_descriptors();
        surf_keypoints = surf.get_keypoints();
    }

    /* Perform SIFT feature detection. */
    sfm::Sift::Descriptors sift_descr;
    sfm::Sift::Keypoints sift_keypoints;
    {
        sfm::Sift::Options sift_options;
        sift_options.verbose_output = true;
        sift_options.debug_output = true;
        sfm::Sift sift(sift_options);
        sift.set_image(image);

        util::WallTimer timer;
        sift.process();
        std::cout << "Computed SIFT features in "
            << timer.get_elapsed() << "ms." << std::endl;

        sift_descr = sift.get_descriptors();
        sift_keypoints = sift.get_keypoints();
    }

    //std::sort(sift_descr.begin(), sift_descr.end(), sift_compare);
    //std::sort(surf_descr.begin(), surf_descr.end(), surf_compare);
    //std::random_shuffle(sift_descr.begin(), sift_descr.end());
    //std::random_shuffle(surf_descr.begin(), surf_descr.end());
    //sift_descr.resize(500);
    //surf_descr.resize(500);

    /* Draw features. */
    std::vector<sfm::Visualizer::Keypoint> surf_drawing;
    for (std::size_t i = 0; i < surf_descr.size(); ++i)
    {
        sfm::Visualizer::Keypoint kp;
        kp.orientation = surf_descr[i].orientation;
        kp.radius = surf_descr[i].scale;
        kp.x = surf_descr[i].x;
        kp.y = surf_descr[i].y;
        surf_drawing.push_back(kp);
    }

    std::vector<sfm::Visualizer::Keypoint> sift_drawing;
    for (std::size_t i = 0; i < sift_descr.size(); ++i)
    {
        sfm::Visualizer::Keypoint kp;
        kp.orientation = sift_descr[i].orientation;
        kp.radius = sift_descr[i].scale;
        kp.x = sift_descr[i].x;
        kp.y = sift_descr[i].y;
        sift_drawing.push_back(kp);
    }

    mve::ByteImage::Ptr surf_image = sfm::Visualizer::draw_keypoints(image,
        surf_drawing, sfm::Visualizer::RADIUS_BOX_ORIENTATION);
    mve::ByteImage::Ptr sift_image = sfm::Visualizer::draw_keypoints(image,
        sift_drawing, sfm::Visualizer::RADIUS_BOX_ORIENTATION);

    /* Save the two images for SIFT and SURF. */
    std::string surf_out_fname = "/tmp/" + util::fs::replace_extension
        (util::fs::basename(image_filename), "surf.png");
    std::string sift_out_fname = "/tmp/" + util::fs::replace_extension
        (util::fs::basename(image_filename), "sift.png");

    std::cout << "Writing output file: " << surf_out_fname << std::endl;
    mve::image::save_file(surf_image, surf_out_fname);
    std::cout << "Writing output file: " << sift_out_fname << std::endl;
    mve::image::save_file(sift_image, sift_out_fname);

    return 0;
}
