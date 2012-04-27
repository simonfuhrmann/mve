#include <iostream>

#include "util/system.h"

#include "imagefile.h"
#include "imageexif.h"
#include "view.h"

int
main (int argc, char** argv)
{
#if 1
    /* Provoke view corruption. */

    /*
     * Failure case 1
     * --------------
     * P1: Read view
     * P2: Read view
     * P1: Modifiy and save view
     * P2: Read embedding (may read wrong data because view changed)
     */

    {
        /* Generate a view. */
        mve::ByteImage::Ptr img1 = mve::image::load_file("../../data/testimages/test_rgb_32x32.png");
        mve::ByteImage::Ptr img2 = mve::image::load_file("../../data/testimages/test_grey_32x32.png");
        mve::View::Ptr view = mve::View::create();
        view->set_name("View 123");
        view->set_camera(mve::CameraInfo());
        view->add_image("color-pattern", img1);
        view->add_image("gray-pattern", img2);
        view->save_mve_file_as("/tmp/myview.mve");
    }

    {
        /* Simulate two competing processes. */
        mve::View::Ptr view1 = mve::View::create("/tmp/myview.mve");
        mve::View::Ptr view2 = mve::View::create("/tmp/myview.mve");

        /* Change view1. */
        mve::ByteImage::Ptr img1 = mve::image::load_file("../../data/testimages/diaz_color.png");
        view1->set_image("color-pattern", img1);
        view1->save_mve_file();

        /* Retrieve image from view2. */
        mve::ByteImage::Ptr img2 = view2->get_byte_image("color-pattern");
        mve::image::save_file(img2, "/tmp/myresult.png");
    }


    /*
     * Failure case 2
     * --------------
     * P1 and P2: read view
     * P1 adds embeddings, saves view
     * P2 adds embeddings, saves view (issues "save as",
     *   reads uncached embeddings withoug re-reading (changed) headers)
     *
     * Practical failure case: MVS on two scales on the same view:
     * ./dmrecon -s1 -l0 DATASET_DIR
     * ./dmrecon -s2 -l0 DATASET_DIR
     */




#endif


#if 0
#   define MVE_FILE "/tmp/myscene/views/view_0000.mve"
#   define VIEW_CLASS View

    std::cout << "--- View tests ---" << std::endl;
    if (argc != 2)
    {
        std::cout << "Pass a command!" << std::endl;
        return 1;
    }

    if (std::string(argv[1]) == "build")
    {
        mve::ByteImage::Ptr img1;
        img1 = mve::image::load_file("../../data/testimages/test_rgb_32x32.png");

        mve::ByteImage::Ptr img2;
        img2 = mve::image::load_file("../../data/testimages/test_grey_32x32.png");

        mve::VIEW_CLASS::Ptr view(mve::VIEW_CLASS::create());
        view->set_name("View 123");
        view->set_camera(mve::CameraInfo());

        view->add_image("color-pattern", img1);
        view->add_image("gray-pattern", img2);

        view->save_mve_file_as(MVE_FILE);
        return 0;
    }

    mve::VIEW_CLASS::Ptr view(mve::VIEW_CLASS::create());
    view->load_mve_file(MVE_FILE);
    view->print_debug(); std::cout << std::endl;

    if (std::string(argv[1]) == "name-modify")
    {
        view->set_name("testview123");
        view->save_mve_file();
    }

    if (std::string(argv[1]) == "addimage")
    {
        mve::ByteImage::Ptr img1;
        img1 = mve::image::load_file("../../data/testimages/diaz_color.png");
        view->set_image("testimage", img1);

        view->save_mve_file();
        view->save_mve_file();
    }

    if (std::string(argv[1]) == "noop")
    {
        view->save_mve_file();
    }

    if (std::string(argv[1]) == "exif")
    {
        mve::ByteImage::Ptr exif = view->get_data("exif");
        char const* exif_data = reinterpret_cast<char const*>(exif->begin());
        mve::image::ExifInfo einfo;
        einfo = mve::image::exif_extract(exif_data, exif->width());
        mve::image::exif_debug_print(einfo);
    }

    if (std::string(argv[1]) == "export")
    {
        mve::ByteImage::Ptr img(view->get_byte_image("color-pattern"));
        if (img.get() == 0)
            std::cout << "Warning: Image was not properly loaded" << std::endl;
        else
        {
            std::cout << "Saving image to file..." << std::endl;
            mve::image::save_file(img, "/tmp/colorpattern.png");
        }
    }

    if (std::string(argv[1]) == "cachecleanup")
    {
        mve::ByteImage::Ptr keep(view->get_byte_image("color-pattern"));
        view->get_byte_image("testimage");
        view->print_debug(); std::cout << std::endl;
        view->cache_cleanup();
        view->print_debug(); std::cout << std::endl;
    }

    if (std::string(argv[1]) == "writeback")
    {
        mve::ByteImage::Ptr keep(view->get_byte_image("color-pattern"));
        view->set_image("color-pattern", keep);
        view->print_debug(); std::cout << std::endl;
        keep.reset();
        view->cache_cleanup();
        view->print_debug(); std::cout << std::endl;

        view->save_mve_file();
    }

    if (std::string(argv[1]) == "nastysave")
    {
        //mve::ByteImage::Ptr keep(view->get_byte_image("color-pattern"));
        //view->set_image("color-pattern", keep);
        view->save_mve_file_as(MVE_FILE);
    }


    if (std::string(argv[1]) == "stresstest")
    {
        for (int j = 0; j < 10; ++j)
        {
#pragma omp parallel for
            for (int i = 0; i < 2; ++i)
            {
                view->get_image("testimage");
            }
            view->cache_cleanup();
        }
    }
#endif

    return 0;
}


