/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <cassert>

#include "util/system.h"
#include "util/file_system.h"
#include "util/strings.h"
#include "util/exception.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "mve/bundle_io.h"
#include "mve/image_tools.h"
#include "mve/depthmap.h"


MVE_NAMESPACE_BEGIN

/* ------------------- MVE native bundle format ------------------- */

Bundle::Ptr
load_mve_bundle (std::string const& filename)
{
    return load_photosynther_bundle(filename);
}

void
save_mve_bundle (Bundle::ConstPtr bundle, std::string const& filename)
{
    save_photosynther_bundle(bundle, filename);
}

/* -------------- Support for NVM files (VisualSFM) --------------- */

namespace
{
    /* Conversion from quaternion to rotation matrix. */
    math::Matrix3f
    get_rot_from_quaternion(double const* values)
    {
        math::Vec4f q(values);
        q.normalize();

        math::Matrix3f rot;
        rot[0] = 1.0f - 2.0f * q[2] * q[2] - 2.0f * q[3] * q[3];
        rot[1] = 2.0f * q[1] * q[2] - 2.0f * q[3] * q[0];
        rot[2] = 2.0f * q[1] * q[3] + 2.0f * q[2] * q[0];

        rot[3] = 2.0f * q[1] * q[2] + 2.0f * q[3] * q[0];
        rot[4] = 1.0f - 2.0f * q[1] * q[1] - 2.0f * q[3] * q[3];
        rot[5] = 2.0f * q[2] * q[3] - 2.0f * q[1] * q[0];

        rot[6] = 2.0f * q[1] * q[3] - 2.0f * q[2] * q[0];
        rot[7] = 2.0f * q[2] * q[3] + 2.0f * q[1] * q[0];
        rot[8] = 1.0f - 2.0f * q[1] * q[1] - 2.0f * q[2] * q[2];
        return rot;
    }
}  // namespace

Bundle::Ptr
load_nvm_bundle (std::string const& filename,
    std::vector<AdditionalCameraInfo>* camera_info)
{
    std::ifstream in(filename.c_str());
    if (!in.good())
        throw util::FileException(filename, std::strerror(errno));

    /* Check NVM file signature. */
    std::cout << "NVM: Loading file..." << std::endl;
    std::string signature;
    in >> signature;
    if (signature != "NVM_V3")
        throw util::Exception("Invalid NVM signature");

    /* Discard the rest of the line (e.g. fixed camera parameter info). */
    {
        std::string temp;
        std::getline(in, temp);
    }

    // TODO: Handle multiple models.

    /* Read number of views. */
    int num_views = 0;
    in >> num_views;
    if (num_views < 0 || num_views > 10000)
        throw util::Exception("Invalid number of views: ",
            util::string::get(num_views));

    /* Create new bundle and prepare NVM specific output. */
    Bundle::Ptr bundle = Bundle::create();
    Bundle::Cameras& bundle_cams = bundle->get_cameras();
    bundle_cams.reserve(num_views);
    std::vector<AdditionalCameraInfo> nvm_cams;
    nvm_cams.reserve(num_views);

    /* Read views. */
    std::cout << "NVM: Number of views: " << num_views << std::endl;
    std::string nvm_path = util::fs::dirname(filename);
    for (int i = 0; i < num_views; ++i)
    {
        AdditionalCameraInfo nvm_cam;
        CameraInfo bundle_cam;

        /* Filename and focal length. */
        in >> nvm_cam.filename;
        in >> bundle_cam.flen;

        /* Camera rotation and center. */
        double quat[4];
        for (int j = 0; j < 4; ++j)
            in >> quat[j];
        math::Matrix3f rot = get_rot_from_quaternion(quat);
        math::Vec3f center, trans;
        for (int j = 0; j < 3; ++j)
            in >> center[j];
        trans = rot * -center;
        std::copy(rot.begin(), rot.end(), bundle_cam.rot);
        std::copy(trans.begin(), trans.end(), bundle_cam.trans);

        /* Radial distortion. */
        in >> nvm_cam.radial_distortion;
        bundle_cam.dist[0] = nvm_cam.radial_distortion;
        bundle_cam.dist[1] = 0.0f;

        /* If the filename is not absolute, make relative to NVM. */
        if (!util::fs::is_absolute(nvm_cam.filename))
            nvm_cam.filename = util::fs::join_path(nvm_path, nvm_cam.filename);

        /* Jettison trailing zero. */
        int temp;
        in >> temp;

        if (in.eof())
            throw util::Exception("Unexpected EOF in NVM file");

        bundle_cams.push_back(bundle_cam);
        nvm_cams.push_back(nvm_cam);
    }

    /* Read number of features. */
    int num_features = 0;
    in >> num_features;
    if (num_features < 0 || num_features > 1000000000)
        throw util::Exception("Invalid number of features: ",
            util::string::get(num_features));

    Bundle::Features& features = bundle->get_features();
    features.reserve(num_features);

    /* Read points. */
    std::cout << "NVM: Number of features: " << num_features << std::endl;
    std::size_t num_strange_points = 0;
    for (int i = 0; i < num_features; ++i)
    {
        Bundle::Feature3D feature;
        for (int j = 0; j < 3; ++j)
            in >> feature.pos[j];
        for (int j = 0; j < 3; ++j)
        {
            in >> feature.color[j];
            feature.color[j] /= 255.0f;
        }

        /* Read number of refs. */
        int num_refs = 0;
        in >> num_refs;

        /* Detect strange points not seen by cameras. Why does this happen? */
        if (num_refs == 0)
        {
            num_strange_points += 1;
            continue;
        }

        /* There should be at least 2 cameras that see the point. */
        if (num_refs < 2 || num_refs > num_views)
            throw util::Exception("Invalid number of feature refs: ",
                util::string::get(num_refs));

        /* Read refs. */
        feature.refs.reserve(num_refs);
        for (int j = 0; j < num_refs; ++j)
        {
            Bundle::Feature2D ref;
            in >> ref.view_id >> ref.feature_id >> ref.pos[0] >> ref.pos[1];
            feature.refs.push_back(ref);
        }
        features.push_back(feature);
    }
    in.close();

    /* Warn about strange points. */
    if (num_strange_points > 0)
    {
        std::cout << "NVM: " << num_strange_points
            << " strange points not seem by any camera!" << std::endl;
    }

    if (camera_info != nullptr)
        std::swap(*camera_info, nvm_cams);

    return bundle;
}

