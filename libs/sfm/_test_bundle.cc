#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

#include "mve/image.h"
#include "mve/image_io.h"
#include "sfm/bundler.h"

mve::ByteImage::Ptr
load_image (std::string const& filename)
{
    std::cout << "Loading " << filename << "..." << std::endl;
    return mve::image::load_file(filename);
}

int
main (void)
{
    std::vector<std::string> filenames;
    filenames.push_back("/Users/sfuhrmann/Downloads/Fountain-R25/0011.jpg");
    filenames.push_back("/Users/sfuhrmann/Downloads/Fountain-R25/0013.jpg");
    filenames.push_back("/Users/sfuhrmann/Downloads/Fountain-R25/0015.jpg");

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
        bundler.add_image(load_image(filenames[0]), 31.0 / 35.0);
        bundler.add_image(load_image(filenames[1]), 31.0 / 35.0);
        bundler.add_image(load_image(filenames[2]), 31.0 / 35.0);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    bundler.create_bundle();

    return 0;
}
