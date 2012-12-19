#include <iostream>

#include "util/timer.h"
#include "mve/image.h"
#include "mve/imagetools.h"
#include "mve/imagefile.h"

#include "surf.h"
#include "sift.h"

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
    try
    {
        std::cout << "Loading " << argv[1] << "..." << std::endl;
        image = mve::image::load_file(argv[1]);
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
        sfm::Surf surf;
        surf.set_image(image);
        surf.set_contrast_threshold(500);
        surf.set_upright_descriptor(false);

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
        sfm::Sift sift;
        sift.set_image(image);

        util::WallTimer timer;
        sift.process();
        std::cout << "Computed SIFT features in "
            << timer.get_elapsed() << "ms." << std::endl;

        sift_descr = sift.get_descriptors();
        sift_keypoints = sift.get_keypoints();
    }

    // TODO: Create common feature drawing code, remove impl. specific code.
    // TODO: Save two images for SIFT and SURF.

    return 0;
}