/* ----------- Common code for Bundler and Photosynther ----------- */

/*
 * Both Bundler and Photosynther formats are quite similar so one parser
 * can do all the work with minor differences given the format. The format
 * could easily be detected automatically but this is avoided here.
 *
 * Photosynther bundle file format
 * -------------------------------
 *
 * "drews 1.0"
 * <num_cameras> <num_features>
 * <cam 1 line 1> // Focal length, Radial distortion: f rd1 rd2
 * <cam 1 line 2> // Rotation matrix row 1: r11 r12 r13
 * <cam 1 line 3> // Rotation matrix row 2: r21 r22 r23
 * <cam 1 line 4> // Rotation matrix row 3: r31 r32 r33
 * <cam 1 line 5> // Translation vector: t1 t2 t3
 * ...
 * <point 1 position> // x y z (floats)
 * <point 1 color> // r g b (uchars)
 * <point 1 visibility> // <list length (uint)> ( <img id (uint)> <sift id (uint)> <reproj. quality (float)> ) ...
 * ...
 *
 * Noah Snavely bundle file format
 * -------------------------------
 *
 * "# Bundle file v0.3"
 * <num_cameras> <num_features>
 * <cam 1 line 1> // Focal length, Radial distortion: f k1 k2
 * <cam 1 line 2> // Rotation matrix row 1: r11 r12 r13
 * <cam 1 line 3> // Rotation matrix row 2: r21 r22 r23
 * <cam 1 line 4> // Rotation matrix row 3: r31 r32 r33
 * <cam 1 line 5> // Translation vector: t1 t2 t3
 * ...
 * <point 1 position> // x y z (floats)
 * <point 1 color> // r g b (uchars)
 * <point 1 visibility> // <list length (uint)> ( <img ID (uint)> <sift ID (uint)> <x (float)> <y (float)> ) ...
 * ...
 *
 * A few notes on the bundler format: Each camera in the bundle file
 * corresponds to the ordered list of input images. Some cameras are set
 * to zero, which means the input image was not registered. <cam ID> is
 * the ID w.r.t. the input images, <sift ID> is the ID of the SIFT feature
 * point for that image. In the Noah bundler, <x> and <y> are floating point
 * positions of the keypoint in the in the image-centered coordinate system.
 */

namespace
{
    enum BundleFormat
    {
        BUNDLE_FORMAT_PHOTOSYNTHER,
        BUNDLE_FORMAT_NOAHBUNDLER,
        BUNDLE_FORMAT_ERROR
    };
}  // namespace

