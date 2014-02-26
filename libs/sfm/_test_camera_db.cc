#include <iostream>
#include <string>

#include "mve/image.h"
#include "mve/image_io.h"
#include "mve/image_exif.h"
#include "sfm/camera_database.h"
#include "sfm/extract_focal_length.h"

int main (void)
{
    //std::string filename = "/gris/gris-f/projects/mvs_datasets/inputimages/mathilde_hass/IMG_0140.JPG";
    std::string filename = "/tmp/Nexus-4-camera-sample-2.jpg";
    std::cout << filename << std::endl;
    std::string exif_str;
    mve::ByteImage::Ptr img = mve::image::load_jpg_file(filename, &exif_str);
    mve::image::ExifInfo exif = mve::image::exif_extract(exif_str.c_str(), exif_str.size(), false);


    std::pair<float, sfm::FocalLengthMethod> fl;
    fl = sfm::extract_focal_length(exif);
    std::cout << fl.first << " " << fl.second << std::endl;

    return 1;
}
