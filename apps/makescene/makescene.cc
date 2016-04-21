/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 *
 * App to create MVE scenes from images and bundles. The app supports:
 * - Import of calibrated images from Photosynther and Noah bundler
 * - Import of calibrated images from VisualSfM
 * - Import of uncalibrated 8 bit, 16 bit or float images from a directory
 *   8 bit formats: JPEG, PNG, TIFF, PPM
 *   16 bit formats: TIFF, PPM
 *   float formats: PFM
 */

#include <cstring>
#include <cstdlib>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <atomic>

#include "math/matrix.h"
#include "math/matrix_tools.h"
#include "math/algo.h"
#include "util/system.h"
#include "util/timer.h"
#include "util/arguments.h"
#include "util/string.h"
#include "util/file_system.h"
#include "util/tokenizer.h"
#include "mve/bundle.h"
#include "mve/bundle_io.h"
#include "mve/view.h"
#include "mve/image.h"
#include "mve/image_tools.h"
#include "mve/image_io.h"
#include "mve/image_exif.h"
#include "mve/mesh.h"
#include "mve/mesh_tools.h"

#define THUMBNAIL_SIZE 50

#define BUNDLE_PATH "bundle/"
#define PS_BUNDLE_LOG "coll.log"
#define PS_IMAGE_DIR "images/"
#define PS_UNDIST_DIR "undistorted/"
#define BUNDLER_FILE_LIST "list.txt"
#define BUNDLER_IMAGE_DIR ""
#define VIEWS_DIR "views/"

typedef std::vector<std::string> StringVector;
typedef std::vector<util::fs::File> FileVector;
typedef FileVector::iterator FileIterator;

/* ---------------------------------------------------------------- */

struct AppSettings
{
    std::string input_path;
    std::string output_path;
    int bundle_id = 0;
    bool import_orig = false;
    bool skip_invalid = true;
    bool images_only = false;
    bool append_images = false;
    int max_pixels = std::numeric_limits<int>::max();

    /* Computed values. */
    std::string bundle_path;
    std::string views_path;
};

/* ---------------------------------------------------------------- */

void
wait_for_user_confirmation (void)
{
    std::cerr << "-> Press ENTER to continue, or CTRL-C to exit." << std::endl;
    std::string line;
    std::getline(std::cin, line);
}

/* ---------------------------------------------------------------- */

void
read_noah_imagelist (std::string const& filename, StringVector& files)
{
    /*
     * The list of the original images is read from the list.txt file.
     */
    std::ifstream in(filename.c_str(), std::ios::binary);
    if (!in.good())
    {
        std::cerr << "Error: Cannot read bundler list file!" << std::endl;
        std::cerr << "File: " << filename << std::endl;
        std::exit(EXIT_FAILURE);
    }

    while (true)
    {
        std::string file, dummy;
        in >> file;
        std::getline(in, dummy);
        if (file.empty())
            break;
        files.push_back(file);
    }

    in.close();
}

/* ---------------------------------------------------------------- */

mve::ByteImage::Ptr
load_8bit_image (std::string const& fname, std::string* exif)
{
    std::string lcfname(util::string::lowercase(fname));
    std::string ext4 = util::string::right(lcfname, 4);
    std::string ext5 = util::string::right(lcfname, 5);
    try
    {
        if (ext4 == ".jpg" || ext5 == ".jpeg")
            return mve::image::load_jpg_file(fname, exif);
        else if (ext4 == ".png" ||  ext4 == ".ppm"
            || ext4 == ".tif" || ext5 == ".tiff")
            return mve::image::load_file(fname);
    }
    catch (...)
    {
        throw;
    }

    return mve::ByteImage::Ptr();
}

/* ---------------------------------------------------------------- */

mve::RawImage::Ptr
load_16bit_image (std::string const& fname)
{
    std::string lcfname(util::string::lowercase(fname));
    std::string ext4 = util::string::right(lcfname, 4);
    std::string ext5 = util::string::right(lcfname, 5);
    try
    {
        if (ext4 == ".tif" || ext5 == ".tiff")
            return mve::image::load_tiff_16_file(fname);
        else if (ext4 == ".ppm")
            return mve::image::load_ppm_16_file(fname);
    }
    catch (...)
    { }

    return mve::RawImage::Ptr();
}

