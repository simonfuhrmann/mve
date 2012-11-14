#include <iostream>

#include "util/string.h"
#include "util/timer.h"

#include "image.h"
#include "imagefile.h"
#include "imagetools.h"
#include "bilateral.h"
#include "depthmap.h"

int
main (void)
{

#if 0

    /* Test Bilateral filtering on double images, comparison with float. */
    mve::ByteImage::Ptr orig = mve::image::load_file
        ("../../data/testimages/lenna_color_small.png");

    mve::DoubleImage::Ptr dimg(mve::image::byte_to_double_image(orig));
    mve::DoubleImage::Ptr dret = mve::image::bilateral_filter<double,3>
        (dimg, 4.0f, 0.1f);

    mve::FloatImage::Ptr fimg(mve::image::byte_to_float_image(orig));
    mve::FloatImage::Ptr fret = mve::image::bilateral_filter<float,3>
        (fimg, 4.0f, 0.1f);

    mve::ByteImage::Ptr dtb(mve::image::double_to_byte_image(dret));
    mve::ByteImage::Ptr ftb(mve::image::float_to_byte_image(fret));

    mve::image::save_file(dtb, "/tmp/bilateral_dret.png");
    mve::image::save_file(ftb, "/tmp/bilateral_fret.png");

    std::size_t diff_cnt = 0;
    for (std::size_t i = 0; i < orig->get_value_amount(); ++i)
        if (dtb->at(i) != ftb->at(i))
            diff_cnt += 1;
    std::cout << diff_cnt << " values differ!" << std::endl;

#endif

#if 0

    /* Test Bilateral filter on byte images. (FAILED!) */
    mve::ByteImage::Ptr orig = mve::image::load_file
        ("../../data/testimages/lenna_color_small.png");
    mve::ByteImage::Ptr ret = mve::image::bilateral_filter<unsigned char,3>
        (orig, 2.0f, 30.0f);
    mve::image::save_file(ret, "/tmp/bilateral_fail.png");

#endif

#if 1

    /* Bilateral filtering of color images for tiling */

    util::ClockTimer timer;

    mve::ByteImage::Ptr orig = mve::image::load_file
        ("../../data/testimages/lenna_color_small.png");

    for (float gc_sigma = 1.0f; gc_sigma < 4.1f; gc_sigma *= 2.0f) // 3 times
        for (float pc_sigma = 5.0f; pc_sigma < 81.0f; pc_sigma *= 2.0f) // 5 times
        {
            mve::ByteImage::Ptr ret = mve::image::bilateral_filter<uint8_t,3>
                (orig, gc_sigma, pc_sigma);

            mve::image::save_file(ret,
                "/tmp/test_out_" + util::string::get(gc_sigma) + "_"
                + util::string::get(pc_sigma) + ".png");
        }

    std::cout << "Took " << timer.get_elapsed() << " ms." << std::endl;

#endif



#if 0

    /* Bilateral filtering of color image test */

    mve::ByteImage::Ptr bi(mve::image::load_file("../../data/testimages/lenna_color.png"));
    mve::FloatImage::Ptr fi(mve::image::byte_to_float_image(bi));


    util::ClockTimer timer;
    mve::FloatImage::Ptr ret = mve::image::bilateral_filter<float,3>(fi, 2.0f, 0.2f);
    std::cout << "Took " << timer.get_elapsed() << " ms." << std::endl;

    bi = mve::image::float_to_byte_image(ret);
    mve::image::save_file(bi, "/tmp/bilateral_test12.png");

#endif


#if 0
    /* Bilateral filtering of gray byte images. */
    mve::ByteImage::Ptr bi(mve::image::load_file("../../data/testimages/lenna_gray.png"));
    mve::ByteImage::Ptr ret = mve::image::bilateral_filter<uint8_t,1>(bi, 2.0f, 20.0f);
    mve::image::save_file(ret, "/tmp/lenna_gray_ret.png");
#endif


#if 0

    /* Bilateral filtering of gray image test */

    mve::ByteImage::Ptr bi(mve::image::load_file("../../data/testimages/lenna_gray.png"));
    mve::FloatImage::Ptr fi(mve::image::byte_to_float_image(bi));

    util::ClockTimer timer;
    mve::FloatImage::Ptr ret = mve::image::bilateral_filter<float,1>(fi, 2.0f, 0.2f);
    std::cout << "Took " << timer.get_elapsed() << " ms." << std::endl;

    bi = mve::image::float_to_byte_image(ret);
    mve::image::save_file(bi, "/tmp/bilateral_newimpl.png");

#endif


#if 0
    /* Old filtering (gray image), for timing comparison. */
    /* New one wins (slightly) :) */
    mve::ByteImage::Ptr bi(mve::image::load_file("../../data/testimages/lenna_gray.png"));
    mve::FloatImage::Ptr fi(mve::image::byte_to_float_image(bi));


    util::ClockTimer timer;
    mve::FloatImage::Ptr ret = mve::image::bilateral_gray_image(fi, 2.0f, 0.2f);
    std::cout << "Took " << timer.get_elapsed() << " ms." << std::endl;

    bi = mve::image::float_to_byte_image(ret);
    mve::image::save_file(bi, "/tmp/bilateral_oldimpl.png");


#endif


    return 0;
}
