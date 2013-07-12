#include <csignal>
#include <iostream>
#include <fstream>

#include "util/timer.h"
#include "util/system.h"
#include "util/filesystem.h"
#include "math/vector.h"
#include "mve/image.h"
#include "mve/imagetools.h"
#include "mve/imagefile.h"
#include "mve/imageexif.h"
#include "mve/bilateral.h"

int
main (int argc, char** argv)
{
    std::signal(SIGSEGV, util::system::signal_segfault_handler);

#if 1
    /* Timing test for bilateral filter. */
    mve::ByteImage::Ptr img = mve::image::load_file("/tmp/mouse.jpg");
    util::WallTimer timer;
    img = mve::image::bilateral_filter<uint8_t, 3>(img, 20.0f, 50.0f);
    std::cout << "Took " << timer.get_elapsed() << "ms." << std::endl;
    mve::image::save_file(img, "/tmp/bilateral.png");
#endif

#if 0
    // Test image undistortion
    mve::ByteImage::Ptr img = mve::image::load_file("/tmp/mouse.jpg");
    mve::ByteImage::Ptr out = mve::image::image_undistort_msps<uint8_t>(img, 2.0f, 0.0f);
    out = mve::image::rescale_half_size<uint8_t>(out);
    mve::image::save_file(out, "/tmp/undist_msps_test-1.png");

    out = mve::image::rescale_half_size<uint8_t>(img);
    out = mve::image::image_undistort_msps<uint8_t>(out, 2.0f, 0.0f);
    mve::image::save_file(out, "/tmp/undist_msps_test-2.png");

#endif

#if 0
    /* Test new linear interpolation. */
    mve::ByteImage::Ptr img = mve::image::load_file("/tmp/warped-mve.png");
    float w = (float)img->width();
    float h = (float)img->height();
    int num = 0;
    util::ClockTimer timer;

    float x = 0.0f;
    float y = 0.0f;
    {
        uint8_t px[3];
        for (int i = 0; i < 10000000; ++i, ++num)
        {
            x += 0.00002f;
            y += 0.00002f;
            img->linear_at(x, y, px);
        }
    }
    std::cout << "Performed uint8 " << num << " lookups in "
        << timer.get_elapsed() << "ms." << std::endl;

    mve::FloatImage::Ptr fimg = mve::image::byte_to_float_image(img);
    timer.reset();
    num = 0;

    x = 0.0f;
    y = 0.0f;
    {
        float px[3];
        for (int i = 0; i < 10000000; ++i, ++num)
        {
            x += 0.00002f;
            y += 0.00002f;
            fimg->linear_at(x, y, px);
        }
    }
    std::cout << "Performed float " << num << " lookups in "
        << timer.get_elapsed() << "ms." << std::endl;
#endif

#if 0
    /* Test blur_boxfilter function. */
    std::cout << "Loading image..." << std::endl;
    mve::ByteImage::Ptr img = mve::image::load_file
        ("/tmp/boxfilter-ind.png");
        //("../../data/testimages/diaz_color.png");
        //("/data/pics/photography/20120107-focusstacking/P1040362.JPG");

    std::cout << "Blurring image..." << std::flush;
    util::ClockTimer timer;
    img = mve::image::blur_boxfilter<uint8_t>(img, 40);
    //img = mve::image::blur_gaussian<uint8_t>(img, 10);
    std::cout << " took " << timer.get_elapsed() << "ms." << std::endl;

    std::cout << "Saving image..." << std::endl;
    mve::image::save_file(img, "/tmp/out-box2.png");

#endif

#if 0

    util::fs::Directory dir("/gris/scratch2/bundlertest/citywall/images/");
    for (std::size_t i = 0; i < dir.size(); ++i)
    {
        std::cout << "Loading image " << dir[i].name << "..." << std::endl;
        std::string exif;
        mve::image::load_jpg_file(dir[i].get_absolute_name(), &exif);

        if (exif.empty())
        {
            std::cout << "    No EXIF, skipping..." << std::endl;
            continue;
        }

        std::cout << "    EXIF data has " << exif.size() << " bytes." << std::endl;

        try
        {
            mve::image::ExifInfo info = mve::image::exif_extract(exif.c_str(), exif.size(), false);
            mve::image::exif_debug_print(std::cout, info);
        }
        catch (std::exception& e)
        {
            std::cout << "    Corrupt EXIF: " << e.what() << std::endl;
        }
    }
#endif

#if 0
    /* Test swapping of images. */
    mve::ByteImage::Ptr img(mve::image::load_file
        ("../../data/testimages/diaz_color.png"));
    mve::ByteImage::Ptr other(mve::ByteImage::create(1000, 2000, 4));

    util::ClockTimer timer;
    for (int i = 0; i < 1001; ++i)
        std::swap(*img, *other);

    std::cout << "Img: " << img->width() << "x" << img->height() << std::endl;
    std::cout << "took " << timer.get_elapsed() << "ms" << std::endl;

#endif

#if 0
    /* Test integral images. */
    mve::ByteImage::Ptr img(mve::image::load_file("/tmp/orig.png"));
    mve::FloatImage::Ptr fimg = mve::image::integral_image<uint8_t,float>(img);
    //img = mve::image::integral_image<uint8_t,uint8_t>(img);
    img = mve::image::float_to_byte_image(fimg, 0, 255);
    mve::image::save_file(img, "/tmp/integral.png");
#endif

#if 0
    /* Test image difference / subtract. */
    mve::ByteImage::Ptr i1(mve::image::load_file("/tmp/grad1.png"));
    mve::ByteImage::Ptr i2(mve::image::load_file("/tmp/grad2.png"));

    mve::ByteImage::Ptr out(mve::image::subtract<uint8_t>(i2, i1));
    mve::image::save_file(out, "/tmp/grad-out.png");

#endif

#if 0
    /* Test image half size subsampling. */
    mve::ByteImage::Ptr img(mve::image::load_file
        //("../../data/testimages/test_scaling_rings.png"));
        //("../../data/testimages/lenna_gray.png"));
        ("../../data/testimages/lenna_color.png"));
        //("../../data/testimages/test_scaling_5x5.png"));

    util::ClockTimer t;
    mve::ByteImage::Ptr out(mve::image::rescale_half_size_subsample<uint8_t>(img));
    std::cout << "took " << t.get_elapsed() << "ms." << std::endl;
    mve::image::save_file(out, "/tmp/halfsize.png");
#endif

#if 0
    /* Test double size of image. */
    mve::ByteImage::Ptr img(mve::image::load_file
        //("../../data/testimages/test_scaling_rings.png"));
        //("../../data/testimages/lenna_color.png"));
        //("../../data/testimages/color_pattern_5x5.png"));
        ("/tmp/img.png"));
    util::ClockTimer t;
    //mve::ByteImage::Ptr out(mve::image::rescale_double_size_supersample<uint8_t>(img));
    //std::cout << "took " << t.get_elapsed() << "ms." << std::endl;
    //mve::image::save_file(out, "/tmp/doubled.png");

    //t.reset();
    mve::ByteImage::Ptr out = mve::ByteImage::create(img->width() * 2, img->height() * 2, img->channels());
    mve::image::rescale_linear<uint8_t>(img, out);
    std::cout << "took " << t.get_elapsed() << "ms." << std::endl;
    mve::image::save_file(out, "/tmp/doubled2.png");

#endif

#if 0
    /* Test blurring of images. */
    mve::ByteImage::Ptr img(mve::image::load_file
        //("../../data/testimages/test_scaling_rings.png"));
        ("../../data/testimages/lenna_color.png"));
        //("/tmp/dot.png"));

    std::size_t w(img->width());
    std::size_t h(img->height());
    util::ClockTimer timer;


    for (int i = 0; i < 50; ++i)
    {
        std::cout << i << " Blurring..." << std::flush;
        timer.reset();
        mve::ByteImage::Ptr out(mve::image::blur_gaussian<uint8_t>(img, (float)i * 0.1f));
        std::cout << " took " << timer.get_elapsed() << "ms" << std::endl;
        mve::image::save_file(out, "/tmp/blur-" + util::string::get_filled(i, 2, '0') + ".png");
    }


    std::cout << "Resizing..." << std::flush;
    mve::ByteImage::Ptr out = mve::ByteImage::create(w, h, img->channels());
    timer.reset();
    mve::image::rescale_gaussian<uint8_t>(img, out, 1.0f);
    std::cout << " took " << timer.get_elapsed() << "ms" << std::endl;
    mve::image::save_file(out, "/tmp/scaling_blur.png");

    return 0;

#endif

#if 0
    /* Test rescaling to half size and compare. */
    mve::ByteImage::Ptr img(mve::image::load_file
        ("../../data/testimages/diaz_color.png"));
        //("/tmp/test1px.png"));

    std::size_t iw = img->width();
    std::size_t ih = img->height();
    std::size_t ow = (iw + 1) >> 1;
    std::size_t oh = (ih + 1) >> 1;

    mve::ByteImage::Ptr out;

    util::ClockTimer timer;
    //out = mve::image::rescale_half_size<uint8_t>(img);
    //out = mve::image::rescale_half_size<uint8_t>(out);
    //mve::image::save_file(out, "/tmp/halfsize-regular.png");
    //std::cout << "Regular took " << timer.get_elapsed() << "ms" << std::endl;

    timer.reset();
    out = mve::image::rescale_half_size_gaussian<uint8_t>(img, 1.0f);
    mve::image::save_file(out, "/tmp/halfsize-gaussian-new1.png");
    out = mve::image::rescale_half_size_gaussian<uint8_t>(out, 1.0f);
    mve::image::save_file(out, "/tmp/halfsize-gaussian-new2.png");
    std::cout << "Gaussian took " << timer.get_elapsed() << "ms" << std::endl;

    //timer.reset();
    //out = mve::image::rescale<uint8_t>(img, mve::image::RESCALE_GAUSSIAN, ow, oh);
    //out = mve::image::rescale<uint8_t>(img, mve::image::RESCALE_GAUSSIAN, (ow + 1) >> 1, (oh + 1) >> 1);
    //mve::image::save_file(out, "/tmp/halfsize-perfect.png");
    //std::cout << "Perfect took " << timer.get_elapsed() << "ms" << std::endl;

    return 0;

#endif

#if 0
    /* Test broken image loading. */
    if (argc < 2)
    {
        std::cout << "Pass image as parameter" << std::endl;
        return 1;
    }
    mve::ByteImage::Ptr img = mve::image::load_jpg_file(argv[1]);
    mve::image::save_file(img, "/tmp/test-out.png");

#endif

#if 0
    /* Test image gamma correction. */

    if (argc < 2)
    {
        std::cout << "Pass image as parameter" << std::endl;
        return 1;
    }

    mve::ByteImage::Ptr image(mve::image::load_file(argv[1]));
    mve::image::gamma_correct(image, 1.0f/2.2f);
    mve::image::save_file(image, "/tmp/out-gamma.png");

#endif

#if 0
    /* Test image flipping. */

    if (argc < 2)
    {
        std::cout << "Pass image as parameter" << std::endl;
        return 1;
    }

    mve::ByteImage::Ptr image(mve::image::load_file(argv[1]));
    mve::ByteImage::Ptr out;

    out = mve::ByteImage::create(*image);
    mve::image::flip<uint8_t>(out, mve::image::FLIP_HORIZONTAL);
    mve::image::save_file(out, "/tmp/out-hori.png");

    out = mve::ByteImage::create(*image);
    mve::image::flip<uint8_t>(out, mve::image::FLIP_VERTICAL);
    mve::image::save_file(out, "/tmp/out-verti.png");

    out = mve::ByteImage::create(*image);
    mve::image::flip<uint8_t>(out, mve::image::FLIP_BOTH);
    mve::image::save_file(out, "/tmp/out-both.png");

    return 0;
#endif


#if 0
    /* Test image rotation. */

    if (argc < 2)
    {
        std::cout << "Pass image as parameter" << std::endl;
        return 1;
    }

    mve::ByteImage::Ptr image(mve::image::load_file(argv[1]));
    mve::ByteImage::Ptr out;

    out = mve::image::rotate<uint8_t>(image, mve::image::ROTATE_180);
    mve::image::save_file(out, "/tmp/out-180.png");

    out = mve::image::rotate<uint8_t>(image, mve::image::ROTATE_CW);
    mve::image::save_file(out, "/tmp/out-cw.png");

    out = mve::image::rotate<uint8_t>(image, mve::image::ROTATE_CCW);
    mve::image::save_file(out, "/tmp/out-ccw.png");

    out = mve::image::rotate<uint8_t>(image, mve::image::ROTATE_SWAP);
    mve::image::save_file(out, "/tmp/out-swap.png");

    return 0;
#endif

#if 0
    /* Test image type conversion. */

    mve::ByteImage::Ptr img(mve::image::load_file
        ("../../data/testimages/lenna_color.png"));

    mve::FloatImage::Ptr fi(mve::image::byte_to_float_image(img));
    mve::ByteImage::Ptr bi(mve::image::float_to_byte_image(fi, 0.0f, 0.5f));

    mve::image::save_file(bi, "/tmp/testimg.png");

    //for (std::size_t i = 0; i < bi->get_value_amount(); ++i)
    //    if (bi->at(i) != img->at(i))
    //        std::cout << "Different value at " << i << std::endl;

#endif

#if 0
    /* Test gaussian rescaling. */
    std::cout << "Loading..." << std::endl;
    mve::ByteImage::Ptr img(mve::image::load_file
        ("../../data/testimages/lenna_color_small.png"));
        //("/tmp/color_pattern_5x5.png"));
        //("../../data/testimages/test_scaling_rings.png"));

    std::cout << "Resizing..." << std::endl;
    mve::ByteImage::Ptr out(mve::ByteImage::create(148, 148, img->channels()));

    mve::image::rescale_gaussian<uint8_t>(img, out, 0.25f);
    mve::image::save_file(out, "/tmp/scaling_025.png");

    mve::image::rescale_gaussian<uint8_t>(img, out, 0.5f);
    mve::image::save_file(out, "/tmp/scaling_050.png");

    mve::image::rescale_gaussian<uint8_t>(img, out, 1.00f);
    mve::image::save_file(out, "/tmp/scaling_100.png");

    mve::image::rescale_gaussian<uint8_t>(img, out, 2.00f);
    mve::image::save_file(out, "/tmp/scaling_200.png");
#endif

#if 0
    /* Test gaussian rescaling. */
    std::cout << "Loading..." << std::endl;
    mve::ByteImage::Ptr in(mve::image::load_file
        ("/gris/scratch/mvs_datasets/playhouse-091218/undistorted/undistorted_0000_0004.jpg"));
        //("../../data/testimages/test_scaling_rings.png"));
    mve::FloatImage::Ptr fin(mve::image::byte_to_float_image(in));

    std::cout << "Resizing..." << std::endl;
    mve::FloatImage::Ptr out1(mve::FloatImage::create(fin->width()/5, fin->height()/5, fin->channels()));
    mve::FloatImage::Ptr out2(mve::FloatImage::create(fin->width()/5, fin->height()/5, fin->channels()));

    mve::image::rescale_gaussian<float>(fin, out1);
    mve::image::rescale_linear<float>(fin, out2);

    std::cout << "Writing..." << std::endl;
    mve::ByteImage::Ptr out(mve::image::float_to_byte_image(out1));
    mve::image::save_file(out, "/tmp/test_rings_gaussian.png");
    out = mve::image::float_to_byte_image(out2);
    mve::image::save_file(out, "/tmp/test_rings_linear.png");
#endif

#if 0
    /* Test rescaling of small images. */
    mve::ByteImage::Ptr img(mve::ByteImage::create());
    img->allocate(2, 1, 3);
    img->fill(0);
    img->at(0,0,0) = 255;
    img->at(1,0,1) = 255;

    img = mve::image::rescale(img, mve::image::RESCALE_LINEAR, 10, 1);
    mve::image::save_file(img, "/tmp/small_scaling.png");

#endif

#if 0
    /* Crop, desaturate and rescale tests. */
    mve::ByteImage::Ptr img(mve::image::load_file
        //("../../data/testimages/test_scaling_10x10.png"));
        //("/tmp/Rings1.png"));
        ("/tmp/in.ppm"));
    mve::ByteImage::Ptr out;
#endif

#if 0
    /* Test image desaturate. */
    out = mve::image::desaturate<unsigned char>(img, mve::image::DESATURATE_LIGHTNESS);
    mve::image::save_file(out, "/tmp/desat_lightness.png");
    out = mve::image::desaturate<unsigned char>(img, mve::image::DESATURATE_LUMINOSITY);
    mve::image::save_file(out, "/tmp/desat_luminosity.png");
    out = mve::image::desaturate<unsigned char>(img, mve::image::DESATURATE_AVERAGE);
    mve::image::save_file(out, "/tmp/desat_average.png");
#endif

#if 0
    /* Test image scaling (nearest neighbor). */
    std::cout << "Rescaling NN..." << std::endl;
    out = mve::image::rescale<unsigned char>(img,
        mve::image::RESCALE_NEAREST, 200, 0);
    mve::image::save_file(out, "/tmp/rings_nearest.png");

    std::cout << "Rescaling linear..." << std::endl;
    out = mve::image::rescale<unsigned char>(img,
        mve::image::RESCALE_LINEAR, 200, 0);
    mve::image::save_file(out, "/tmp/rings_linear.png");

    std::cout << "Rescaling gaussian..." << std::endl;
    out = mve::image::rescale<unsigned char>(img,
        mve::image::RESCALE_GAUSSIAN, 200, 0);
    mve::image::save_file(out, "/tmp/rings_gaussian.png");

    /* Test image cropping. */
    out = mve::image::crop<unsigned char>(img, 100, 100, 400, 300);
#endif

#if 0
    /* Test image mipmapping - rescale image to half size. */
    std::cout << "image " << img->width() << "x" << img->height() << std::endl;
    out = mve::image::rescale_half_size<unsigned char>(img);
    mve::image::save_file(out, "/tmp/lenna_by02.png");

    std::cout << "image " << out->width() << "x" << out->height() << std::endl;
    out = mve::image::rescale_half_size<unsigned char>(out);
    mve::image::save_file(out, "/tmp/lenna_by04.png");

    std::cout << "image " << out->width() << "x" << out->height() << std::endl;
    out = mve::image::rescale_half_size<unsigned char>(out);
    mve::image::save_file(out, "/tmp/lenna_by08.png");

    std::cout << "image " << out->width() << "x" << out->height() << std::endl;
    out = mve::image::rescale_half_size<unsigned char>(out);
    mve::image::save_file(out, "/tmp/lenna_by16.png");
#endif

    return 0;
}

