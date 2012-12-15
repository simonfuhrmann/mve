#include <iostream>

#include "util/timer.h"
#include "mve/image.h"
#include "mve/imagefile.h"

#include "surf.h"

#include "matching.h"

// TODO: Move to its own file in util.
template <typename T, uintptr_t MODULO = 16>
struct AlignedArray
{
    AlignedArray (void) : raw(0), aligned(0) {}
    AlignedArray (std::size_t size) { this->allocate(size); }
    ~AlignedArray (void) { this->deallocate(); }
    void allocate (std::size_t size)
    {
        this->raw = new unsigned char[(size + MODULO - 1) * sizeof(T)];
        uintptr_t tmp = reinterpret_cast<uintptr_t>(this->raw);
        this->aligned = (tmp & ~(MODULO - 1)) == tmp
            ? reinterpret_cast<T*>(tmp)
            : reinterpret_cast<T*>((tmp + MODULO) & ~(MODULO - 1));
    }
    void deallocate (void)
    {
        delete this->raw;
        this->raw = 0;
        this->aligned = 0;
    }

    unsigned char* raw;
    T* aligned;
};


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

    sfm::Surf surf;
    surf.set_image(image1);
    surf.process();
    sfm::Surf::SurfDescriptors const& descr1 = surf.get_descriptors();

    surf.set_image(image2);
    surf.process();
    sfm::Surf::SurfDescriptors const& descr2 = surf.get_descriptors();


    /*
     * Convert the descriptors to aligned short arrays.
     */
    AlignedArray<short> match_descr_1(64 * descr1.size());
    AlignedArray<short> match_descr_2(64 * descr2.size());

    short* mem_ptr = match_descr_1.aligned;
    for (std::size_t i = 0; i < descr1.size(); ++i)
    {
        std::vector<float> const& d = descr1[i].data;
        for (int j = 0; j < 64; ++j)
            *(mem_ptr++) = static_cast<short>(d[j] * 127.0f);
    }
    mem_ptr = match_descr_2.aligned;
    for (std::size_t i = 0; i < descr2.size(); ++i)
    {
        std::vector<float> const& d = descr2[i].data;
        for (int j = 0; j < 64; ++j)
            *(mem_ptr++) = static_cast<short>(d[j] * 127.0f);
    }

    /* Perform matching. */
    sfm::MatchingOptions matchopts;
    matchopts.descriptor_length = 64;
    matchopts.lowe_ratio_threshold = 0.8f;
    sfm::MatchingResult matchresult;

    util::WallTimer timer;
    sfm::match_features(matchopts, match_descr_1.aligned, descr1.size(),
        match_descr_2.aligned, descr2.size(), &matchresult);
    sfm::remove_inconsistent_matches(&matchresult);
    std::cout << "Two-view matching took " << timer.get_elapsed() << "ms." << std::endl;

    /* Visualize matches. */


    return 0;
}
