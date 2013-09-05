/*
 * App to create MVE scenes from images and bundles.
 * Written by Simon Fuhrmann.
 *
 * The app supports:
 * - Import of calibrated images from Photosynther and Noah bundler
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
#include "mve/trianglemesh.h"
#include "mve/meshtools.h"

#define THUMBNAIL_SIZE 50

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
    bool append_images;

    /* Computed values. */
    std::string bundle_path;
    std::string views_path;
};

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
        util::string::clip_newlines(&line);
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
        std::cerr << "Error: Cannot read bundler list file!" << std::endl;
        std::cerr << "File: " << filename << std::endl;
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
    { }

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
    if (img_8 != NULL)
        return img_8;

    mve::RawImage::Ptr img_16 = load_16bit_image(fname);
    if (img_16 != NULL)
        return img_16;

    mve::FloatImage::Ptr img_float = load_float_image(fname);
    if (img_float != NULL)
        return img_float;

    std::cerr << "Skipping file " << util::fs::get_file_component(fname)
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
    view->add_data("exif", exif_image);
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
                (img, THUMBNAIL_SIZE, THUMBNAIL_SIZE);
            break;

        case mve::IMAGE_TYPE_UINT16:
        {
            mve::RawImage::Ptr temp = mve::image::create_thumbnail<uint16_t>
                (img, THUMBNAIL_SIZE, THUMBNAIL_SIZE);
            uint16_t vmin, vmax;
            find_min_max_percentile(temp, &vmin, &vmax);
            image = mve::image::raw_to_byte_image(temp, vmin, vmax);
            break;
        }

        case mve::IMAGE_TYPE_FLOAT:
        {
            mve::FloatImage::Ptr temp = mve::image::create_thumbnail<float>
                (img, THUMBNAIL_SIZE, THUMBNAIL_SIZE);
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

/*
 * The VisualSFM tool saves bundles in NVM format. Docs are here:
 * http://homes.cs.washington.edu/~ccwu/vsfm/doc.html#nvm
 */

struct NVMView
{
    std::string filename;
    double focal_length;
    double quaternion[4];
    double camera_center[3];
    double radial_distortion;
};

struct NVMRef
{
    int image_id;
    int feature_id;
    float pos[2];
};

struct NVMPoint
{
    double pos[3];
    unsigned char color[3];
    std::vector<NVMRef> refs;
};

math::Matrix3f
get_rot_from_quaternion(double const* values)
{
    math::Vec4f q(values);
    q.normalize();

    math::Matrix3f rot;
    rot[0] = 1.0f - 2.0f * q[2]*q[2] - 2.0f * q[3]*q[3];
    rot[1] = 2.0f * q[1]*q[2] - 2.0f * q[3]*q[0];
    rot[2] = 2.0f * q[1]*q[3] + 2.0f * q[2]*q[0];

    rot[3] = 2.0f * q[1]*q[2] + 2.0f * q[3]*q[0];
    rot[4] = 1.0f - 2.0f * q[1]*q[1] - 2.0f * q[3]*q[3];
    rot[5] = 2.0f * q[2]*q[3] - 2.0f * q[1]*q[0];

    rot[6] = 2.0f * q[1]*q[3] - 2.0f * q[2]*q[0];
    rot[7] = 2.0f * q[2]*q[3] + 2.0f * q[1]*q[0];
    rot[8] = 1.0f - 2.0f * q[1]*q[1] - 2.0f * q[2]*q[2];
    return rot;
}

std::string
get_relative_path_component (std::string const& full_path)
{
    std::size_t pos = full_path.find_last_of('/');
    if (pos == std::string::npos)
        return "./";
    else if (pos == 0)
        return "/.";
    else
        return full_path.substr(0, pos);
}

void
import_bundle_nvm (AppSettings const& conf)
{
    std::ifstream in(conf.input_dir.c_str());
    if (!in.good())
    {
        std::cerr << "Error: Cannot open NVM file" << std::endl;
        return;
    }

    /* Check NVM file signature. */
    std::cout << "Start parsing NVM file..." << std::endl;
    std::string signature;
    in >> signature;
    if (signature != "NVM_V3")
    {
        std::cerr << "Error: Invalid NVM signature" << std::endl;
        in.close();
        return;
    }

    // TODO: Handle multiple models.

    /* Read number of views. */
    int num_views = 0;
    in >> num_views;
    if (num_views < 0 || num_views > 10000)
    {
        std::cerr << "Error: Invalid number of views: "
            << num_views << std::endl;
        in.close();
        return;
    }
    std::cout << "Number of views: " << num_views << std::endl;

    /* Read views. */
    std::string nvm_path = get_relative_path_component(conf.input_dir);
    std::vector<NVMView> views;
    for (int i = 0; i < num_views; ++i)
    {
        NVMView view;
        in >> view.filename;
        in >> view.focal_length;
        for (int j = 0; j < 4; ++j)
            in >> view.quaternion[j];
        for (int j = 0; j < 3; ++j)
            in >> view.camera_center[j];
        in >> view.radial_distortion;
        if (in.eof())
        {
            std::cerr << "Error: Premature end of NVM file." << std::endl;
            in.close();
            return;
        }

        /* If the filename is not absolute, make relative to NVM. */
        if (view.filename[0] != '/')
            view.filename = nvm_path + "/" + view.filename;

        views.push_back(view);

        int temp;
        in >> temp;
    }

    /* Read number of points. */
    int num_points = 0;
    in >> num_points;
    if (num_points < 0 || num_points > 1000000000)
    {
        std::cerr << "Error: Invalid number of SfM points: "
            << num_points << std::endl;
        in.close();
        return;
    }
    std::cout << "Number of points: " << num_points << std::endl;

    /* Read points. */
    std::vector<NVMPoint> points;
    for (int i = 0; i < num_points; ++i)
    {
        NVMPoint point;

        /* Read position and color. */
        for (int j = 0; j < 3; ++j)
            in >> point.pos[j];
        int color_tmp[3];
        for (int j = 0; j < 3; ++j)
            in >> color_tmp[j];
        std::copy(color_tmp, color_tmp + 3, point.color);

        /* Read number of refs. */
        int num_refs = 0;
        in >> num_refs;
        if (num_refs < 2 || num_refs > 1000)
        {
            std::cerr << "Error: Invalid number of feature refs: "
                << num_refs << std::endl;
            in.close();
            return;
        }

        /* Read refs. */
        for (int j = 0; j < num_refs; ++j)
        {
            NVMRef ref;
            in >> ref.image_id >> ref.feature_id >> ref.pos[0] >> ref.pos[1];
            point.refs.push_back(ref);
        }
        points.push_back(point);
    }
    in.close();

    /* Create output directories. */
    std::cout << "Creating output directories..." << std::endl;
    util::fs::mkdir(conf.output_dir.c_str());
    util::fs::mkdir(conf.views_path.c_str());

    /* Write output bundle file for MVE. */
    std::cout << "Writing bundle file..." << std::endl;
    std::ofstream out((conf.output_dir + "/synth_0.out").c_str());
    if (!out.good())
    {
        std::cerr << "Error: Cannot open output bundle file" << std::endl;
        in.close();
        return;
    }
    out << "drews 1.0" << std::endl;
    out << num_views << " " << num_points << std::endl;
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        NVMView const& view = views[i];
        math::Matrix3f rot = get_rot_from_quaternion(view.quaternion);
        math::Vec3f trans = rot * -math::Vec3f(view.camera_center);

        out << view.focal_length << " " << views[i].radial_distortion
            << " 0" << std::endl;
        out << rot[0] << " " << rot[1] << " " << rot[2] << std::endl;
        out << rot[3] << " " << rot[4] << " " << rot[5] << std::endl;
        out << rot[6] << " " << rot[7] << " " << rot[8] << std::endl;
        out << trans[0] << " " << trans[1] << " " << trans[2] << std::endl;
    }
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        NVMPoint const& point = points[i];
        out << point.pos[0] << " " << point.pos[1] << " " << point.pos[2] << std::endl;
        out << static_cast<int>(point.color[0]) << " "
            << static_cast<int>(point.color[1]) << " "
            << static_cast<int>(point.color[2]) << std::endl;
        out << point.refs.size();
        for (std::size_t j = 0; j < point.refs.size(); ++j)
            out << " " << point.refs[j].image_id << " "
                << point.refs[j].feature_id << " 0";
        out << std::endl;
    }
    out.close();

    /* Create and write views. */
    std::cout << "Writing MVE files..." << std::endl;