/* ---------------------------------------------------------------- */

mve::FloatImage::Ptr
load_float_image (std::string const& fname)
{
    std::string lcfname(util::string::lowercase(fname));
    std::string ext4 = util::string::right(lcfname, 4);
    try
    {
        if (ext4 == ".pfm")
            return mve::image::load_pfm_file(fname);
    }
    catch (...)
    { }

    return mve::FloatImage::Ptr();
}

/* ---------------------------------------------------------------- */

mve::ImageBase::Ptr
load_any_image (std::string const& fname, std::string* exif)
{
    mve::ByteImage::Ptr img_8 = load_8bit_image(fname, exif);
    if (img_8 != nullptr)
        return img_8;

    mve::RawImage::Ptr img_16 = load_16bit_image(fname);
    if (img_16 != nullptr)
        return img_16;

    mve::FloatImage::Ptr img_float = load_float_image(fname);
    if (img_float != nullptr)
        return img_float;

#pragma omp critical
    std::cerr << "Skipping file " << util::fs::basename(fname)
        << ", cannot load image." << std::endl;
    return mve::ImageBase::Ptr();
}

/* ---------------------------------------------------------------- */

template <typename T>
void
find_min_max_percentile (typename mve::Image<T>::ConstPtr image,
    T* vmin, T* vmax)
{
    typename mve::Image<T>::Ptr copy = mve::Image<T>::create(*image);
    std::sort(copy->begin(), copy->end());
    *vmin = copy->at(copy->get_value_amount() / 10);
    *vmax = copy->at(9 * copy->get_value_amount() / 10);
}

/* ---------------------------------------------------------------- */

void
add_exif_to_view (mve::View::Ptr view, std::string const& exif)
{
    if (exif.empty())
        return;

    mve::ByteImage::Ptr exif_image = mve::ByteImage::create(exif.size(), 1, 1);
    std::copy(exif.begin(), exif.end(), exif_image->begin());
    view->set_blob(exif_image, "exif");
}

/* ---------------------------------------------------------------- */

mve::ByteImage::Ptr
create_thumbnail (mve::ImageBase::ConstPtr img)
{
    mve::ByteImage::Ptr image;
    switch (img->get_type())
    {
        case mve::IMAGE_TYPE_UINT8:
            image = mve::image::create_thumbnail<uint8_t>
                (std::dynamic_pointer_cast<mve::ByteImage const>(img),
                THUMBNAIL_SIZE, THUMBNAIL_SIZE);
            break;

        case mve::IMAGE_TYPE_UINT16:
        {
            mve::RawImage::Ptr temp = mve::image::create_thumbnail<uint16_t>
                (std::dynamic_pointer_cast<mve::RawImage const>(img),
                THUMBNAIL_SIZE, THUMBNAIL_SIZE);
            uint16_t vmin, vmax;
            find_min_max_percentile(temp, &vmin, &vmax);
            image = mve::image::raw_to_byte_image(temp, vmin, vmax);
            break;
        }

        case mve::IMAGE_TYPE_FLOAT:
        {
            mve::FloatImage::Ptr temp = mve::image::create_thumbnail<float>
                (std::dynamic_pointer_cast<mve::FloatImage const>(img),
                THUMBNAIL_SIZE, THUMBNAIL_SIZE);
            float vmin, vmax;
            find_min_max_percentile(temp, &vmin, &vmax);
            image = mve::image::float_to_byte_image(temp, vmin, vmax);
            break;
        }

        default:
            return mve::ByteImage::Ptr();
    }

    return image;
}

/* ---------------------------------------------------------------- */

std::string
remove_file_extension (std::string const& filename)
{
    std::size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos)
        return filename.substr(0, pos);
    return filename;
}

/* ---------------------------------------------------------------- */

