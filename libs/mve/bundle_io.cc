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

#include "util/file_system.h"
#include "util/string.h"
#include "util/exception.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "mve/bundle_io.h"

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
    std::vector<NVMCameraInfo>* camera_info)
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
    std::vector<NVMCameraInfo> nvm_cams;
    nvm_cams.reserve(num_views);

    /* Read views. */
    std::cout << "NVM: Number of views: " << num_views << std::endl;
    std::string nvm_path = util::fs::dirname(filename);
    for (int i = 0; i < num_views; ++i)
    {
        NVMCameraInfo nvm_cam;
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

	if (num_views < 0 || num_views >= 10000
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

MVE_NAMESPACE_END
