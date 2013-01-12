#include <iostream>
#include <fstream>

#include "util/alignedmemory.h"
#include "util/timer.h"
#include "mve/image.h"
#include "mve/imagetools.h"
#include "mve/imagefile.h"

#include "surf.h"
#include "sift.h"
#include "matching.h"
#include "visualizer.h"

uint8_t const color_black[] = { 0, 0, 0 };

template <typename DESCR, int DIM>
sfm::MatchingResult
twoview_matching (std::vector<DESCR> const& descr1,
    std::vector<DESCR> const& descr2)
{
#if 1
    /* Convert the descriptors to aligned float arrays. */
    util::AlignedMemory<float, 16> matchset_1(DIM * descr1.size());
    float* out_ptr1 = matchset_1.begin();
    for (std::size_t i = 0; i < descr1.size(); ++i, out_ptr1 += DIM)
        std::copy(descr1[i].data.begin(), descr1[i].data.end(), out_ptr1);

    util::AlignedMemory<float, 16> matchset_2(DIM * descr2.size());
    float* out_ptr2 = matchset_2.begin();
    for (std::size_t i = 0; i < descr2.size(); ++i, out_ptr2 += DIM)
        std::copy(descr2[i].data.begin(), descr2[i].data.end(), out_ptr2);
#else
    /* Convert the descriptors to aligned short arrays. */
    util::AlignedMemory<short, 16> matchset_1(64 * descr1->size());
    short* mem_ptr = matchset_1.begin();
    for (std::size_t i = 0; i < descr1->size(); ++i)
    {
        std::vector<float> const& d = descr1->at(i).data;
        for (int j = 0; j < 64; ++j)
            *(mem_ptr++) = static_cast<short>(d[j] * 127.0f);
    }

    util::AlignedMemory<short, 16> matchset_2(64 * descr2->size());
    mem_ptr = matchset_2.begin();
    for (std::size_t i = 0; i < descr2->size(); ++i)
    {
        std::vector<float> const& d = descr2->at(i).data;
        for (int j = 0; j < 64; ++j)
            *(mem_ptr++) = static_cast<short>(d[j] * 127.0f);
    }
#endif

    /* Perform matching. */
    sfm::MatchingOptions matchopts;
    matchopts.descriptor_length = DIM;
    matchopts.lowe_ratio_threshold = 0.9f;
    matchopts.distance_threshold = std::numeric_limits<float>::max();

    util::WallTimer timer;
    sfm::MatchingResult matching;
    sfm::match_features(matchopts, matchset_1.begin(), descr1.size(),
        matchset_2.begin(), descr2.size(), &matching);
    sfm::remove_inconsistent_matches(&matching);
    std::cout << "Two-view matching took " << timer.get_elapsed() << "ms." << std::endl;

    int num_matches = 0;
    for (std::size_t i = 0; i < matching.matches_1_2.size(); ++i)
        num_matches += matching.matches_1_2[i] < 0 ? 0 : 1;
    std::cout << "Kept " << num_matches << " good matches." << std::endl;
    return matching;
}

template <typename DESCR>
mve::ByteImage::Ptr
visualize_matching (sfm::MatchingResult const& matching,
    mve::ByteImage::Ptr image1, mve::ByteImage::Ptr image2,
    std::vector<DESCR> const& descr1, std::vector<DESCR> const& descr2)
{
    /* Visualize keypoints. */
    std::vector<sfm::Visualizer::Match> vis_matches;
    for (std::size_t i = 0; i < matching.matches_1_2.size(); ++i)
    {
        if (matching.matches_1_2[i] < 0)
            continue;
        int j = matching.matches_1_2[i];

        sfm::Visualizer::Match match;
        match.p1[0] = descr1[i].x;
        match.p1[1] = descr1[i].y;
        match.p2[0] = descr2[j].x;
        match.p2[1] = descr2[j].y;
        vis_matches.push_back(match);
    }

    std::cout << "Drawing matches..." << std::endl;
    mve::ByteImage::Ptr match_image = sfm::Visualizer::draw_matches
        (mve::image::expand_grayscale<uint8_t>(image1),
        mve::image::expand_grayscale<uint8_t>(image2), vis_matches);
    return match_image;
}

void
sift_matching (mve::ByteImage::Ptr image1, mve::ByteImage::Ptr image2)
{
    sfm::Sift::Descriptors descr1;
    {
        sfm::Sift sift;
        sift.set_image(image1);
        sift.process();
        descr1 = sift.get_descriptors();
    }

    sfm::Sift::Descriptors descr2;
    {
        sfm::Sift sift;
        sift.set_image(image2);
        sift.process();
        descr2 = sift.get_descriptors();
    }

    sfm::MatchingResult matching
        = twoview_matching<sfm::Sift::Descriptor, 128>(descr1, descr2);
    mve::ByteImage::Ptr match_image
        = visualize_matching(matching, image1, image2, descr1, descr2);
    mve::image::save_file(match_image, "/tmp/matching_sift.png");
}

void
surf_matching (mve::ByteImage::Ptr image1, mve::ByteImage::Ptr image2)
{
    sfm::Surf::Descriptors descr1;
    {
        sfm::Surf surf;
        surf.set_contrast_threshold(500.0f);
        surf.set_image(image1);
        surf.process();
        descr1 = surf.get_descriptors();
    }

    sfm::Surf::Descriptors descr2;
    {
        sfm::Surf surf;
        surf.set_contrast_threshold(500.0f);
        surf.set_image(image2);
        surf.process();
        descr2 = surf.get_descriptors();
    }

    sfm::MatchingResult matching
        = twoview_matching<sfm::Surf::Descriptor, 64>(descr1, descr2);
    mve::ByteImage::Ptr match_image
        = visualize_matching(matching, image1, image2, descr1, descr2);
    mve::image::save_file(match_image, "/tmp/matching_surf.png");
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

    //sift_matching(image1, image2);
    //surf_matching(image1, image2);

#if 1
    image1 = mve::image::crop<uint8_t>(image1, image1->width() + 200, image1->height() + 200, -100, -100, color_black);
    image2 = mve::image::crop<uint8_t>(image2, image2->width() + 200, image2->height() + 200, -100, -100, color_black);
    mve::image::save_file(image1, "/tmp/testcropped.png");
    std::ofstream gnuplot_data("/tmp/gnuplot-data.txt");
    for (int i = 0; i < 360; i += 5)
    {
        mve::ByteImage::Ptr image2b = mve::image::rotate<uint8_t>(image2, i * MATH_PI / 180.0, color_black);

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
        std::string filename = "/tmp/featurematching_" + util::string::get_filled(i, 3, '0') + ".png";
        mve::image::save_file(visualize_matching(matching, image1, image2b, descr1, descr2), filename);

        int num_matches = 0;
        for (std::size_t i = 0; i < matching.matches_1_2.size(); ++i)
            num_matches += matching.matches_1_2[i] < 0 ? 0 : 1;
        gnuplot_data << i << " " << num_matches << std::endl;
    }
    gnuplot_data.close();

#endif

    return 0;
}
