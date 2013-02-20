/*
 * Simple app to import calibrated images from the Photosynth bundler,
 * and the MVS reconstruction if available.
 * Written by Simon Fuhrmann.
 */

#include <cstring>
#include <cstdlib>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

#include "math/matrix.h" // for rot matrix determinant
#include "math/matrixtools.h" // for rot matrix determinant
#include "math/algo.h"
#include "util/arguments.h"
#include "util/string.h"
#include "util/filesystem.h"
#include "util/tokenizer.h"
#include "mve/bundlefile.h"
#include "mve/view.h"
#include "mve/image.h"
#include "mve/imagetools.h"
#include "mve/imagefile.h"
#include "mve/imageexif.h" // extract EXIF for JPEG images

#define THUMB_SIZE 50
#define THUMB_SIZEF 50.0f

#define BUNDLE_PATH "bundle/"
#define PS_BUNDLE_LOG "coll.log"
#define PS_IMAGE_DIR "images/"
#define PS_UNDIST_DIR "undistorted/"
#define NOAH_BUNDLE_LIST "list.txt"
#define NOAH_IMAGE_DIR ""
#define VIEWS_DIR "views/"

typedef std::vector<std::string> StringVector;
typedef std::vector<util::fs::File> FileVector;
typedef FileVector::iterator FileIterator;

/* ---------------------------------------------------------------- */

struct AppSettings
{
    std::string input_dir;
    std::string output_dir;
    int bundle_id;
    bool import_orig;
    bool skip_invalid;
    bool images_only;

    /* Computed values. */
    std::string bundle_path;
    std::string views_path;
};

/* ---------------------------------------------------------------- */

mve::ByteImage::Ptr
get_thumbnail (mve::ByteImage::ConstPtr image)
{
    std::size_t iw = image->width();
    std::size_t ih = image->height();

    std::size_t dw, dh, dl, dt;
    if (iw > ih)
    {
        dh = THUMB_SIZE;
        dw = (std::size_t)(THUMB_SIZEF * (float)iw / (float)ih);
        dl = (dw - THUMB_SIZE) / 2;
        dt = 0;
    }
    else
    {
        dw = THUMB_SIZE;
        dh = (std::size_t)(THUMB_SIZEF * (float)ih / (float)iw);
        dl = 0;
        dt = (dh - THUMB_SIZE) / 2;
    }

    mve::ByteImage::Ptr ret = mve::image::rescale<uint8_t>
        (image, mve::image::RESCALE_LINEAR, dw, dh);
    ret = mve::image::crop<uint8_t>(ret, THUMB_SIZE, THUMB_SIZE, dl, dt, 0);

    return ret;
}

/* ---------------------------------------------------------------- */

void
read_photosynther_log (std::string const& filename,
    int bundle_id, StringVector& files)
{
    std::string from_line = "Synth " + util::string::get(bundle_id);

    /*
     * The list of original images is read from the coll.log file.
     */
    std::ifstream in(filename.c_str());
    if (!in.good())
    {
        std::cerr << "Error: Cannot read bundler log file!" << std::endl;
        std::cerr << "File: " << filename << std::endl;
        std::exit(1);
    }

    /* Find starting line. */
    std::string line;
    while (!in.eof())
    {
        std::getline(in, line);
        if (line.substr(0, from_line.size()) == from_line)
            break;
    }

    if (in.eof())
    {
        std::cerr << "Error: Parsing bundler log file failed!" << std::endl;
        std::cerr << "File: " << filename << std::endl;
        std::exit(1);
    }

    /* Find number of images. */
    util::Tokenizer tok;
    tok.split(line);
    if (tok.size() != 7)
    {
        std::cerr << "Error: Parsing bundler log file failed!" << std::endl;
        std::cerr << "File: " << filename << std::endl;
        std::exit(1);
    }
    std::size_t num = util::string::convert<std::size_t>(tok[3]);

    /* Find file names. */
    for (std::size_t i = 0; i < num && !in.eof(); ++i)
    {
        std::getline(in, line);
        util::string::chop(line);
        std::size_t pos = line.find_last_of('\\');
        std::string fname = line.substr(pos + 1, line.size() - pos - 2);
        files.push_back(fname);
    }

    in.close();
    return;
}