Bundle::Ptr
load_bundler_ps_intern (std::string const& filename, BundleFormat format)
{
    std::ifstream in(filename.c_str());
    if (!in.good())
        throw util::FileException(filename, std::strerror(errno));

    /* Read version information in the first line. */
    std::string version_string;
    std::getline(in, version_string);
    util::string::clip_newlines(&version_string);
    util::string::clip_whitespaces(&version_string);

    std::string parser_string;
    if (format == BUNDLE_FORMAT_PHOTOSYNTHER)
    {
        parser_string = "Photosynther";
        if (version_string != "drews 1.0")
            format = BUNDLE_FORMAT_ERROR;
    }
    else if (format == BUNDLE_FORMAT_NOAHBUNDLER)
    {
        parser_string = "Bundler";
        if (version_string != "# Bundle file v0.3")
            format = BUNDLE_FORMAT_ERROR;
    }
    else
        throw util::Exception("Invalid parser format");

    if (format == BUNDLE_FORMAT_ERROR)
        throw util::Exception("Invalid file signature: ", version_string);

    /* Read number of cameras and number of points. */
    int num_views = 0;
    int num_features = 0;
    in >> num_views >> num_features;

    if (in.eof())
        throw util::Exception("Unexpected EOF in bundle file");

    if (num_views < 0 || num_views > 10000
        || num_features < 0 || num_features > 100000000)
        throw util::Exception("Spurious amount of cameras or features");

    /* Print message according to detected parser format. */
    std::cout << "Reading " << parser_string << " file ("
        << num_views << " cameras, "
        << num_features << " features)..." << std::endl;

    Bundle::Ptr bundle = Bundle::create();

    /* Read all cameras. */
    Bundle::Cameras& cameras = bundle->get_cameras();
    cameras.reserve(num_views);
    for (int i = 0; i < num_views; ++i)
    {
        cameras.push_back(CameraInfo());
        CameraInfo& cam = cameras.back();
        in >> cam.flen >> cam.dist[0] >> cam.dist[1];
        for (int j = 0; j < 9; ++j)
            in >> cam.rot[j];
        for (int j = 0; j < 3; ++j)
            in >> cam.trans[j];
    }

    if (in.eof())
        throw util::Exception("Unexpected EOF in bundle file");
    if (in.fail())
        throw util::Exception("Bundle file read error");

    /* Read all features. */
    Bundle::Features& features = bundle->get_features();
    features.reserve(num_features);
    for (int i = 0; i < num_features; ++i)
    {
        /* Insert the new (uninitialized) point. */
        features.push_back(Bundle::Feature3D());
        Bundle::Feature3D& feature = features.back();

        /* Read point position and color. */
        for (int j = 0; j < 3; ++j)
            in >> feature.pos[j];
        for (int j = 0; j < 3; ++j)
        {
            in >> feature.color[j];
            feature.color[j] /= 255.0f;
        }

        /* Read feature references. */
        int ref_amount = 0;
        in >> ref_amount;
        if (ref_amount < 0 || ref_amount > num_views)
        {
            in.close();
            throw util::Exception("Invalid feature reference amount");
        }

        for (int j = 0; j < ref_amount; ++j)
        {
            /*
             * Photosynther: The third parameter is the reprojection quality.
             * Bundler: The third and forth parameter are the floating point
             * x- and y-coordinate in an image-centered coordinate system.
             */
            Bundle::Feature2D ref;
            float dummy_float;
            if (format == BUNDLE_FORMAT_PHOTOSYNTHER)
            {
                in >> ref.view_id >> ref.feature_id;
                in >> dummy_float; // Drop reprojection quality.
                std::fill(ref.pos, ref.pos + 2, -1.0f);
            }
            else if (format == BUNDLE_FORMAT_NOAHBUNDLER)
            {
                in >> ref.view_id >> ref.feature_id;
                in >> ref.pos[0] >> ref.pos[1];
            }
            feature.refs.push_back(ref);
        }

        /* Check for premature EOF. */
        if (in.eof())
        {
            std::cerr << "Warning: Unexpected EOF (at feature "
                << i << ")" << std::endl;
            features.pop_back();
            break;
        }
    }

    in.close();
    return bundle;
}

/* ------------------ Support for Noah "Bundler"  ----------------- */

Bundle::Ptr
load_bundler_bundle (std::string const& filename)
{
    return load_bundler_ps_intern(filename, BUNDLE_FORMAT_NOAHBUNDLER);
}

/* ------------------- Support for Photosynther ------------------- */

Bundle::Ptr
load_photosynther_bundle (std::string const& filename)
{
    return load_bundler_ps_intern(filename, BUNDLE_FORMAT_PHOTOSYNTHER);
}

void
save_photosynther_bundle (Bundle::ConstPtr bundle, std::string const& filename)
{
    Bundle::Features const& features = bundle->get_features();
    Bundle::Cameras const& cameras = bundle->get_cameras();

    std::cout << "Writing bundle (" << cameras.size() << " cameras, "
        << features.size() << " features): " << filename << "...\n";

    std::ofstream out(filename.c_str(), std::ios::binary);
    if (!out.good())
        throw util::FileException(filename, std::strerror(errno));

    out << "drews 1.0\n";
    out << cameras.size() << " " << features.size() << "\n";

    /* Write all cameras to bundle file. */
    for (std::size_t i = 0; i < cameras.size(); ++i)
    {
        CameraInfo const& cam = cameras[i];

        bool camera_valid = true;
        for (int j = 0; camera_valid && j < 3; ++j)
            if (MATH_ISINF(cam.trans[j]) || MATH_ISNAN(cam.trans[j]))
                camera_valid = false;
        for (int j = 0; camera_valid && j < 9; ++j)
            if (MATH_ISINF(cam.rot[j]) || MATH_ISNAN(cam.rot[j]))
                camera_valid = false;

        if (cam.flen == 0.0f || !camera_valid)
        {
            for (int i = 0; i < 5 * 3; ++i)
                out << "0" << (i % 3 == 2 ? "\n" : " ");
            continue;
        }

        out << cam.flen << " " << cam.dist[0] << " " << cam.dist[1] << "\n";
        out << cam.rot[0] << " " << cam.rot[1] << " " << cam.rot[2] << "\n";
        out << cam.rot[3] << " " << cam.rot[4] << " " << cam.rot[5] << "\n";
        out << cam.rot[6] << " " << cam.rot[7] << " " << cam.rot[8] << "\n";
        out << cam.trans[0] << " " << cam.trans[1]
            << " " << cam.trans[2] << "\n";
    }

    /* Write all features to bundle file. */
    for (std::size_t i = 0; i < features.size(); ++i)
    {
        Bundle::Feature3D const& p = features[i];
        out << p.pos[0] << " " << p.pos[1] << " " << p.pos[2] << "\n";
        out << static_cast<int>(p.color[0] * 255.0f + 0.5f) << " "
            << static_cast<int>(p.color[1] * 255.0f + 0.5f) << " "
            << static_cast<int>(p.color[2] * 255.0f + 0.5f) << "\n";
        out << p.refs.size();
        for (std::size_t j = 0; j < p.refs.size(); ++j)
        {
            Bundle::Feature2D const& ref = p.refs[j];
            out << " " << ref.view_id << " " << ref.feature_id << " 0";
        }
        out << "\n";
    }

    out.close();
}

/* -------------- Support for Colmap --------------- */

