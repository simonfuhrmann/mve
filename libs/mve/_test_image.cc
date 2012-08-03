#include <csignal>
#include <iostream>
#include <fstream>

#include "image.h"
#include "imagetools.h"
#include "imagefile.h"
#include "imageexif.h"
#include "surf.h"
#include "util/hrtimer.h"
#include "util/clocktimer.h"
#include "util/system.h"
#include "util/fs.h"
#include "math/vector.h"

int
main (int argc, char** argv)
{
    std::signal(SIGSEGV, util::system::signal_segfault_handler);

#if 1
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
    /* SURF test. */
    mve::ByteImage::Ptr img = mve::image::load_file("/tmp/testimage.png");
    mve::Surf surf;
    surf.set_image(img);
    surf.process();
#endif

#if 0
    /* Test load 16 bit PNG. */
    mve::ByteImage::Ptr img;
    try
    {
        img = mve::image::load_png_file("/gris/scratch/home/jackerma/tmp/both.png");
    }
    catch (std::exception& e)
    {
        std::cout << "Error loading: " << e.what() << std::endl;
    }

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
    // Reveal steganography image
    // download http://en.wikipedia.org/wiki/File:StenographyOriginal.png
    mve::ByteImage::Ptr img(mve::image::load_file("/tmp/StenographyOriginal.png"));
    for (std::size_t i = 0; i < img->get_value_amount(); ++i)
    {
        uint8_t& val = img->at(i);
        val = val << 6;
    }
    mve::image::save_file(img, "/tmp/out.png");
#endif


#if 0
    /* Test operation invariance against rotation. */
    mve::ByteImage::Ptr bimg(mve::image::load_file("/tmp/diaz.png"));
    mve::FloatImage::Ptr fimg(mve::image::byte_to_float_image(bimg));

    mve::FloatImage::Ptr orig(fimg->duplicate());
    mve::FloatImage::Ptr img(fimg->duplicate());
    img = mve::image::rotate<float>(img, mve::image::ROTATE_CCW);
    //img = mve::image::rescale_half_size<float>(img);
    //fimg = mve::image::rescale_half_size<float>(fimg);
    //img = mve::image::rescale_half_size_gaussian<float>(img);
    //fimg = mve::image::rescale_half_size_gaussian<float>(fimg);
    fimg = mve::image::blur_gaussian<float>(fimg, 50.0f);
    fimg = mve::image::subtract<float>(fimg, orig);
    orig = mve::image::rotate<float>(orig, mve::image::ROTATE_CCW);
    img = mve::image::blur_gaussian<float>(img, 50.0f);
    img = mve::image::subtract<float>(img, orig);
    //fimg = mve::image::rescale_double_size<float>(fimg);
    //img = mve::image::rescale_double_size<float>(img);
    img = mve::image::rotate<float>(img, mve::image::ROTATE_CW);

    /* Dump images. */
    mve::image::save_file(mve::image::float_to_byte_image(fimg,-0.1,0.1), "/tmp/invariance-original.png");
    mve::image::save_file(mve::image::float_to_byte_image(img,-0.1,0.1), "/tmp/invariance-rotated.png");

    // Difference
    std::size_t count = 0;
    float largest = 0.0f;
    float average = 0.0f;
    for (std::size_t i = 0; i < img->get_value_amount(); ++i)
    {
        float v1 = img->at(i);
        float v2 = fimg->at(i);
        if (v1 != v2)
        {
            float diff = (v1 - v2);
            if (std::abs(diff) > largest)
                largest = std::abs(diff);
            average += std::abs(diff);
            count += 1;
        }
    }

    std::cout << count << " of " << img->get_value_amount()
        <<  " values differ, max error: " << largest
        << ", average: " << (average / (float)count) << std::endl;
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
    util::ClockTimer timer;
    util::HRTimer hrtimer;
    mve::ByteImage::Ptr img(mve::image::load_file
        //("../../data/testimages/diaz_color.png"));
        //("/tmp/_testnoise.png"));
        (argv[1]));

    img = mve::image::nl_means_filter<uint8_t>(img, 5.0f);
    mve::image::save_file(img, "/tmp/nlmeans_out.png");
    std::cout << "took " << timer.get_elapsed() << "ms cpu time" << std::endl;
    std::cout << "took " << hrtimer.get_elapsed() << "ms real time" << std::endl;
#endif

#if 0
    mve::ByteImage::Ptr img(mve::image::load_file
        ("../../data/testimages/diaz_color.png"));
    mve::FloatImage::Ptr fimg = mve::image::byte_to_float_image(img);
    mve::image::save_file(fimg, "/tmp/test.pfm");
    mve::image::save_file(img, "/tmp/test.png");
    return 0;
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
    /* Test scale space axioms. */
    mve::ByteImage::Ptr img(mve::image::load_file
        ("../../data/testimages/diaz_color.png"));

    mve::ByteImage::Ptr i1 = mve::image::blur_gaussian<uint8_t>(img, 2.0f);
    mve::ByteImage::Ptr i2 = mve::image::blur_gaussian<uint8_t>(i1, 2.0f);

    mve::image::save_file(i2, "/tmp/_inc2.png");

    i1 = mve::image::blur_gaussian<uint8_t>(img, 2.82f);
    mve::image::save_file(i1, "/tmp/_full2.png");
    return 1;

    mve::ByteImage::Ptr out = img->duplicate();
    mve::image::rescale_gaussian<uint8_t>(img, out, 2.0f);
    mve::ByteImage::Ptr out2 = out->duplicate();
    mve::image::rescale_gaussian<uint8_t>(out, out2, 2.0f);
    mve::image::save_file(out2, "/tmp/_rescale_inc.png");

    mve::image::rescale_gaussian<uint8_t>(img, out, 4.0f);
    mve::image::save_file(out, "/tmp/_rescale_full.png");


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
    /* Hack to create gnuplot data file for scanline. */
    mve::ByteImage::Ptr img = mve::image::load_file(argv[1]);

    std::ofstream out("/tmp/scanline.gp");
    if (!out.good())
        return 1;
    for (std::size_t i = 0; i < img->get_pixel_amount(); ++i)
        out << i << " "
            << (int)img->at(i, 0) << " "
            << (int)img->at(i, 1) << " "
            << (int)img->at(i, 2) << std::endl;
    out.close();
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
    /* Test dark channel filter. */
    if (argc < 2)
    {
        std::cout << "Pass image as parameter" << std::endl;
        return 1;
    }

    mve::ByteImage::Ptr image(mve::image::load_file(argv[1]));
    mve::ByteImage::Ptr dark(mve::image::dark_channel<uint8_t>(image, 7));

    mve::image::save_file(dark, "/tmp/dc.png");

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


#if 0
    /* Test new image load interface. */
    std::cout << "Loading image..." << std::endl;
    //mve::ByteImage::Ptr image = mve::image::load_png_file("../../data/testimages/test_rgba_32x32.png");
    mve::ByteImage::Ptr image = mve::image::load_tiff_file("../../data/testimages/test_rgba_32x32.tiff");
    //mve::ByteImage::Ptr image = mve::image::load_tiff_file("/tmp/testtiff.tiff");

    std::cout << "Image loaded. "
        << image->width() << "x"
        << image->height() << "x"
        << image->channels() << std::endl;

    //mve::image::save_png_file(image, "/tmp/testpng.png");
    //mve::image::save_tiff_file(image, "/tmp/testtiff.tiff");

    std::size_t _w = image->width();
    std::size_t _h = image->height();
    std::size_t _c = image->channels();

    for (std::size_t i = 0; i < 20; ++i)
    {
        std::size_t x = i % _w;
        std::size_t y = i / _w;
        std::cout << "  Pixel " << i << ": ";
        /* Actual values. */
        std::cout << "P(";
        for (std::size_t c = 0; c < _c; ++c)
            std::cout << (int)image->at(x, y, c) << " ";
        std::cout << ")" << std::endl;
    }
    std::cout << std::endl;

#endif


#if 0

    /* Test PNG file reading. */
    mve::PNGFile png;
    std::cout << "Loading PNG..." << std::endl;
    try
    {
        png.load_png("../../data/testimages/test_rgba_32x32.png");
    }
    catch (util::Exception& e)
    {
        std::cout << "Error loading PNG: " << e << std::endl;
        return 1;
    }

    std::cout << "  Is valid: " << png.is_valid_png() << std::endl;
    std::cout << "  Size: " << png.width() << "x" << png.height() << std::endl;
    std::cout << "  Channels: " << png.channels() << std::endl;

    std::cout << "  First N pixels: " << std::endl;

    std::size_t _w = png.width();
    std::size_t _h = png.height();
    std::size_t _c = png.channels();

    for (std::size_t i = 0; i < 20; ++i)
    {
        std::size_t x = i % _w;
        std::size_t y = i / _w;
        std::cout << "  Pixel " << i << ": ";
        /* Actual values. */
        std::cout << "P(";
        for (std::size_t c = 0; c < _c; ++c)
            std::cout << (int)png.at(x, y, c) << " ";
        std::cout << "), RGB(";
        for (std::size_t c = 0; c < 3; ++c)
            std::cout << (int)png.at_rgb(x, y, c) << " ";
        std::cout << "), RGBA(";
        for (std::size_t c = 0; c < 4; ++c)
            std::cout << (int)png.at_rgba(x, y, c) << " ";
        std::cout << ")" << std::endl;
    }
    std::cout << std::endl;

    std::cout << "Saving PNG to file..." << std::endl;
    try
    {
        png.save_png("/tmp/output.png");
    }
    catch (util::Exception& e)
    {
        std::cout << "Error writing PNG: " << e.what() << std::endl;
    }

#endif

#if 0
    /* Test PFM file reading */
    mve::FloatImage::Ptr img = mve::image::load_pfm_file("../../data/testimages/uffizi_probe.pfm");

    std::cout << "Size: " << img->width() << ", " << img->height() << ", " << img->channels() << std::endl;

    /* Test PFM writing */
    mve::image::save_pfm_file(img, "/tmp/output.pfm");
#endif

#if 0

    /* Test JPG file reading. */
    mve::JPGFile jpg;
    std::cout << "Loading jpg..." << std::endl;
    try
    {
        //jpg.load_jpg("../../data/testimages/test_rgb_32x32.jpg");
        jpg.load_jpg("/home/simlan/temp/diaz16.jpg");
    }
    catch (util::Exception& e)
    {
        std::cout << "Error loading JPG: " << e << std::endl;
        return 1;
    }

    std::cout << "  Is valid: " << jpg.is_valid_jpg() << std::endl;
    std::cout << "  Size: " << jpg.width() << "x" << jpg.height() << std::endl;
    std::cout << "  Channels: " << jpg.channels() << std::endl;

    std::cout << "  First N pixels: " << std::endl;

    std::size_t _w = jpg.width();
    std::size_t _h = jpg.height();
    std::size_t _c = jpg.channels();

    for (std::size_t i = 0; i < 20; ++i)
    {
        std::size_t x = i % _w;
        std::size_t y = i / _w;
        std::cout << "  Pixel " << i << ": ";
        /* Actual values. */
        std::cout << "P(";
        for (std::size_t c = 0; c < _c; ++c)
            std::cout << (int)jpg.at(x, y, c) << " ";
        std::cout << "), RGB(";
        for (std::size_t c = 0; c < 3; ++c)
            std::cout << (int)jpg.at_rgb(x, y, c) << " ";
        std::cout << "), RGBA(";
        for (std::size_t c = 0; c < 4; ++c)
            std::cout << (int)jpg.at_rgba(x, y, c) << " ";
        std::cout << ")" << std::endl;
    }
    std::cout << std::endl;

    /* Test JPG file writing. */
    std::cout << "Saving JPG to file..." << std::endl;
    try
    {
        jpg.save_jpg("/tmp/output.jpg", 85);
    }
    catch (util::Exception& e)
    {
        std::cout << "Error writing JPG: " << e.what() << std::endl;
    }

#endif


#if 0
    /* Test PPM file reading */


    mve::FloatImage::Ptr img = mve::image::load_ppm_16_file("/tmp/in.ppm");
    /*
    mve::FloatImage::Ptr img = mve::FloatImage::create(100,100,3);
    float v = ((0x00000001) << 14) | ((0x00000001) << 2)  | ((0x00000001) << 5) ;

    std::cout << v << std::endl;
    img->fill(v);

    img->at(0) = 0.0f;
    img->at(1) = v+30;
    */
    std::cout << "Size: " << img->width() << ", " << img->height() << ", " << img->channels() << std::endl;

    /* Test PPM writing */
    mve::image::save_png_file(mve::image::float_to_byte_image(img), "/tmp/output.png");
    mve::image::save_ppm_16_file(img, "/tmp/output.ppm");

    mve::FloatImage::Ptr img2 = mve::image::load_ppm_16_file("/tmp/output.ppm");
    mve::image::save_ppm_16_file(img2, "/tmp/output2.ppm");
    mve::image::save_png_file(mve::image::float_to_byte_image(img2), "/tmp/output2.png");
#endif

#if 0
    /* Test TIFF image reading and writing. */
    mve::Image<uint16_t>::Ptr testPattern = mve::Image<uint16_t>::create(100, 100, 3);
    for (std::size_t x=0; x < testPattern->width(); x++) {
    for (std::size_t y=0; y < testPattern->height(); y++) {
        math::Vec3ui color;
        if (y < testPattern->height()/2) {
        if (x < testPattern->width()/2) {
            color = math::Vec3ui(0xFFFF, 0, 0);
        }
        else {
            color = math::Vec3ui(0, 0xFFFF, 0);
        }
        }
        else {
        if (x < testPattern->width()/2) {
            color = math::Vec3ui(0, 0, 0xFFFF);
        }
        else {
            color = math::Vec3ui(0xFFFF, 0xFFFF, 0);
        }
        }
        for (std::size_t c=0; c<3; c++) {
        testPattern->at(x,y,c) = color[c];
        }
    }
    }

    /* Test 16bit TIFF file writing */
    mve::image::save_tiff_16_file(testPattern, "/tmp/test16bit.tiff");

    /* Test 16bit TIFF file reading */
    mve::Image<uint16_t>::Ptr img = mve::image::load_tiff_16_file("/tmp/test16bit.tiff");
    std::cout << "Size: " << img->width() << ", " << img->height() << ", " << img->channels() << std::endl;

    /* check consistency */
    mve::image::save_tiff_16_file(img, "/tmp/test16bit2.tiff");
#endif

    return 0;
}