template <class T>
typename mve::Image<T>::Ptr
limit_image_size (typename mve::Image<T>::Ptr img, int max_pixels)
{
    while (img->get_pixel_amount() > max_pixels)
        img = mve::image::rescale_half_size<T>(img);
    return img;
}

/* ---------------------------------------------------------------- */

mve::ImageBase::Ptr
limit_image_size (mve::ImageBase::Ptr image, int max_pixels)
{
    switch (image->get_type())
    {
        case mve::IMAGE_TYPE_FLOAT:
            return limit_image_size<float>(std::dynamic_pointer_cast
                <mve::FloatImage>(image), max_pixels);
        case mve::IMAGE_TYPE_UINT8:
            return limit_image_size<uint8_t>(std::dynamic_pointer_cast
                <mve::ByteImage>(image), max_pixels);
        case mve::IMAGE_TYPE_UINT16:
            return limit_image_size<uint16_t>(std::dynamic_pointer_cast
                <mve::RawImage>(image), max_pixels);
        default:
            break;
    }
    return mve::ImageBase::Ptr();
}

/* ---------------------------------------------------------------- */

bool
has_jpeg_extension (std::string const& filename)
{
    std::string lcfname(util::string::lowercase(filename));
    return util::string::right(lcfname, 4) == ".jpg"
        || util::string::right(lcfname, 5) == ".jpeg";
}

/* ---------------------------------------------------------------- */

std::string
make_image_name (int id)
{
    return "view_" + util::string::get_filled(id, 4) + ".mve";
}

/* ---------------------------------------------------------------- */

void
import_bundle_nvm (AppSettings const& conf)
{
    std::vector<mve::NVMCameraInfo> nvm_cams;
    mve::Bundle::Ptr bundle = mve::load_nvm_bundle(conf.input_path, &nvm_cams);
    mve::Bundle::Cameras& cameras = bundle->get_cameras();

    if (nvm_cams.size() != cameras.size())
    {
        std::cerr << "Error: NVM info inconsistent with bundle!" << std::endl;
        return;
    }

    /* Create output directories. */
    std::cout << "Creating output directories..." << std::endl;
    util::fs::mkdir(conf.output_path.c_str());
    util::fs::mkdir(conf.views_path.c_str());

    /* Create and write views. */
    std::cout << "Writing MVE views..." << std::endl;
#pragma omp parallel for schedule(dynamic, 1)
    for (std::size_t i = 0; i < cameras.size(); ++i)
    {
        mve::CameraInfo& mve_cam = cameras[i];
        mve::NVMCameraInfo const& nvm_cam = nvm_cams[i];
        std::string fname = "view_" + util::string::get_filled(i, 4) + ".mve";

        mve::View::Ptr view = mve::View::create();
        view->set_id(i);
        view->set_name(util::string::get_filled(i, 4, '0'));

        /* Load original image. */
        std::string exif;
        mve::ByteImage::Ptr image = load_8bit_image(nvm_cam.filename, &exif);
        if (image == nullptr)
        {
            std::cout << "Error loading: " << nvm_cam.filename
                << " (skipping " << fname << ")" << std::endl;
            continue;
        }

        /* Add original image. */
        if (conf.import_orig)
        {
            if (has_jpeg_extension(nvm_cam.filename))
                view->set_image_ref(nvm_cam.filename, "original");
            else
                view->set_image(image, "original");
        }
        view->set_image(create_thumbnail(image), "thumbnail");
        add_exif_to_view(view, exif);

        /* Normalize focal length, add undistorted image. */
        int const maxdim = std::max(image->width(), image->height());
        mve_cam.flen = mve_cam.flen / static_cast<float>(maxdim);

        mve::ByteImage::Ptr undist = mve::image::image_undistort_vsfm<uint8_t>
            (image, mve_cam.flen, nvm_cam.radial_distortion);
        undist = limit_image_size<uint8_t>(undist, conf.max_pixels);
        view->set_image(undist, "undistorted");
        view->set_camera(mve_cam);

        /* Save view. */
#pragma omp critical
        std::cout << "Writing MVE view: " << fname << "..." << std::endl;
        view->save_view_as(util::fs::join_path(conf.views_path, fname));
    }

    /* Use MVE to write MVE bundle file. */
    std::cout << "Writing bundle file..." << std::endl;
    std::string bundle_filename
        = util::fs::join_path(conf.output_path, "synth_0.out");
    mve::save_mve_bundle(bundle, bundle_filename);

    std::cout << std::endl << "Done importing NVM file!" << std::endl;
}