// See colmap/src/util/types.h
typedef uint32_t camera_t;
typedef uint32_t image_t;
typedef uint64_t image_pair_t;
typedef uint32_t point2D_t;
typedef uint64_t point3D_t;

std::map<camera_t,std::string> camera_model_code_to_name;

void
define_camera_models() {
    camera_model_code_to_name.emplace(0, "SIMPLE_PINHOLE");
    camera_model_code_to_name.emplace(1, "PINHOLE");
    camera_model_code_to_name.emplace(2, "SIMPLE_RADIAL");
    camera_model_code_to_name.emplace(3, "RADIAL");
    camera_model_code_to_name.emplace(4, "OPENCV");
    camera_model_code_to_name.emplace(5, "OPENCV_FISHEYE");
    camera_model_code_to_name.emplace(6, "FULL_OPENCV");
    camera_model_code_to_name.emplace(7, "FOV");
    camera_model_code_to_name.emplace(8, "SIMPLE_RADIAL_FISHEYE");
    camera_model_code_to_name.emplace(9, "RADIAL_FISHEYE");
    camera_model_code_to_name.emplace(10, "THIN_PRISM_FISHEYE");
}

void
check_stream(std::ifstream & in, std::string const& filename)
{
    if (!in.good())
        throw util::FileException(filename, std::strerror(errno));
}

void
consume_comment_lines(std::ifstream & in)
{
    while (in.peek() == '#')
    {
        std::string temp;
        std::getline(in, temp);
    }
}

void
create_camera_info_from_params(CameraInfo& camera_info,
    std::string const& model,
    std::vector<float> const& params,
    uint32_t width,
    uint32_t height)
{
    // https://github.com/colmap/colmap/blob/dev/src/base/camera_models.h
    // https://github.com/simonfuhrmann/mve/blob/master/libs/mve/camera.cc
    if (model == "SIMPLE_PINHOLE")
    {
        // Simple pinhole: f, cx, cy
        camera_info.flen = params[0];
        camera_info.ppoint[0] = params[1] / width;
        camera_info.ppoint[1] = params[2] / height;
    }
    else if (model == "PINHOLE")
    {
        // Pinhole: fx, fy, cx, cy
        float fx = params[0];
        float fy = params[1];
        float dim_aspect = static_cast<float>(width) / height;
        float pixel_aspect = fy / fx;
        float img_aspect = dim_aspect * pixel_aspect;
        if (img_aspect < 1.0f) {
            camera_info.flen = fy / height;
        } else {
            camera_info.flen = fx / width;
        }
        camera_info.paspect = pixel_aspect;
        camera_info.ppoint[0] = params[2] / width;
        camera_info.ppoint[1] = params[3] / height;
    }
    else 
    {
        std::string msg = "Unsupported camera model with radial distortion "
            "detected! If possible, re-run the SfM reconstruction with the "
            "SIMPLE_PINHOLE or the PINHOLE camera model (recommended). "
            "Otherwise, use the undistortion step in Colmap to obtain "
            "undistorted images and corresponding camera models without radial "
            "distortion.";
        throw util::Exception(msg.c_str());
    }
}

void
load_colmap_cameras_txt(std::string const& cameras_filename,
    std::map<uint32_t, CameraInfo>& camera_colmap_id_to_info)
{
    std::cout << "Colmap: Loading camera.txt file..." << std::endl;
    std::ifstream in_cameras(cameras_filename.c_str());
    check_stream(in_cameras, cameras_filename);
    consume_comment_lines(in_cameras);
    std::string camera_line;
    uint32_t camera_colmap_id;  // starts at 1
    std::string model_name;
    uint32_t width, height;
    std::vector<float> params;
    while(getline(in_cameras, camera_line))
    {
        std::stringstream camera_line_ss(camera_line);
        camera_line_ss >> camera_colmap_id >> model_name >> width >> height;
        if (camera_line_ss.eof())
            throw util::Exception("Missing parameters");
        params.clear();
        float param;
        while (camera_line_ss >> param)
            params.push_back(param);
        CameraInfo camera_info;
        create_camera_info_from_params(
            camera_info, model_name, params, width, height);
        camera_colmap_id_to_info[camera_colmap_id] = camera_info;
    }
    in_cameras.close();
}

void
initialize_bundle_cam(CameraInfo& model,
    math::Vec4d& quat,
    math::Vec3d& trans,
    CameraInfo* bundle_cam)
{
    bundle_cam->flen = model.flen;
    bundle_cam->paspect = model.paspect;
    bundle_cam->ppoint[0] = model.ppoint[0];
    bundle_cam->ppoint[1] = model.ppoint[1];
    bundle_cam->dist[0] = model.dist[0];
    bundle_cam->dist[1] = model.dist[1];
    math::Matrix3f rot = get_rot_from_quaternion(&quat[0]);
    std::copy(rot.begin(), rot.end(), bundle_cam->rot);
    std::copy(trans.begin(), trans.end(), bundle_cam->trans);
}

void
initialize_cam_info(CameraInfo& model,
    std::string const& image_path,
    std::string& image_name,
    std::string const& depth_path,
    std::string& depth_map_name,
    AdditionalCameraInfo* colmap_cam_info)
{
    colmap_cam_info->filename = image_name;
    colmap_cam_info->depth_map_name = depth_map_name;
    colmap_cam_info->radial_distortion = model.dist[0];
    if (!util::fs::is_absolute(colmap_cam_info->filename))
        colmap_cam_info->filename = util::fs::join_path(image_path,
            colmap_cam_info->filename);
    if (!util::fs::is_absolute(colmap_cam_info->depth_map_name))
        colmap_cam_info->depth_map_name = util::fs::join_path(depth_path,
            colmap_cam_info->depth_map_name);
}

