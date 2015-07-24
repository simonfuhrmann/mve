/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>
#include <string>

#include "mve/image.h"
#include "mve/image_io.h"
#include "mve/image_exif.h"
#include "sfm/camera_database.h"
#include "sfm/extract_focal_length.h"

int main (void)
{
    std::string filename = "/tmp/Nexus-4-camera-sample-2.jpg";
    std::cout << filename << std::endl;
    std::string exif_str;
    mve::image::load_jpg_file(filename, &exif_str);
    mve::image::ExifInfo exif = mve::image::exif_extract(exif_str.c_str(), exif_str.size(), false);

    sfm::FocalLengthEstimate fl = sfm::extract_focal_length(exif);
    std::cout << fl.first << " " << fl.second << std::endl;

    return 1;
}
