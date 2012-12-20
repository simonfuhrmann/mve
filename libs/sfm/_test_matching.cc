#include <iostream>
#include <fstream>

#include "util/alignedmemory.h"
#include "util/timer.h"
#include "mve/image.h"
#include "mve/imagetools.h"
#include "mve/imagefile.h"

#include "surf.h"
#include "matching.h"
#include "matchingvisualizer.h"

uint8_t const color_black[] = { 0, 0, 0 };

void
two_view_matching (mve::ByteImage::Ptr image1, mve::ByteImage::Ptr image2,
    sfm::Surf::Descriptors* descr1, sfm::Surf::Descriptors* descr2,
    sfm::MatchingResult* matching)
{
    /* Run SURF feature detection */
    {
        sfm::Surf surf;
        surf.set_contrast_threshold(6000.0f);

        surf.set_image(image1);
        surf.process();
        *descr1 = surf.get_descriptors();
        //surf.draw_features(image1);
        //mve::image::save_file(image1, "/tmp/surf-features-1.png");

        surf.set_image(image2);
        surf.process();
        *descr2 = surf.get_descriptors();
        //surf.draw_features(image2);
        //mve::image::save_file(image2, "/tmp/surf-features-2.png");
    }

    /*
     * Convert the descriptors to aligned short arrays.
     */
    util::AlignedMemory<short, 16> match_descr_1(64 * descr1->size());
    util::AlignedMemory<short, 16> match_descr_2(64 * descr2->size());

    short* mem_ptr = match_descr_1.begin();
    for (std::size_t i = 0; i < descr1->size(); ++i)
    {
        std::vector<float> const& d = descr1->at(i).data;
        for (int j = 0; j < 64; ++j)
            *(mem_ptr++) = static_cast<short>(d[j] * 127.0f);
    }
    mem_ptr = match_descr_2.begin();
    for (std::size_t i = 0; i < descr2->size(); ++i)
    {
        std::vector<float> const& d = descr2->at(i).data;
        for (int j = 0; j < 64; ++j)
            *(mem_ptr++) = static_cast<short>(d[j] * 127.0f);
    }

    /* Perform matching. */
    sfm::MatchingOptions matchopts;
    matchopts.descriptor_length = 64;
    matchopts.lowe_ratio_threshold = 0.9f;
    matchopts.distance_threshold = 3000.0f;

    util::WallTimer timer;
    sfm::match_features(matchopts, match_descr_1.begin(), descr1->size(),
        match_descr_2.begin(), descr2->size(), matching);
    sfm::remove_inconsistent_matches(matching);
    std::cout << "Two-view matching took " << timer.get_elapsed() << "ms." << std::endl;

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

#if 0
    /* Regular two-view matching. */
    mve::ByteImage::Ptr image1, image2;
    try
    {
        std::cout << "Loading " << argv[1] << "..." << std::endl;
        image1 = mve::image::load_file(argv[1]);
        std::cout << "Loading " << argv[2] << "..." << std::endl;
        image2 = mve::image::load_file(argv[2]);
        image2 = mve::image::rescale_double_size_supersample<unsigned char>(image2);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    /* Perform two-view matching. */
    sfm::Surf::SurfDescriptors descr1, descr2;
    sfm::MatchingResult matchresult;
    two_view_matching(image1, image2, &descr1, &descr2, &matchresult);
    //visualize_matches("/tmp/featurematching.png", image1, image2, descr1, descr2, matchresult);

#else
    /* Benchmarking. */
    mve::ByteImage::Ptr image1, image2;
    try
    {
        std::cout << "Loading " << argv[1] << "..." << std::endl;
        image1 = mve::image::load_file(argv[1]);
        image1 = mve::image::crop<uint8_t>(image1, image1->width() + 200, image1->height() + 200, -100, -100, color_black);
        std::cout << "Done loading images." << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::ofstream gnuplot_data("/tmp/gnuplot-data.txt");
    for (int i = 0; i < 360; i += 5)
    {
        image2 = mve::image::rotate<uint8_t>(image1, i * MATH_PI / 180.0, color_black);

        sfm::Surf::Descriptors descr1, descr2;
        sfm::MatchingResult matchresult;
        two_view_matching(image1, image2, &descr1, &descr2, &matchresult);
        //std::string filename = "/tmp/featurematching_" + util::string::get_filled(i, 3, '0') + ".png";
        //visualize_matches(filename, image1, image2, descr1, descr2, matchresult);

        int num_matches = 0;
        for (std::size_t i = 0; i < matchresult.matches_1_2.size(); ++i)
            if (matchresult.matches_1_2[i] >= 0)
                num_matches += 1;
        gnuplot_data << i << " " << num_matches << std::endl;
    }
    gnuplot_data.close();

#endif


    return 0;
}