void
determine_depth_map_path(std::string const& depth_path, std::string& image_name,
    std::string* depth_map_name)
{
    std::string geometric_depth_map_name = util::fs::join_path(
        depth_path, image_name + ".geometric.bin");
    std::string photometric_depth_map_name = util::fs::join_path(
        depth_path, image_name + ".photometric.bin");

    if (util::fs::file_exists(geometric_depth_map_name.c_str()))
        *depth_map_name = geometric_depth_map_name;
    else if (util::fs::file_exists(photometric_depth_map_name.c_str()))
        *depth_map_name = photometric_depth_map_name;
    else
        *depth_map_name = "";
}

void
load_colmap_images_txt(std::string const& images_filename,
    std::string const& image_path,
    std::string const& depth_path,
    std::map<uint32_t, CameraInfo>& camera_colmap_id_to_model,
    Bundle::Ptr& bundle,
    std::map< int, std::vector<Bundle::Feature2D> >* view_id_to_features_2d,
    std::vector<AdditionalCameraInfo>* camera_info)
{
    std::cout << "Colmap: Loading images.txt file..." << std::endl;
    std::cout << "Colmap: image_path " << image_path << std::endl;
    std::cout << "Colmap: depth_path " << depth_path << std::endl;
    std::ifstream in_images(images_filename.c_str());
    check_stream(in_images, images_filename);
    consume_comment_lines(in_images);

    Bundle::Cameras& bundle_cams = bundle->get_cameras();
    std::vector<AdditionalCameraInfo> colmap_cams_info;
    std::string image_line;
    std::string point_2d_line;
    math::Vec4d quat;
    math::Vec3d trans;
    int view_id;                    // starts at 0
    uint32_t view_colmap_id;        // starts at 1
    uint32_t camera_colmap_id;      // starts at 1
    int feature_3d_colmap_id;  // starts at 1
    std::string image_name;
    std::string depth_map_name;
    while (std::getline(in_images, image_line))
    {
        AdditionalCameraInfo colmap_cam_info;
        CameraInfo bundle_cam;
        std::stringstream image_line_ss(image_line);
        image_line_ss >> view_colmap_id
            >> quat[0] >> quat[1] >> quat[2] >> quat[3]
            >> trans[0] >> trans[1] >> trans[2]
            >> camera_colmap_id >> image_name;
        view_id = view_colmap_id - 1;
        image_name = util::fs::sanitize_path(image_name);
        CameraInfo model = camera_colmap_id_to_model.at(camera_colmap_id);
        initialize_bundle_cam(model, quat, trans, &bundle_cam);
        determine_depth_map_path(depth_path, image_name, &depth_map_name);
        initialize_cam_info(model, image_path, image_name, depth_path, 
            depth_map_name, &colmap_cam_info);

        std::string point_2d_line;
        std::getline(in_images, point_2d_line);
        std::stringstream point_2d_line_ss(point_2d_line);
        while (!point_2d_line_ss.eof())
        {
            Bundle::Feature2D ref;
            ref.view_id = view_id;
            point_2d_line_ss >> ref.pos[0] >> ref.pos[1];
            point_2d_line_ss >> feature_3d_colmap_id;
            // A POINT2D that does not correspond to a POINT3D has a POINT3D_ID 
            // of -1
            if (feature_3d_colmap_id == -1)
                ref.feature_id = -1;
            else
                ref.feature_id = feature_3d_colmap_id - 1;
            (*view_id_to_features_2d)[view_id].push_back(ref);
        }
        bundle_cams.push_back(bundle_cam);
        colmap_cams_info.push_back(colmap_cam_info);
    }    
    if (camera_info != nullptr)
        std::swap(*camera_info, colmap_cams_info);
    in_images.close();
}

void
load_colmap_points_3D_txt(std::string const& points3D_filename,
    std::map< int, std::vector<Bundle::Feature2D> >& view_id_to_features_2d,
    Bundle::Ptr& bundle)
{
    std::cout << "Colmap: Loading points3D.txt file..." << std::endl;
    std::ifstream in_points3D(points3D_filename.c_str());
    check_stream(in_points3D, points3D_filename);
    consume_comment_lines(in_points3D);
    Bundle::Features& features = bundle->get_features();

    std::size_t num_views = bundle->get_cameras().size();
    int num_points_3d = 0;
    std::string point_3d_line;
    while (std::getline(in_points3D, point_3d_line))
    {
        std::stringstream point_3d_line_ss(point_3d_line);
        Bundle::Feature3D feature_3d;
        int r,g,b;
        float e;
        int feature_3d_id;          // starts at 0
        int feature_3d_colmap_id;   // starts at 1
        point_3d_line_ss >> feature_3d_colmap_id;
        feature_3d_id = feature_3d_colmap_id - 1;
        point_3d_line_ss >> feature_3d.pos[0]
            >> feature_3d.pos[1]
            >> feature_3d.pos[2];
        point_3d_line_ss >> r >> g >> b;
        feature_3d.color[0] = r / 255.0f;
        feature_3d.color[1] = g / 255.0f;
        feature_3d.color[2] = b / 255.0f;
        point_3d_line_ss >> e;
        if (point_3d_line_ss.eof())
            continue;

        std::size_t num_refs = 0;
        std::vector<int> view_ids;
        std::vector<Bundle::Feature2D> refs;
        while (!point_3d_line_ss.eof())
        {
            int view_colmap_id;      // starts at 1
            int feature_2d_idx;      // starts at 0
            point_3d_line_ss >> view_colmap_id >> feature_2d_idx;
            int view_id = view_colmap_id - 1;
            // Make sure that each point only has a single observation per 
            // image, since MVE does not support multiple observations.
            if (std::find(view_ids.begin(), view_ids.end(), view_id) != 
                view_ids.end())
                continue;
            view_ids.push_back(view_id);
            Bundle::Feature2D corresponding_feature = view_id_to_features_2d.at(
                view_id).at(feature_2d_idx);
            assert(corresponding_feature.feature_id == feature_3d_id);
            Bundle::Feature2D ref;
            ref.view_id = view_id;
            ref.feature_id = feature_2d_idx;
            ref.pos[0] = corresponding_feature.pos[0];
            ref.pos[1] = corresponding_feature.pos[1];
            refs.push_back(ref);
            ++num_refs;
        }

        /* There should be at least 2 cameras that see the point. */
        if (num_refs < 2 || num_refs > num_views)
        {
            throw util::Exception("Invalid number of feature refs: ",
                util::string::get(num_refs));
        }
        feature_3d.refs = refs;
        features.push_back(feature_3d);
        ++num_points_3d;
    }
    in_points3D.close();
}