#pragma omp parallel for schedule(dynamic, 1)
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        mve::View::Ptr view = mve::View::create();
        view->set_id(i);
        view->set_name(util::string::get_filled(i, 4, '0'));

        mve::ByteImage::Ptr image;
        std::string exif;
        try
        { image = load_8bit_image(views[i].filename, &exif); }
        catch (...)
        {
            std::cout << "Error loading: " << views[i].filename << std::endl;
            continue;
        }

        int const maxdim = std::max(image->width(), image->height());
        view->add_image("original", image);
        view->add_image("thumbnail", create_thumbnail(image));
        add_exif_to_view(view, exif);

        mve::CameraInfo cam;
        cam.flen = views[i].focal_length / static_cast<float>(maxdim);
        cam.dist[0] = views[i].radial_distortion;
        cam.dist[1] = 0.0f;
        math::Matrix3f rot = get_rot_from_quaternion(views[i].quaternion);
        math::Vec3f trans = rot * -math::Vec3f(views[i].camera_center);
        std::copy(*rot, *rot + 9, cam.rot);
        std::copy(*trans, *trans + 3, cam.trans);

        mve::ByteImage::Ptr undist = mve::image::image_undistort_vsfm<uint8_t>
            (image, cam.flen, views[i].radial_distortion);
        view->add_image("undistorted", undist);

        view->set_camera(cam);
        std::string fname = "view_" + util::string::get_filled(i, 4) + ".mve";
        std::cout << "Writing MVE file: " << fname << "..." << std::endl;
        view->save_mve_file_as(conf.views_path + fname);
    }
    std::cout << std::endl << "Done importing NVM file!" << std::endl;
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

    /**
     * Try to detect VisualSFM bundle format. This is detected if the input
     * is a file with extension "nmv".
     */
    if (bundler_fmt == mve::BUNDELR_UNKNOWN)
    {
        if (util::string::right(conf.input_dir, 4) == ".nvm"
            && util::fs::file_exists(conf.input_dir.c_str()))
        {
            import_bundle_nvm(conf);
            return;
        }
    }

    /*
     * Try to detect Photosynther software. This is detected if the
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
        std::cerr << "Error: Bundle format and directory layout don't match!"
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
            std::cerr << "Skipping view '" << fname
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
                std::cerr << "Skipping view '" << fname
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
            original = load_8bit_image(orig_filename, &exif);
            thumb = create_thumbnail(original);

            /* Convert Noah focal length to MVE focal length. */
            cam.flen /= (float)std::max(original->width(), original->height());
            view->set_camera(cam);

            if (cam.flen != 0.0f)
                undist = mve::image::image_undistort_bundler<uint8_t>
                    (original, cam.flen, cam.dist[0], cam.dist[1]);

            if (!conf.import_orig)
                original.reset();
        }
        else if (bundler_fmt == mve::BUNDLER_PHOTOSYNTHER)
        {
            /*
             * With the Photosynther, load undistorted and original.
             * The new version uses "forStereo_xxxx_yyyy.png" as file
             * name and the older version uses "undistorted_xxxx_yyyy.jpg".
             */
            std::string undist_new_filename = undist_path + "forStereo_"
                + util::string::get_filled(conf.bundle_id, 4) + "_"
                + util::string::get_filled(valid_cnt, 4) + ".png";
            std::string undist_old_filename = undist_path + "undistorted_"
                + util::string::get_filled(conf.bundle_id, 4) + "_"
                + util::string::get_filled(valid_cnt, 4) + ".jpg";
            /* Try the newer file name and fall back if not existing. */
            if (util::fs::file_exists(undist_new_filename.c_str()))
                undist = mve::image::load_file(undist_new_filename);
            else
                undist = mve::image::load_file(undist_old_filename);

            if (conf.import_orig)
            {
                std::string orig_filename(image_path + orig_files[valid_cnt]);
                original = load_8bit_image(orig_filename, &exif);
                /* Overwrite undistorted images with manually undistorted
                 * original images. This reduces JPEG artifacts. */
                undist = mve::image::image_undistort_msps<uint8_t>
                    (original, cam.dist[0], cam.dist[1]);
            }
            thumb = create_thumbnail(undist);
        }

        /* Add images to view. */
        if (thumb != NULL)
            view->add_image("thumbnail", thumb);

        if (undist != NULL)
            view->add_image("undistorted", undist);
        else if (cam.flen != 0.0f && undist == NULL)
            std::cerr << "Warning: Undistorted image missing!" << std::endl;

        if (original != NULL)
            view->add_image("original", original);
        else if (conf.import_orig && original == NULL)
            std::cerr << "Warning: Original image missing!" << std::endl;

        /* Add EXIF data to view if available. */
        add_exif_to_view(view, exif);

        /* Save MVE file. */
        std::cout << "Writing MVE file: " << fname << "..." << std::endl;
        view->save_mve_file_as(conf.views_path + fname);

        if (cam.flen != 0)
            valid_cnt += 1;
        if (undist != NULL)
            undist_imported += 1;
    }

    std::cout << std::endl;
    std::cout << "Created " << cams.size() << " views with "
        << valid_cnt << " valid cameras." << std::endl
        << "Imported " << undist_imported << " undistorted images."
        << std::endl;
}