void
read_noah_imagelist (std::string const& filename, StringVector& files)
{
    /*
     * The list of the original images is read from the list.txt file.
     */
    std::ifstream in(filename.c_str());
    if (!in.good())
    {
        std::cout << "Error: Cannot read bundler list file!" << std::endl;
        std::cout << "File: " << filename << std::endl;
        std::exit(1);
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
load_original_image (std::string const& fname, std::string& exif)
{
    mve::ByteImage::Ptr image;
    std::string lcfname(util::string::lowercase(fname));
    std::string ext4 = util::string::right(lcfname, 4);
    std::string ext5 = util::string::right(lcfname, 5);
    if (ext4 != ".png" && ext4 != ".jpg" && ext5 != ".jpeg"
        && ext4 != ".tif" && ext5 != ".tiff")
    {
        std::cout << "Skipping file " << util::fs::get_file_component(fname)
            << ", unknown image type." << std::endl;
        return image;
    }

    try
    {
        if (ext4 == ".jpg" || ext5 == ".jpeg")
            image = mve::image::load_jpg_file(fname, &exif);
        else
            image = mve::image::load_file(fname);
    }
    catch (std::exception& e)
    {
        std::cerr << "    " << e.what() << std::endl;
        return image;
    }

    return image;
}

/* ---------------------------------------------------------------- */

void
import_bundle (AppSettings const& conf)
{
    /* Build some paths. */
    std::string bundle_fname;
    std::string imagelist_file;
    std::string image_path;
    std::string undist_path;
    mve::BundleFormat bundler_fmt = mve::BUNDELR_UNKNOWN;

    /*
     * Try to detect Photosynther software. This is deteected if the
     * file synth_N.out (for bundle N) is present in the bundler dir.
     */
    if (bundler_fmt == mve::BUNDELR_UNKNOWN)
    {
        bundle_fname = "synth_" + util::string::get(conf.bundle_id) + ".out";
        bundle_fname = conf.bundle_path + bundle_fname;
        imagelist_file = conf.input_dir + "/" PS_BUNDLE_LOG;
        image_path = conf.input_dir + "/" PS_IMAGE_DIR;
        undist_path = conf.input_dir + "/" PS_UNDIST_DIR;

        if (util::fs::file_exists(bundle_fname.c_str()))
            bundler_fmt = mve::BUNDLER_PHOTOSYNTHER;
    }

    /*
     * Try to detect Noah bundler software. Noah bundler is detected if
     * file bundle.out (for bundle 0) or bundle_%03d.out (for bundle > 0)
     * is present in the bundler directory.
     */
    if (bundler_fmt == mve::BUNDELR_UNKNOWN)
    {
        if (conf.bundle_id > 0)
            bundle_fname = "bundle_" + util::string::get_filled
                (conf.bundle_id, 3, '0') + ".out";
        else
            bundle_fname = "bundle.out";

        bundle_fname = conf.bundle_path + bundle_fname;
        imagelist_file = conf.input_dir + "/" NOAH_BUNDLE_LIST;
        image_path = conf.input_dir + "/" NOAH_IMAGE_DIR;

        if (util::fs::file_exists(bundle_fname.c_str()))
            bundler_fmt = mve::BUNDLER_NOAHBUNDLER;
    }

    /* Bundle file still not detected? */
    if (bundler_fmt == mve::BUNDELR_UNKNOWN)
    {
        std::cerr << "Error: Cannot find bundle file." << std::endl;
        std::exit(1);
    }

    /* Read bundle file. */
    mve::BundleFile bf;
    try
    { bf.read_bundle(bundle_fname); }
    catch (std::exception& e)
    {
        std::cerr << "Error: Cannot read bundle file." << std::endl
            << "  Message: " << e.what() << std::endl;
        std::exit(1);
    }

    /* Consistency check. */
    if (bf.get_format() != bundler_fmt)
    {
        std::cerr << "Error: Bundle format and directory layout do not match!"
            << std::endl;
        std::exit(1);
    }

    /*
     * Read the list of original images filenames.
     * For the Noah bundler, this is always required because it does not
     * create the undistorted images. For the Photosynther, this is only
     * required if importing the original images is requested (otherwise
     * it just takes the undistorted images).
     */
    StringVector orig_files;
    if (bundler_fmt == mve::BUNDLER_PHOTOSYNTHER && conf.import_orig)
    {
        read_photosynther_log(imagelist_file, conf.bundle_id, orig_files);
        if (orig_files.empty())
        {
            std::cerr << "Error: Empty list of original images." << std::endl;
            std::exit(1);
        }
        if (orig_files.size() != bf.get_num_valid_cameras())
        {
            std::cerr << "Error: Invalid amount of original images." << std::endl;
            std::cerr << "Cameras: " << bf.get_num_valid_cameras()
                << ", image list: " << orig_files.size() << std::endl;
            std::exit(1);
        }
        std::cout << "Recognized " << orig_files.size()
            << " original images in Photosynther log." << std::endl;
    }
    else if (bundler_fmt == mve::BUNDLER_NOAHBUNDLER)
    {
        read_noah_imagelist(imagelist_file, orig_files);
        if (orig_files.empty())
        {
            std::cerr << "Error: Empty list of original images." << std::endl;
            std::exit(1);
        }
        if (orig_files.size() != bf.get_num_cameras())
        {
            std::cerr << "Error: Invalid amount of original images." << std::endl;
            std::exit(1);
        }
        std::cout << "Recognized " << orig_files.size()
            << " original image file names in Noah bundler." << std::endl;
    }

    /* ------------------ Start importing views ------------------- */

    /* Create destination dir. */
    std::cout << "Creating output directories..." << std::endl;
    util::fs::mkdir(conf.output_dir.c_str());
    util::fs::mkdir(conf.views_path.c_str());

    /* Save bundle file. */
    std::cout << "Saving bundle file..." << std::endl;
    bf.write_bundle(conf.output_dir + "/synth_0.out");

    std::size_t valid_cnt = 0;
    std::size_t undist_imported = 0;
    mve::BundleFile::BundleCameras const& cams(bf.get_cameras());
    for (std::size_t i = 0; i < cams.size(); ++i)
    {
        /*
         * For each camera in the bundle file, a new view is created.
         * Views are populated with id, name, camera information,
         * undistorted RGB image, and eventually the original RGB image.
         */
        std::string fname = "view_" + util::string::get_filled(i, 4) + ".mve";

        std::cout << std::endl;
        std::cout << "Processing view " << fname << "..." << std::endl;

        /* Skip invalid cameras... */
        mve::CameraInfo cam(cams[i]);
        if (cam.flen == 0.0f && (conf.skip_invalid
            || bundler_fmt == mve::BUNDLER_PHOTOSYNTHER))
        {
            std::cout << "Skipping view '" << fname
                << "' (invalid camera)" << std::endl;
            continue;
        }

        /* Extract name of view from original image or sequentially. */
        std::string viewname = (conf.import_orig
            ? util::fs::get_file_component(orig_files[i])
            : util::string::get_filled(i, 4));

        /* Throw away extension if present. */
        std::string name_ext = util::string::lowercase
            (util::string::right(viewname, 4));
        if (name_ext == ".jpg" || name_ext == ".png")
            viewname = util::string::left(viewname, viewname.size() - 4);

        /* Convert from Photosynther camera conventions. */
        if (bundler_fmt == mve::BUNDLER_PHOTOSYNTHER)
        {
            /* Nothing to do here. */
        }

        /* Fix issues with Noah Bundler camera specification. */
        if (bundler_fmt == mve::BUNDLER_NOAHBUNDLER)
        {
            /* Check focal length of camera, fix negative focal length. */
            if (cam.flen < 0.0f)
            {
                std::cout << "Fixing focal length for view '"
                    << fname << "'" << std::endl;
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
                std::cout << "Skipping view '" << fname
                    << "' (bad rotation matrix)" << std::endl;
                continue;
            }
        }

        /* Create view and set headers. */
        mve::View::Ptr view = mve::View::create();
        view->set_id(i);
        view->set_name(viewname);
        view->set_camera(cam);

        /* FIXME: Handle exceptions, i.e. while loading images? */

        /* Load undistorted and original image, create thumbnail. */
        mve::ByteImage::Ptr original, undist, thumb;
        std::string exif;
        if (bundler_fmt == mve::BUNDLER_NOAHBUNDLER)
        {
            /* For Noah datasets, load original image and undistort it. */
            std::string orig_filename = image_path + orig_files[i];
            original = load_original_image(orig_filename, exif);
            thumb = get_thumbnail(original);

            /* Convert Noah focal length to MVE focal length. */
            cam.flen /= (float)std::max(original->width(), original->height());
            view->set_camera(cam);

            if (cam.flen != 0.0f)
                undist = mve::image::image_undistort_noah<uint8_t>(original, cam);

            if (!conf.import_orig)
                original.reset();
        }
        else if (bundler_fmt == mve::BUNDLER_PHOTOSYNTHER)
        {
            /* With the Photosynther, load undistorted and original. */
            std::string undist_filename = undist_path + "undistorted_"
                + util::string::get_filled(conf.bundle_id, 4) + "_"
                + util::string::get_filled(valid_cnt, 4) + ".jpg";
            undist = mve::image::load_file(undist_filename);
            thumb = get_thumbnail(undist);

            if (conf.import_orig)
            {
                std::string orig_filename(image_path + orig_files[valid_cnt]);
                original = load_original_image(orig_filename, exif);
                /* Overwrite undistorted images with manually undistorted
                 * original images. This reduces JPEG artifacts. */
                undist = mve::image::image_undistort<uint8_t>(original, cam);
            }
        }

        /* Add images to view. */
        if (thumb.get())
            view->add_image("thumbnail", thumb);

        if (undist.get())
            view->add_image("undistorted", undist);
        else if (cam.flen != 0.0f && !undist.get())
            std::cout << "Warning: Undistorted image missing!" << std::endl;

        if (original.get())
            view->add_image("original", original);
        else if (conf.import_orig && undist.get())
            std::cout << "Warning: Original image missing!" << std::endl;

        /* Add EXIF data to view if available. */
        if (!exif.empty())
        {
            mve::ByteImage::Ptr exif_image = mve::ByteImage::create(exif.size(), 1, 1);
            std::copy(exif.begin(), exif.end(), exif_image->begin());
            view->add_data("exif", exif_image);
        }

        /* Save MVE file. */
        {
            std::string view_filename = conf.views_path + fname;
            view->save_mve_file_as(view_filename);
        }

        if (cam.flen != 0)
            valid_cnt += 1;
        if (undist.get())
            undist_imported += 1;
    }

    std::cout << std::endl;
    std::cout << "Created " << cams.size() << " views with "
        << valid_cnt << " valid cameras." << std::endl
        << "Imported " << undist_imported << " undistorted images."
        << std::endl;
}

/* ---------------------------------------------------------------- */

void
import_images (AppSettings const& conf)
{
    util::fs::Directory dir;
    try { dir.scan(conf.input_dir); }
    catch (std::exception& e)
    {
        std::cerr << "Error scanning input dir: " << e.what() << std::endl;
        std::exit(1);
    }
    std::cout << "Found " << dir.size() << " directory entries." << std::endl;

    /* ------------------ Start importing images ------------------- */

    /* Create destination dir. */
    std::cout << "Creating output directories..." << std::endl;
    util::fs::mkdir(conf.output_dir.c_str());
    util::fs::mkdir(conf.views_path.c_str());

    /* Sort file names, iterate over file names. */
    std::sort(dir.begin(), dir.end());
    std::size_t id_cnt = 0;
    for (std::size_t i = 0; i < dir.size(); ++i)
    {
        if (dir[i].is_dir)
        {
            std::cout << "Skipping directory " << dir[i].name << std::endl;
            continue;
        }

        std::string fname(dir[i].name);
        std::cout << "Importing image " << fname << "..." << std::endl;

        std::string exif;
        mve::ByteImage::Ptr image = load_original_image
            (dir[i].get_absolute_name(), exif);
        if (!image.get())
            continue;

        /* Generate view name. */
        std::string viewname = fname;
        {
            std::size_t pos = fname.find_last_of('.');
            if (pos != std::string::npos)
                viewname = fname.substr(0, pos);
        }

        /* Create view and set headers. */
        mve::View::Ptr view = mve::View::create();
        view->set_id(id_cnt);
        view->set_name(viewname);

        /* Add thumbnail and image to view. */
        mve::ByteImage::Ptr thumb = get_thumbnail(image);
        view->add_image("thumbnail", thumb);
        view->add_image("original", image);

        /* Add EXIF data to view if available. */
        if (!exif.empty())
        {
            mve::ByteImage::Ptr exif_image = mve::ByteImage::create(exif.size(), 1, 1);
            std::copy(exif.begin(), exif.end(), exif_image->begin());
            view->add_data("exif", exif_image);
        }

        /* Save view to disc. */
        fname = "view_" + util::string::get_filled(id_cnt, 4) + ".mve";
        view->save_mve_file_as(conf.views_path + fname);

        /* Advance ID of successfully imported images. */
        id_cnt += 1;
    }

    std::cout << "Imported " << id_cnt << " input images." << std::endl;
}

/* ---------------------------------------------------------------- */

int
main (int argc, char** argv)
{
    /* Setup argument parser. */
    util::Arguments args;
    args.add_option('o', "original", false, "Import original images");
    args.add_option('b', "bundle-id", true, "ID of the bundle [0]");
    args.add_option('k', "keep-invalid", false, "Keeps images with invalid cameras");
    args.add_option('i', "images-only", false, "Imports images from INPUT_DIR only");
    args.set_description("This utility creates MVE scenes by importing "
        "from a Photosynther or Noah format bundle directory INPUT_DIR. "
        "Images corresponding to invalid cameras are ignored by default. "

        "For Photosynther, makescene expects an \"undistorted\" directory "
        "with the bundled images, and an \"images\" directory with the "
        "original images, if requested. This also requires coll.log. "
        "For Photosynther, it is not possible to keep invalid views. "

        "For Noah bundles, makescene expects \"list.txt\" in INPUT_DIR "
        "and the bundle file in the \"bundle\" directory. "

        "With the \"images-only\" option, all images in INPUT_DIR "
        "are directly imported without camera information.");
    args.set_exit_on_error(true);
    args.set_nonopt_maxnum(2);
    args.set_nonopt_minnum(2);
    args.set_helptext_indent(22);
    args.set_usage(argv[0], "[ OPTIONS ] INPUT_DIR OUT_SCENE");
    args.parse(argc, argv);

    /* Setup defaults. */
    AppSettings conf;
    conf.import_orig = false;
    conf.skip_invalid = true;
    conf.images_only = false;
    conf.input_dir = args.get_nth_nonopt(0);
    conf.output_dir = args.get_nth_nonopt(1);
    conf.bundle_id = 0;

    /* General settings. */
    for (util::ArgResult const* i = args.next_option();
        i != 0; i = args.next_option())
    {
        switch (i->opt->sopt)
        {
            case 'o': conf.import_orig = true; break;
            case 'b': conf.bundle_id = i->get_arg<int>(); break;
            case 'k': conf.skip_invalid = false; break;
            case 'i': conf.images_only = true; break;
            default: throw std::invalid_argument("Unexpected option");
        }
    }

    /* Check command line arguments. */
    if (conf.input_dir.empty() || conf.output_dir.empty())
    {
        args.generate_helptext(std::cerr);
        return 1;
    }

    /* Build some paths. */
    conf.views_path = conf.output_dir + "/" VIEWS_DIR;
    conf.bundle_path = conf.input_dir + "/" BUNDLE_PATH;

    /* Check if output dir exists. */
    if (util::fs::dir_exists(conf.output_dir.c_str()))
    {
        std::cout << std::endl;
        std::cout << "** WARNING: Output dir already exists." << std::endl;
        std::cout << "** This may leave old views in your scene." << std::endl;
        std::cout << std::endl;
        std::cout << "Press ENTER to continue, or CTRL-C to exit." << std::endl;
        std::string line;
        std::getline(std::cin, line);
    }

    if (conf.images_only)
        import_images(conf);
    else
        import_bundle(conf);

    return 0;
}