void
read_colmap_cameras_bin_params(std::vector<float>& params,
    std::string const& model, std::ifstream& in_cameras)
{
    if (model == "SIMPLE_PINHOLE")
        params.resize(3);
    else if (model == "PINHOLE" || model == "SIMPLE_RADIAL")
        params.resize(4);
    else
        throw util::Exception("Unsupported camera model provided");
    for (std::size_t param_idx = 0; param_idx < params.size(); ++param_idx)
        params[param_idx] = util::system::read_binary_little_endian<double>(
            &in_cameras);
}

void
load_colmap_cameras_bin(std::string const& cameras_filename,
    std::map<uint32_t, CameraInfo>& camera_colmap_id_to_info)
{
    using util::system::read_binary_little_endian;
    std::cout << "Colmap: Loading cameras.bin file..." << std::endl;
    define_camera_models();
    std::ifstream in_cameras(cameras_filename.c_str());

    uint32_t camera_colmap_id;  // starts at 1
    std::string model_name;
    uint32_t width, height;
    std::vector<float> params;
    uint64_t num_cam_models = read_binary_little_endian<uint64_t>(&in_cameras);
    for (uint64_t model_idx = 0; model_idx < num_cam_models; ++model_idx)
    {
        camera_colmap_id = read_binary_little_endian<camera_t>(&in_cameras);
        camera_t model_code = read_binary_little_endian<int>(&in_cameras);
        model_name = camera_model_code_to_name.at(model_code);
        width = (uint32_t)read_binary_little_endian<uint64_t>(&in_cameras);
        height = (uint32_t)read_binary_little_endian<uint64_t>(&in_cameras);
        read_colmap_cameras_bin_params(params, model_name, in_cameras);
        CameraInfo camera_info;
        create_camera_info_from_params(
            camera_info, model_name, params, width, height);
        camera_colmap_id_to_info[camera_colmap_id] = camera_info;
    }
    in_cameras.close();
}

void
read_image_name(std::istream* in_images, std::string* image_name)
{
    (*image_name) = "";
    char nameChar;
    do
    {
        in_images->read(&nameChar, 1);
        if (nameChar != '\0')
            (*image_name) += nameChar;
    }
    while (nameChar != '\0');
}

void
load_colmap_images_bin(std::string const& images_filename,
    std::string const& image_path,
    std::string const& depth_path,
    std::map<uint32_t, CameraInfo>& camera_colmap_id_to_model,
    Bundle::Ptr& bundle,
    std::map< int, std::vector<Bundle::Feature2D> >* view_id_to_features_2d,
    std::vector<AdditionalCameraInfo>* camera_info)
{
    using util::system::read_binary_little_endian;
    std::cout << "Colmap: Loading images.bin file..." << std::endl;
    std::cout << "Colmap: image_path " << image_path << std::endl;
    std::cout << "Colmap: depth_path " << depth_path << std::endl;
    std::ifstream in_images(images_filename.c_str());
    check_stream(in_images, images_filename);

    Bundle::Cameras& bundle_cams = bundle->get_cameras();
    std::vector<AdditionalCameraInfo> colmap_cams_info;
    uint64_t num_views = read_binary_little_endian<uint64_t>(&in_images);
    bundle_cams.reserve(num_views);
    colmap_cams_info.reserve(num_views);

    math::Vec4d quat;
    math::Vec3d trans;
    int view_id;                    // starts at 0
    uint32_t view_colmap_id;        // starts at 1
    uint32_t camera_colmap_id;      // starts at 1
    int feature_3d_colmap_id;       // starts at 1
    std::string image_name;
    std::string depth_map_name;
    for (std::size_t view_index = 0; view_index < num_views; ++view_index)
    {
        AdditionalCameraInfo colmap_cam_info;
        CameraInfo bundle_cam;
        view_colmap_id = read_binary_little_endian<image_t>(&in_images);
        view_id = view_colmap_id - 1;
        quat[0] = read_binary_little_endian<double>(&in_images);
        quat[1] = read_binary_little_endian<double>(&in_images);
        quat[2] = read_binary_little_endian<double>(&in_images);
        quat[3] = read_binary_little_endian<double>(&in_images);
        trans[0] = read_binary_little_endian<double>(&in_images);
        trans[1] = read_binary_little_endian<double>(&in_images);
        trans[2] = read_binary_little_endian<double>(&in_images);
        camera_colmap_id = read_binary_little_endian<camera_t>(&in_images);
        read_image_name(&in_images, &image_name);
        image_name = util::fs::sanitize_path(image_name);
        CameraInfo model = camera_colmap_id_to_model.at(camera_colmap_id);
        initialize_bundle_cam(model, quat, trans, &bundle_cam);
        determine_depth_map_path(depth_path, image_name, &depth_map_name);
        initialize_cam_info(model, image_path, image_name, depth_path, 
            depth_map_name, &colmap_cam_info);

        const std::size_t num_points_2D = read_binary_little_endian<uint64_t>(
            &in_images);
        for (std::size_t point_2d_idx = 0; point_2d_idx < num_points_2D; 
            ++point_2d_idx) 
        {
            Bundle::Feature2D ref;
            ref.view_id = view_id;
            ref.pos[0] = (float)read_binary_little_endian<double>(&in_images);
            ref.pos[1] = (float)read_binary_little_endian<double>(&in_images);
            // A POINT2D that does not correspond to a POINT3D has a POINT3D_ID 
            // of -1
            feature_3d_colmap_id = (uint32_t)read_binary_little_endian<
                point3D_t>(&in_images);
            if (feature_3d_colmap_id == -1)
                ref.feature_id = -1;
            else
                ref.feature_id = feature_3d_colmap_id - 1;
            (*view_id_to_features_2d)[view_id].push_back(ref);
        }
        bundle_cams.push_back(bundle_cam);
        colmap_cams_info.push_back(colmap_cam_info);
    }
    if (camera_info != nullptr)
        std::swap(*camera_info, colmap_cams_info);
    in_images.close();
}