/* ---------------------------------------------------------------- */

namespace
{
    enum BundleFormat
    {
        BUNDLE_FORMAT_NOAH_BUNDLER,
        BUNDLE_FORMAT_PHOTOSYNTHER,
        BUNDLE_FORMAT_UNKNOWN
    };
}

void
import_bundle (AppSettings const& conf)
{
    /**
     * Try to detect VisualSFM bundle format.
     * In this case the input is a file with extension ".nvm".
     */
    if (util::string::right(conf.input_path, 4) == ".nvm"
        && util::fs::file_exists(conf.input_path.c_str()))
    {
        std::cout << "Info: Detected VisualSFM bundle format." << std::endl;
        import_bundle_nvm(conf);
        return;
    }

    /* Build some paths. */
    std::string bundle_fname;
    std::string imglist_file;
    std::string image_path;
    std::string undist_path;
    BundleFormat bundler_fmt = BUNDLE_FORMAT_UNKNOWN;
    bool import_original = conf.import_orig;

    /*
     * Try to detect Photosynther software. This is detected if the
     * file synth_N.out (for bundle N) is present in the bundler dir.
     */
    if (bundler_fmt == BUNDLE_FORMAT_UNKNOWN)
    {
        bundle_fname = "synth_" + util::string::get(conf.bundle_id) + ".out";
        bundle_fname = util::fs::join_path(conf.bundle_path, bundle_fname);
        imglist_file = util::fs::join_path(conf.input_path, PS_BUNDLE_LOG);
        image_path = util::fs::join_path(conf.input_path, PS_IMAGE_DIR);
        undist_path = util::fs::join_path(conf.input_path, PS_UNDIST_DIR);

        if (util::fs::file_exists(bundle_fname.c_str()))
        {
            std::cout << "Info: Detected Photosynther format." << std::endl;
            bundler_fmt = BUNDLE_FORMAT_PHOTOSYNTHER;
        }
    }

    /*
     * Try to detect Noah bundler software. Noah bundler is detected if
     * file bundle.out (for bundle 0) or bundle_%03d.out (for bundle > 0)
     * is present in the bundler directory.
     */
    if (bundler_fmt == BUNDLE_FORMAT_UNKNOWN)
    {
        if (conf.bundle_id > 0)
            bundle_fname = "bundle_" + util::string::get_filled
                (conf.bundle_id, 3, '0') + ".out";
        else
            bundle_fname = "bundle.out";

        bundle_fname = util::fs::join_path(conf.bundle_path, bundle_fname);
        imglist_file = util::fs::join_path(conf.input_path, BUNDLER_FILE_LIST);
        image_path = util::fs::join_path(conf.input_path, BUNDLER_IMAGE_DIR);

        if (util::fs::file_exists(bundle_fname.c_str()))
        {
            std::cout << "Info: Detected Noah's Bundler format." << std::endl;
            bundler_fmt = BUNDLE_FORMAT_NOAH_BUNDLER;
        }
    }

    /* Read bundle file. */
    mve::Bundle::Ptr bundle;
    try
    {
        if (bundler_fmt == BUNDLE_FORMAT_NOAH_BUNDLER)
            bundle = mve::load_bundler_bundle(bundle_fname);
        else if (bundler_fmt == BUNDLE_FORMAT_PHOTOSYNTHER)
            bundle = mve::load_photosynther_bundle(bundle_fname);
        else
        {
            std::cerr << "Error: Could not detect bundle format." << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Error reading bundle: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    /* Read the list of original images filenames. */
    StringVector orig_files;
    if (bundler_fmt == BUNDLE_FORMAT_PHOTOSYNTHER && import_original)
    {
        std::cerr << std::endl << "** Warning: Original images cannot be "
            << "imported from Photosynther." << std::endl;
        wait_for_user_confirmation();
        import_original = false;
    }
    else if (bundler_fmt == BUNDLE_FORMAT_NOAH_BUNDLER)
    {
        /*
         * Each camera in the bundle file corresponds to the ordered list of
         * input images. Some cameras are set to zero, which means the input
         * image was not registered. The paths of original images is required
         * because Bundler does not compute the undistorted images.
         */
        read_noah_imagelist(imglist_file, orig_files);
        if (orig_files.empty())
        {
            std::cerr << "Error: Empty list of original images." << std::endl;
            std::exit(EXIT_FAILURE);
        }
        if (orig_files.size() != bundle->get_num_cameras())
        {
            std::cerr << "Error: Invalid amount of original images." << std::endl;
            std::exit(EXIT_FAILURE);
        }
        std::cout << "Recognized " << orig_files.size()
            << " original images from Noah's Bundler." << std::endl;
    }

    /* ------------------ Start importing views ------------------- */

    /* Create destination directories. */
    std::cout << "Creating output directories..." << std::endl;
    util::fs::mkdir(conf.output_path.c_str());
    util::fs::mkdir(conf.views_path.c_str());

    /* Save bundle file. */
    std::cout << "Saving bundle file..." << std::endl;
    mve::save_photosynther_bundle(bundle,
        util::fs::join_path(conf.output_path, "synth_0.out"));

    /* Save MVE views. */
    int num_valid_cams = 0;
    int undist_imported = 0;
    mve::Bundle::Cameras const& cams = bundle->get_cameras();
    for (std::size_t i = 0; i < cams.size(); ++i)
    {
        /*
         * For each camera in the bundle file, a new view is created.
         * Views are populated with ID, name, camera information,
         * undistorted RGB image, and optionally the original RGB image.
         */
        std::string fname = "view_" + util::string::get_filled(i, 4) + ".mve";
        std::cout << "Processing view " << fname << "..." << std::endl;

        /* Skip invalid cameras... */
        mve::CameraInfo cam = cams[i];
        if (cam.flen == 0.0f && (conf.skip_invalid
            || bundler_fmt == BUNDLE_FORMAT_PHOTOSYNTHER))
        {
            std::cerr << "  Skipping " << fname
                << ": Invalid camera." << std::endl;
            continue;
        }

        /* Extract name of view from original image or sequentially. */
        std::string view_name = (import_original
            ? remove_file_extension(util::fs::basename(orig_files[i]))
            : util::string::get_filled(i, 4, '0'));

        /* Convert from Photosynther camera conventions. */
        if (bundler_fmt == BUNDLE_FORMAT_PHOTOSYNTHER)
        {
            /* Nothing to do here. */
        }

        /* Fix issues with Noah Bundler camera specification. */
        if (bundler_fmt == BUNDLE_FORMAT_NOAH_BUNDLER)
        {
            /* Check focal length of camera, fix negative focal length. */
            if (cam.flen < 0.0f)
            {
                std::cout << "  Fixing focal length for " << fname << std::endl;
                cam.flen = -cam.flen;
                std::for_each(cam.rot, cam.rot + 9,
                    math::algo::foreach_negate_value<float>);
                std::for_each(cam.trans, cam.trans + 3,
                    math::algo::foreach_negate_value<float>);
            }

            /* Convert from Noah Bundler camera conventions. */
            std::for_each(cam.rot + 3, cam.rot + 9,
                math::algo::foreach_negate_value<float>);
            std::for_each(cam.trans + 1, cam.trans + 3,
                math::algo::foreach_negate_value<float>);

            /* Check determinant of rotation matrix. */
            math::Matrix3f rmat(cam.rot);
            float rmatdet = math::matrix_determinant(rmat);
            if (rmatdet < 0.0f)
            {
                std::cerr << "  Skipping " << fname
                    << ": Bad rotation matrix." << std::endl;
                continue;
            }
        }

        /* Create view and set headers. */
        mve::View::Ptr view = mve::View::create();
        view->set_id(i);
        view->set_name(view_name);
        view->set_camera(cam);

        /* FIXME: Handle exceptions while loading images? */

        /* Load undistorted and original image, create thumbnail. */
        mve::ByteImage::Ptr original, undist, thumb;
        std::string exif;
        if (bundler_fmt == BUNDLE_FORMAT_NOAH_BUNDLER)
        {
            /* For Noah datasets, load original image and undistort it. */
            std::string orig_fname
                = util::fs::join_path(image_path, orig_files[i]);
            try
            {
                original = load_8bit_image(orig_fname, &exif);
            }
            catch (util::Exception &e)
            {
                std::cerr << orig_fname << ": " << e.what() << std::endl;
                std::exit(EXIT_FAILURE);
            }
            thumb = create_thumbnail(original);

            /* Convert Bundler's focal length to MVE focal length. */
            cam.flen /= (float)std::max(original->width(), original->height());
            view->set_camera(cam);

            if (cam.flen != 0.0f)
                undist = mve::image::image_undistort_bundler<uint8_t>
                    (original, cam.flen, cam.dist[0], cam.dist[1]);

            if (!import_original)
                original.reset();
        }
        else if (bundler_fmt == BUNDLE_FORMAT_PHOTOSYNTHER)
        {
            /*
			 * Depending on the version and availability of images in original resolution,
				try to load three file names:
			 * New version, level 0 = original size: l0/imgyyyy.jpg
             * New version: forStereo_xxxx_yyyy.png
             * Old version: undistorted_xxxx_yyyy.jpg
             */
			std::string undist_filename
				= util::fs::join_path(undist_path, "l0/img"
				+ util::string::get_filled(num_valid_cams, 4) + ".jpg");

            std::string undist_new_filename
                = util::fs::join_path(undist_path, "forStereo_"
                + util::string::get_filled(conf.bundle_id, 4) + "_"
                + util::string::get_filled(num_valid_cams, 4) + ".png");

            std::string undist_old_filename
                = util::fs::join_path(undist_path, "undistorted_"
                + util::string::get_filled(conf.bundle_id, 4) + "_"
                + util::string::get_filled(num_valid_cams, 4) + ".jpg");

            /* Try the newer file name and fall back if not existing. */
            try
            {
				if (util::fs::file_exists(undist_filename.c_str()))
					undist = mve::image::load_file(undist_filename);
				else if (util::fs::file_exists(undist_new_filename.c_str()))
                    undist = mve::image::load_file(undist_new_filename);
                else
                    undist = mve::image::load_file(undist_old_filename);
            }
            catch (util::FileException &e)
            {
                std::cerr << e.filename << ": " << e.what() << std::endl;
                std::exit(EXIT_FAILURE);
            }
            catch (util::Exception &e)
            {
                std::cerr << e.what() << std::endl;
                std::exit(EXIT_FAILURE);
            }

            /* Create thumbnail. */
            thumb = create_thumbnail(undist);
        }

        /* Add images to view. */
        if (thumb != nullptr)
            view->set_image(thumb, "thumbnail");

        if (undist != nullptr)
        {
            undist = limit_image_size<uint8_t>(undist, conf.max_pixels);
            view->set_image(undist, "undistorted");
        }
        else if (cam.flen != 0.0f && undist == nullptr)
            std::cerr << "Warning: Undistorted image missing!" << std::endl;

        if (original != nullptr)
            view->set_image(original, "original");
        if (original == nullptr && import_original)
            std::cerr << "Warning: Original image missing!" << std::endl;

        /* Add EXIF data to view if available. */
        add_exif_to_view(view, exif);

        /* Save MVE file. */
        view->save_view_as(util::fs::join_path(conf.views_path, fname));

        if (cam.flen != 0.0f)
            num_valid_cams += 1;
        if (undist != nullptr)
            undist_imported += 1;
    }

    std::cout << std::endl;
    std::cout << "Created " << cams.size() << " views with "
        << num_valid_cams << " valid cameras." << std::endl;
    std::cout << "Imported " << undist_imported
        << " undistorted images." << std::endl;
}

/* ---------------------------------------------------------------- */

int
find_max_scene_id (std::string const& view_path)
{
    util::fs::Directory dir;
    try { dir.scan(view_path); }
    catch (...) { return -1; }

    /* Load all MVE files and remember largest view ID. */
    int max_view_id = 0;
    for (std::size_t i = 0; i < dir.size(); ++i)
    {
        std::string ext4 = util::string::right(dir[i].name, 4);
        if (ext4 != ".mve")
            continue;

        mve::View::Ptr view;
        try
        { view = mve::View::create(dir[i].get_absolute_name()); }
        catch (...)
        {
            std::cerr << "Error reading " << dir[i].name << std::endl;
            continue;
        }

        max_view_id = std::max(max_view_id, view->get_id());
    }

    return max_view_id;
}

/* ---------------------------------------------------------------- */

void
import_images (AppSettings const& conf)
{
    util::WallTimer timer;

    util::fs::Directory dir;
    try { dir.scan(conf.input_path); }
    catch (std::exception& e)
    {
        std::cerr << "Error scanning input dir: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "Found " << dir.size() << " directory entries." << std::endl;

    /* ------------------ Start importing images ------------------- */

    /* Create destination dir. */
    if (!conf.append_images)
    {
        std::cout << "Creating output directories..." << std::endl;
        util::fs::mkdir(conf.output_path.c_str());
        util::fs::mkdir(conf.views_path.c_str());
    }

    int max_scene_id = -1;
    if (conf.append_images)
    {
        max_scene_id = find_max_scene_id(conf.views_path);
        if (max_scene_id < 0)
        {
            std::cerr << "Error: Cannot find view ID for appending." << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    /* Sort file names, iterate over file names. */
    std::sort(dir.begin(), dir.end());
    std::atomic_int id_cnt(max_scene_id + 1);
    std::atomic_int num_imported(0);
#pragma omp parallel for ordered schedule(dynamic,1)
    for (std::size_t i = 0; i < dir.size(); ++i)
    {
        if (dir[i].is_dir)
        {
#pragma omp critical
            std::cout << "Skipping directory " << dir[i].name << std::endl;
            continue;
        }

        std::string fname = dir[i].name;
        std::string afname = dir[i].get_absolute_name();

        std::string exif;
        mve::ImageBase::Ptr image = load_any_image(afname, &exif);
        if (image == nullptr)
            continue;

        /* Advance ID of successfully imported images. */
        int id;
#pragma omp ordered
        id = id_cnt++;
        num_imported += 1;

        /* Create view, set headers, add image. */
        mve::View::Ptr view = mve::View::create();
        view->set_id(id);
        view->set_name(remove_file_extension(fname));

        /* Rescale and add original image. */
        int orig_width = image->width();
        image = limit_image_size(image, conf.max_pixels);
        if (orig_width == image->width() && has_jpeg_extension(fname))
            view->set_image_ref(afname, "original");
        else
            view->set_image(image, "original");

        /* Add thumbnail for byte images. */
        mve::ByteImage::Ptr thumb = create_thumbnail(image);
        if (thumb != nullptr)
            view->set_image(thumb, "thumbnail");

        /* Add EXIF data to view if available. */
        add_exif_to_view(view, exif);

        /* Save view to disc. */
        std::string mve_fname = make_image_name(id);
#pragma omp critical
        std::cout << "Importing image: " << fname
            << ", writing MVE view: " << mve_fname << "..." << std::endl;
        view->save_view_as(util::fs::join_path(conf.views_path, mve_fname));
    }

    std::cout << "Imported " << num_imported << " input images, "
        << "took " << timer.get_elapsed() << " ms." << std::endl;
}

/* ---------------------------------------------------------------- */

int
main (int argc, char** argv)
{
    util::system::register_segfault_handler();
    util::system::print_build_timestamp("MVE Makescene");

    /* Setup argument parser. */
    util::Arguments args;
    args.set_usage(argv[0], "[ OPTIONS ] INPUT OUT_SCENE");
    args.set_exit_on_error(true);
    args.set_nonopt_maxnum(2);
    args.set_nonopt_minnum(2);
    args.set_helptext_indent(22);
    args.set_description("This utility creates MVE scenes by importing "
        "from an external SfM software. Supported are Noah's Bundler, "
        "Photosynther and VisualSfM's compact .nvm file.\n\n"

        "For VisualSfM, makescene expects the .nvm file as INPUT. "
        "With VisualSfM, it is not possible to keep invalid views.\n\n"

        "For Noah's Bundler, makescene expects the bundle directory as INPUT, "
        "a file \"list.txt\" in INPUT and the bundle file in the "
        "\"bundle\" directory.\n\n"

        "For Photosynther, makescene expects as INPUT the directory that "
        "contains the \"bundle\" and the \"undistorted\" directory. "
        "With Photosynther, it is not possible to keep invalid views "
        "or import original images.\n\n"

        "With the \"images-only\" option, all images in the INPUT directory "
        "are imported without camera information. If \"append-images\" is "
        "specified, images are added to an existing scene.");
    args.add_option('o', "original", false, "Import original images");
    args.add_option('b', "bundle-id", true, "Bundle ID (Photosynther and Bundler only) [0]");
    args.add_option('k', "keep-invalid", false, "Keeps images with invalid cameras");
    args.add_option('i', "images-only", false, "Imports images from INPUT_DIR only");
    args.add_option('a', "append-images", false, "Appends images to an existing scene");
    args.add_option('m', "max-pixels", true, "Limit image size by iterative half-sizing");
    args.parse(argc, argv);

    /* Setup defaults. */
    AppSettings conf;
    conf.input_path = util::fs::sanitize_path(args.get_nth_nonopt(0));
    conf.output_path = util::fs::sanitize_path(args.get_nth_nonopt(1));

    /* General settings. */
    for (util::ArgResult const* i = args.next_option();
        i != nullptr; i = args.next_option())
    {
        if (i->opt->lopt == "original")
            conf.import_orig = true;
        else if (i->opt->lopt == "bundle-id")
            conf.bundle_id = i->get_arg<int>();
        else if (i->opt->lopt == "keep-invalid")
            conf.skip_invalid = false;
        else if (i->opt->lopt == "images-only")
            conf.images_only = true;
        else if (i->opt->lopt == "append-images")
            conf.append_images = true;
        else if (i->opt->lopt == "max-pixels")
            conf.max_pixels = i->get_arg<int>();
        else
            throw std::invalid_argument("Unexpected option");
    }

    /* Check command line arguments. */
    if (conf.input_path.empty() || conf.output_path.empty())
    {
        args.generate_helptext(std::cerr);
        return EXIT_FAILURE;
    }
    conf.views_path = util::fs::join_path(conf.output_path, VIEWS_DIR);
    conf.bundle_path = util::fs::join_path(conf.input_path, BUNDLE_PATH);

    if (conf.append_images && !conf.images_only)
    {
        std::cerr << "Error: Cannot --append-images without --images-only."
            << std::endl;
        return EXIT_FAILURE;
    }

    /* Check if output dir exists. */
    bool output_path_exists = util::fs::dir_exists(conf.output_path.c_str());
    if (output_path_exists && !conf.append_images)
    {
        std::cerr << std::endl;
        std::cerr << "** Warning: Output dir already exists." << std::endl;
        std::cerr << "** This may leave old views in your scene." << std::endl;
        wait_for_user_confirmation();
    }
    else if (!output_path_exists && conf.append_images)
    {
        std::cerr << "Error: Output dir does not exist. Cannot append images."
            << std::endl;
        return EXIT_FAILURE;
    }

    if (conf.images_only)
        import_images(conf);
    else
        import_bundle(conf);

    return EXIT_SUCCESS;
}
