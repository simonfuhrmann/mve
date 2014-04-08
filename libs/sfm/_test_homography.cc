#include <iostream>

#include "util/aligned_memory.h"
#include "mve/image.h"
#include "mve/image_io.h"
#include "sfm/sift.h"
#include "sfm/matching.h"
#include "sfm/correspondence.h"
#include "sfm/ransac_fundamental.h"
#include "sfm/ransac_homography.h"

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
    mve::ByteImage::Ptr img1 = mve::image::load_file(fname1);
    mve::ByteImage::Ptr img2 = mve::image::load_file(fname2);

    /* Compute SIFT descriptors. */
    sfm::Sift::Descriptors img1_desc, img2_desc;
    {
        sfm::Sift::Options sift_opts;
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
    float data_ptr = descr1.begin();
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
    sfm::Correspondences corr;
    for (std::size_t i = 0; i < matching_result.matches_1_2.size(); ++i)
    {
        int const id1 = i;
        int const id2 = matching_result.matches_1_2[i];

        sfm::Sift::Descriptor const& d1 = img1_descr[id1];
        sfm::Sift::Descriptor const& d2 = img1_descr[id2];
        sfm::Correspondence c2d2d;
        c2d2d.p1[0] = d1.x; c2d2d.p1[1] = d1.y;
        c2d2d.p2[0] = d2.x; c2d2d.p2[1] = d2.y;
        corr.push_back(c2d2d);
    }

    /* Fundamental RANSAC. */
    sfm::RansacFundamental

    /* Homography RANSAC. */

}