void
load_colmap_points_3D_bin(std::string const& points3D_filename,
    std::map< int, std::vector<Bundle::Feature2D> >& view_id_to_features_2d,
    Bundle::Ptr& bundle)
{
    using util::system::read_binary_little_endian;
    std::cout << "Colmap: Loading points3D.bin file..." << std::endl;
    std::ifstream in_points3D(points3D_filename.c_str());
    check_stream(in_points3D, points3D_filename);

    uint64_t num_features = read_binary_little_endian<uint64_t>(&in_points3D);
    Bundle::Features& features = bundle->get_features();
    features.reserve(num_features);
    std::size_t num_views = bundle->get_cameras().size();
    for (std::size_t feature_3d_idx = 0; feature_3d_idx < num_features; 
        ++feature_3d_idx)
    {
        Bundle::Feature3D feature_3d;
        int r,g,b;
        int feature_3d_id;          // starts at 0
        int feature_3d_colmap_id;   // starts at 1
        feature_3d_colmap_id = (uint32_t)read_binary_little_endian<point3D_t>(
            &in_points3D);
        feature_3d_id = feature_3d_colmap_id - 1; // Fix Colmap id
        feature_3d.pos[0] = read_binary_little_endian<double>(&in_points3D);
        feature_3d.pos[1] = read_binary_little_endian<double>(&in_points3D);
        feature_3d.pos[2] = read_binary_little_endian<double>(&in_points3D);
        r = read_binary_little_endian<uint8_t>(&in_points3D);
        g = read_binary_little_endian<uint8_t>(&in_points3D);
        b = read_binary_little_endian<uint8_t>(&in_points3D);
        read_binary_little_endian<double>(&in_points3D);   // read error
        feature_3d.color[0] = r / 255.0f;
        feature_3d.color[1] = g / 255.0f;
        feature_3d.color[2] = b / 255.0f;

        std::size_t num_refs = read_binary_little_endian<uint64_t>(
            &in_points3D);
        std::vector<int> view_ids;
        std::vector<Bundle::Feature2D> refs;
        for (std::size_t ref_idx = 0; ref_idx < num_refs; ++ref_idx)
        {
            uint32_t view_colmap_id;    // starts at 1
            int feature_2d_idx;         // starts at 0
            view_colmap_id = read_binary_little_endian<image_t>(&in_points3D);
            int view_id = view_colmap_id - 1;
            feature_2d_idx = read_binary_little_endian<point2D_t>(&in_points3D);
            // Make sure that each point only has a single observation per 
            // image, since MVE does not support multiple observations.
            if (std::find(view_ids.begin(), view_ids.end(), 
                view_id) != view_ids.end())
                continue;
            view_ids.push_back(view_id);
            Bundle::Feature2D corresponding_feature = view_id_to_features_2d.at(
                view_id).at(feature_2d_idx);
            assert(corresponding_feature.feature_id == feature_3d_id);
            Bundle::Feature2D ref;
            ref.view_id = view_id;
            ref.feature_id = feature_2d_idx;
            ref.pos[0] = corresponding_feature.pos[0];
            ref.pos[1] = corresponding_feature.pos[1];
            refs.push_back(ref);
        }
        num_refs = refs.size();

        /* There should be at least 2 cameras that see the point. */
        if (num_refs < 2 || num_refs > num_views) 
        {
            throw util::Exception("Invalid number of feature refs: ",
                util::string::get(num_refs));
        }
        feature_3d.refs = refs;
        features.push_back(feature_3d);
    }
    in_points3D.close();
}