/* ---------------------------------------------------------------- */

int
find_max_scene_id (std::string const& view_path)
{
    util::fs::Directory dir;
    try { dir.scan(view_path); }
    catch (...) { return -1; }

    /* Load all MVE files and remember largest view ID. */
    std::size_t max_view_id = 0;
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

    return static_cast<int>(max_view_id);
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
    if (!conf.append_images)
    {
        std::cout << "Creating output directories..." << std::endl;
        util::fs::mkdir(conf.output_dir.c_str());
        util::fs::mkdir(conf.views_path.c_str());
    }

    int max_scene_id = -1;
    if (conf.append_images)
    {
        max_scene_id = find_max_scene_id(conf.views_path);
        if (max_scene_id < 0)
        {
            std::cerr << "Error: Cannot find view ID for appending." << std::endl;
            std::exit(1);
        }
    }

    /* Sort file names, iterate over file names. */
    std::sort(dir.begin(), dir.end());
    std::size_t id_cnt = static_cast<std::size_t>(max_scene_id + 1);
    std::size_t num_imported = 0;
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
        mve::ByteImage::Ptr image = load_any_image
            (dir[i].get_absolute_name(), &exif);
        if (image == NULL)
            continue;

        /* Generate view name. */
        std::string viewname = fname;
        {
            std::size_t pos = fname.find_last_of('.');
            if (pos != std::string::npos)
                viewname = fname.substr(0, pos);
        }

        /* Create view, set headers, add image. */
        mve::View::Ptr view = mve::View::create();
        view->set_id(id_cnt);
        view->set_name(viewname);
        view->add_image("original", image);

        /* Add thumbnail for byte images. */
        mve::ByteImage::Ptr thumb = create_thumbnail(image);
        if (thumb != NULL)
            view->add_image("thumbnail", thumb);

        /* Add EXIF data to view if available. */
        add_exif_to_view(view, exif);

        /* Save view to disc. */
        fname = "view_" + util::string::get_filled(id_cnt, 4) + ".mve";
        std::cout << "Writing MVE file: " << fname << "..." << std::endl;
        view->save_mve_file_as(conf.views_path + fname);

        /* Advance ID of successfully imported images. */
        id_cnt += 1;
        num_imported += 1;
    }

    std::cout << "Imported " << num_imported << " input images." << std::endl;
}

