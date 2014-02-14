#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

#include "mve/image.h"
#include "mve/image_io.h"
#include "mve/image_tools.h"
#include "sfm/bundler.h"

#define MAX_RES 1000000

mve::ByteImage::Ptr
load_image (std::string const& filename)
{
    std::cout << "Loading " << filename << "..." << std::endl;
    mve::ByteImage::Ptr image = mve::image::load_file(filename);

    while (image->width() * image->height() > MAX_RES)
        image = mve::image::rescale_half_size<uint8_t>(image);
    return image;
}

int
main (int argc, char** argv)
{
    if (argc <= 1)
    {
        std::cerr << "Syntax: " << argv[0] << " <images>" << std::endl;
        return 1;
    }

    std::vector<std::string> filenames;
    for (int i = 1; i < argc; ++i)
        filenames.push_back(argv[i]);

    sfm::Bundler::Options options;
    options.sift_matching_options.descriptor_length = 128;
    options.sift_matching_options.lowe_ratio_threshold = 0.8f;
    options.sift_matching_options.distance_threshold = 0.7f;
    options.ransac_fundamental_options.max_iterations = 1000;
    options.ransac_fundamental_options.threshold = 2.0f;
    options.ransac_fundamental_options.already_normalized = false;

    sfm::Bundler bundler(options);
    try
    {
        for (std::size_t i = 0; i < filenames.size(); ++i)
            bundler.add_image(load_image(filenames[i]), 31.0 / 35.0);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    bundler.create_bundle();

    return 0;
}