Bundle::Ptr
load_colmap_bundle (std::string const& workspace_path,
    std::vector<AdditionalCameraInfo>* camera_info)
{
    using util::fs::join_path;
    // https://github.com/colmap/colmap/blob/dev/src/base/reconstruction.cc
    // void Reconstruction::ReadText(const std::string& path)
    // void Reconstruction::ReadBinary(const std::string& path)

    std::string model_path = join_path(workspace_path, "sparse");
    std::string image_path = join_path(workspace_path, "images");
    std::string stereo_path = join_path(workspace_path, "stereo");
    std::string depth_path = join_path(stereo_path, "depth_maps");

    std::string cameras_txt_filename = join_path(model_path, "cameras.txt");
    std::string cameras_bin_filename = join_path(model_path, "cameras.bin");

    std::string images_txt_filename = join_path(model_path, "images.txt");
    std::string images_bin_filename = join_path(model_path, "images.bin");

    std::string points_3D_txt_filename = join_path(model_path, "points3D.txt");
    std::string points_3D_bin_filename = join_path(model_path, "points3D.bin");

    std::cout << "Colmap: Loading workspace..." << std::endl;
    std::cout << workspace_path << std::endl;
    // The depth maps are optional
    if (!util::fs::dir_exists(model_path.c_str()))
        throw util::Exception("Sparse model directory missing: ",
            model_path);
    if (!util::fs::dir_exists(image_path.c_str()))
        throw util::Exception("Undistored image directory missing: ",
            image_path);

    std::map<uint32_t, CameraInfo> camera_colmap_id_to_info;
    if (util::fs::file_exists(cameras_txt_filename.c_str()))
        load_colmap_cameras_txt(
            cameras_txt_filename,
            camera_colmap_id_to_info);
    else
        load_colmap_cameras_bin(
            cameras_bin_filename,
            camera_colmap_id_to_info);

    Bundle::Ptr bundle_colmap = Bundle::create();
    std::map< int, std::vector<Bundle::Feature2D> > view_id_to_features_2d;
    std::vector<AdditionalCameraInfo> colmap_camera_info;
    if (util::fs::file_exists(images_txt_filename.c_str()))
        load_colmap_images_txt(
            images_txt_filename,
            image_path,
            depth_path,
            camera_colmap_id_to_info,
            bundle_colmap,
            &view_id_to_features_2d,
            &colmap_camera_info);
    else
        load_colmap_images_bin(
            images_bin_filename,
            image_path,
            depth_path,
            camera_colmap_id_to_info,
            bundle_colmap,
            &view_id_to_features_2d,
            &colmap_camera_info);

    if (util::fs::file_exists(points_3D_txt_filename.c_str()))
        load_colmap_points_3D_txt(
            points_3D_txt_filename,
            view_id_to_features_2d,
            bundle_colmap);
    else
        load_colmap_points_3D_bin(
            points_3D_bin_filename,
            view_id_to_features_2d,
            bundle_colmap);

    *camera_info = colmap_camera_info;

    return bundle_colmap;
}

/* -------------- Support for Colmap Depth Maps --------------- */

mve::FloatImage::Ptr
parse_colmap_depth_map(const std::string& path)
{
    using util::system::read_binary_little_endian;
    std::ifstream text_file(path, std::ios::binary);

    if (!util::fs::file_exists(path.c_str()))
    {
        throw util::Exception("Depth map not found in ", path);
    }

    size_t depth_map_width = 0;
    size_t depth_map_height = 0;
    size_t depth_map_depth = 0;
    char unused_char;
    text_file >> depth_map_width >> unused_char >> depth_map_height
        >> unused_char >> depth_map_depth >> unused_char;
    std::streampos pos = text_file.tellg();
    text_file.close();

    assert(depth_map_width > 0);
    assert(depth_map_height > 0);
    assert(depth_map_depth == 1);

    std::ifstream binary_file(path, std::ios::binary);
    binary_file.seekg(pos);

    mve::FloatImage::Ptr depth_img = mve::FloatImage::create(
        depth_map_width, depth_map_height, 1);

    for (int i = 0; i < depth_img->get_pixel_amount(); ++i)
        depth_img->at(i) = read_binary_little_endian<float>(
            &binary_file);

    binary_file.close();
    return depth_img;
}

mve::FloatImage::Ptr
load_colmap_depth_map(int scale, mve::CameraInfo& mve_cam, int original_width,
    int original_height,
    mve::AdditionalCameraInfo const& cam_info)
{
    assert(scale >= 0);

    mve::FloatImage::Ptr depth_image = parse_colmap_depth_map(
        cam_info.depth_map_name);

    int depth_width = depth_image->width();
    int depth_height = depth_image->height();

    math::Matrix3f inv_calib;
    mve_cam.fill_inverse_calibration(*inv_calib, original_width,
        original_height);
    mve::image::depthmap_convert_conventions<float>(depth_image, inv_calib, 
        true);

    if (depth_width == original_width && depth_height == original_height)
    {
        // Lossless resizing
        for (int i = 0; i < scale; ++i)
            depth_image = mve::image::rescale_half_size_subsample<
                float>(depth_image);
    }
    else
    {
        std::ostringstream string_stream;
        string_stream
            << "Colmap depth map of size " << depth_width << " x "
            << depth_height << " does not match the corresponding undistorted "
            << "image of size " << original_width << " x " << original_height
            << ". Re-compute the depth maps using Colmap without limiting the "
            << "depth map size.";
        throw util::Exception(string_stream.str().c_str());
    }
    return depth_image;
}

MVE_NAMESPACE_END