/* ---------------------------------------------------------------- */

int
main (int argc, char** argv)
{
    /* Setup argument parser. */
    util::Arguments args;
    args.set_usage(argv[0], "[ OPTIONS ] INPUT OUT_SCENE");
    args.set_exit_on_error(true);
    args.set_nonopt_maxnum(2);
    args.set_nonopt_minnum(2);
    args.set_helptext_indent(22);
    args.set_description("This utility creates MVE scenes by importing "
        "from a Photosynther bundle directory, a Noah bundle directory "
        "or a VisualSfM .nvm file.\n\n"

        "For Photosynther, makescene expects the bundle directory as INPUT, "
        "an \"undistorted\" directory in INPUT with the bundled images, and "
        "an \"images\" directory with the original images, if requested. "
        "This also requires coll.log. For Photosynther, it is not possible "
        "to keep invalid views.\n\n"

        "For Noah bundles, makescene expects the bundle directory as INPUT, "
        "a file \"list.txt\" in INPUT and the bundle file in the "
        "\"bundle\" directory.\n\n"

        "For VisualSfM, makescene expects the .nvm file as INPUT.\n\n"

        "With the \"images-only\" option, all images in INPUT_DIR "
        "are directly imported without camera information. If "
        "\"append-images\" is specified, images are added to the scene.");
    args.add_option('o', "original", false, "Import original images");
    args.add_option('b', "bundle-id", true, "ID of the bundle [0]");
    args.add_option('k', "keep-invalid", false, "Keeps images with invalid cameras");
    args.add_option('i', "images-only", false, "Imports images from INPUT_DIR only");
    args.add_option('a', "append-images", false, "Appends images to an existing scene");
    args.parse(argc, argv);

    /* Setup defaults. */
    AppSettings conf;
    conf.import_orig = false;
    conf.skip_invalid = true;
    conf.images_only = false;
    conf.append_images = false;
    conf.input_dir = args.get_nth_nonopt(0);
    conf.output_dir = args.get_nth_nonopt(1);
    conf.bundle_id = 0;

    /* General settings. */
    for (util::ArgResult const* i = args.next_option();
        i != NULL; i = args.next_option())
    {
        switch (i->opt->sopt)
        {
            case 'o': conf.import_orig = true; break;
            case 'b': conf.bundle_id = i->get_arg<int>(); break;
            case 'k': conf.skip_invalid = false; break;
            case 'i': conf.images_only = true; break;
            case 'a': conf.append_images = true; break;
            default: throw std::invalid_argument("Unexpected option");
        }
    }

    /* Check command line arguments. */
    if (conf.input_dir.empty() || conf.output_dir.empty())
    {
        args.generate_helptext(std::cerr);
        return 1;
    }

    if (conf.append_images && !conf.images_only)
    {
        std::cerr << "Error: Cannot --append-images without --images-only."
            << std::endl;
        return 1;
    }

    /* Build some paths. */
    conf.views_path = conf.output_dir + "/" VIEWS_DIR;
    conf.bundle_path = conf.input_dir + "/" BUNDLE_PATH;

    /* Check if output dir exists. */
    bool output_dir_exists = util::fs::dir_exists(conf.output_dir.c_str());
    if (output_dir_exists && !conf.append_images)
    {
        std::cerr << std::endl;
        std::cerr << "** WARNING: Output dir already exists." << std::endl;
        std::cerr << "** This may leave old views in your scene." << std::endl;
        std::cerr << std::endl;
        std::cerr << "Press ENTER to continue, or CTRL-C to exit." << std::endl;
        std::string line;
        std::getline(std::cin, line);
    }
    else if (!output_dir_exists && conf.append_images)
    {
        std::cerr << "Error: Output dir does not exist. Cannot append images."
            << std::endl;
        return 1;
    }

    if (conf.images_only)
        import_images(conf);
    else
        import_bundle(conf);

    return 0;
}
