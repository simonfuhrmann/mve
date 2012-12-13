#include <iostream>

#include "util/timer.h"
#include "mve/image.h"
#include "mve/imagefile.h"

#include "surf.h"

#include <emmintrin.h>

int main (void)
{
#ifdef __SSE2__
    std::cout << "SSE2 is enabled!" << std::endl;
#endif

    int const num_descr = 1024000;
    AlignedMemory<short> mem(64 * num_descr);
    AlignedMemory<short> query(64);

    for (int i = 0; i < 11; ++i)
    {
        mem.aligned[i] = i;
        query.aligned[i] = i;
    }

    util::WallTimer timer;
    sse2_matching(query.aligned, mem.aligned, num_descr);
    std::cout << "took " << timer.get_elapsed() << "ms." << std::endl;

    timer.reset();
    dumb_matching(query.aligned, mem.aligned, num_descr);
    std::cout << "took " << timer.get_elapsed() << "ms." << std::endl;

#if 0
    std::vector<short> vec1(64);
    std::vector<short> vec2(64);
    for (std::size_t i = 0; i < vec1.size(); ++i)
    {
        vec1[i] = (i * 5) % 255 - 128;
        vec2[i] = (i * 3) % 255 - 128;
    }

    util::WallTimer timer;
    int result = 0;
    for (int i = 0; i < 1000000; ++i)
        result = inner_product(vec1, vec2);
    std::cout << "result is " << result << std::endl;
    std::cout << "took " << timer.get_elapsed() << "ms." << std::endl;
#endif

#if 0
    mve::ByteImage::Ptr image;
    try
    {
        image = mve::image::load_file("/tmp/image.jpg");
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    sfm::Surf surf;
    surf.set_image(image);
    surf.set_contrast_threshold(1000.0f);
    surf.process();

    surf.draw_features(image);
    mve::image::save_file(image, "/tmp/out.png");
#endif
    return 0;
}
