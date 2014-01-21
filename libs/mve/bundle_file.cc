#include <cstring>
#include <cerrno>
#include <iostream>
#include <fstream>

#include "util/exception.h"
#include "util/string.h"
#include "mve/bundle_file.h"

MVE_NAMESPACE_BEGIN

void
BundleFile::read_bundle (std::string const& filename)
{
    std::ifstream  in(filename.c_str());
    if (!in.good())
        throw util::Exception("Cannot open file: ", std::strerror(errno));
    std::string first_line;
    std::getline(in, first_line);
    in.close();

    util::string::clip_newlines(&first_line);
    util::string::clip_whitespaces(&first_line);

    if (first_line == "drews 1.0")
        this->format = BUNDLER_PHOTOSYNTHER;
    else if (first_line == "# Bundle file v0.3")
        this->format = BUNDLER_NOAHBUNDLER;
    else
        throw util::Exception("Unknown bundle file identification");

    this->read_bundle_intern(filename);
}

/* -------------------------------------------------------------- */

/*
 * ==== Photosynther bundle file format ====
 *
 * "drews 1.0"
 * <num_cameras> <num_points>
 * <cam 1 line 1> // Focal length, Radial distortion: f rd1 rd2
 * <cam 1 line 2> // Rotation matrix row 1: r11 r12 r13
 * <cam 1 line 3> // Rotation matrix row 2: r21 r22 r23
 * <cam 1 line 4> // Rotation matrix row 3: r31 r32 r33
 * <cam 1 line 5> // Translation vector: t1 t2 t3
 * ...
 * <point 1 position (float)> // x y z
 * <point 1 color (uchar)> // r g b
 * <point 1 visibility> // <list length (uint)> ( <img id (uint)> <sift id (uint)> <reproj. quality (float)> ) ...
 * ...
 *
 *
 * ==== Noah Snavely bundle file format ====
 *
 * "# Bundle file v0.3"
 * <num_cameras> <num_points>
 * <cam 1 line 1> // Focal length, Radial distortion: f k1 k2
 * <cam 1 line 2> // Rotation matrix row 1: r11 r12 r13
 * <cam 1 line 3> // Rotation matrix row 2: r21 r22 r23
 * <cam 1 line 4> // Rotation matrix row 3: r31 r32 r33
 * <cam 1 line 5> // Translation vector: t1 t2 t3
 * ...
 * <point 1 position (float)> // x y z
 * <point 1 color (uchar)> // r g b
 * <point 1 visibility> // <list length (uint)> ( <img ID (uint)> <sift ID (uint)> <x (float)> <y (float)> ) ...
 * ...
 *
 * ==== A few notes on the bundler format ====
 *
 * Each camera in the bundle file corresponds to the ordered
 * list of input images. Some cameras are set to zero, which means the input
 * image was not registered. <cam ID> is the ID w.r.t. the input images,
 * <sift ID> is the ID of the SIFT feature point for that image. In the
 * Noah bundler, <x> and <y> are floating point positions of the keypoint
 * in the image, given in image-centered coordinate system.
 */
void
BundleFile::read_bundle_intern (std::string const& filename)
{
    std::ifstream  in(filename.c_str());
    if (!in.good())
        throw util::Exception("Cannot open file: ", std::strerror(errno));

    /* Read version information in the first line. */
    std::getline(in, this->version);
    util::string::clip_newlines(&this->version);
    util::string::clip_whitespaces(&this->version);

    /* Read number of cameras and number of points. */
    this->num_valid_cams = 0;
    std::size_t num_cameras(0), num_points(0);
    in >> num_cameras >> num_points;

    if (in.eof())
    {
        in.close();
        throw util::Exception("Error reading bundle: Unexpected EOF");
    }

    if (num_cameras > 10000 || num_points > 100000000)
    {
        in.close();
        throw util::Exception("Spurious amount of cameras or features");
    }

    /* Print message according to passed parser format. */
    {
        std::string parser;
        switch (this->format)
        {
            case BUNDLER_PHOTOSYNTHER: parser = "Photosynther"; break;
            case BUNDLER_NOAHBUNDLER: parser = "Noah Bundler"; break;
            default: throw std::runtime_error("Invalid parser format");
        }

        std::cout << "Reading " << parser << " file ("
            << num_cameras << " cameras, "
            << num_points << " points)..." << std::endl;
    }

    this->cameras.reserve(num_cameras);
    this->points.reserve(num_points);

    /* Read all cameras. */
    for (std::size_t i = 0; i < num_cameras; ++i)
    {
        cameras.push_back(CameraInfo());
        CameraInfo& cam = cameras.back();
        in >> cam.flen >> cam.dist[0] >> cam.dist[1];
        for (int j = 0; j < 9; ++j)
            in >> cam.rot[j];
        for (int j = 0; j < 3; ++j)
            in >> cam.trans[j];
        if (cam.flen != 0.0f)
            this->num_valid_cams += 1;
    }

    if (in.eof())
    {
        in.close();
        throw util::Exception("Unexpected EOF");
    }

    /* Read all points. */
    for (std::size_t i = 0; i < num_points; ++i)
    {
        /* Insert the new (uninitialized) point. */
        points.push_back(FeaturePoint());
        FeaturePoint& point(points.back());

        /* Read point position and color. */
        int color[3];
        in >> point.pos[0] >> point.pos[1] >> point.pos[2];
        in >> color[0] >> color[1] >> color[2];
        for (int j = 0; j < 3; ++j)
            point.color[j] = color[j];

        /* Read feature references. */
        int ref_amount;
        in >> ref_amount;
        if (ref_amount < 0 || ref_amount > static_cast<int>(num_cameras))
        {
            in.close();
            throw util::Exception("Invalid feature reference amount");
        }

        for (int j = 0; j < ref_amount; ++j)
        {
            /* For the Photosynther, the third parameter is the reprojection
             * quality, for the Noah-bundler, the third and forth parameter
             * is the floating point x- and y-coordinate in the image. */
            FeaturePointRef point_ref;
            point_ref.error = 0.0f;
            float dummy_float;
            in >> point_ref.img_id >> point_ref.feature_id;
            if (this->format == BUNDLER_PHOTOSYNTHER)
                in >> point_ref.error;
            if (this->format == BUNDLER_NOAHBUNDLER)
                in >> dummy_float >> dummy_float;
            point.refs.push_back(point_ref);
        }

        /* Check for premature EOF. */
        if (in.eof())
        {
            std::cerr << "Warning: Unexpected EOF at point " << i << std::endl;
            points.pop_back();
            break;
        }
    }

    in.close();
}

/* -------------------------------------------------------------- */

void
BundleFile::write_bundle (std::string const& filename)
{
    std::cout << "Writing bundle file " << filename
        << " (version \"" << this->version << "\", "
        << this->cameras.size() << " cameras, "
        << this->points.size() << " points)..." << std::endl;

    std::ofstream out(filename.c_str());
    if (!out.good())
        throw util::FileException(filename, std::strerror(errno));

    out << "drews 1.0" << std::endl;
    out << cameras.size() << " " << points.size() << std::endl;

    for (std::size_t i = 0; i < cameras.size(); ++i)
    {
        if (cameras[i].flen == 0.0f)
        {
            for (int i = 0; i < 5 * 3; ++i)
                out << "0" << (i % 3 == 2 ? "\n" : " ");
            continue;
        }

        CameraInfo& cam = cameras[i];
        out<< cam.flen << " " << cam.dist[0] << " " << cam.dist[1] << std::endl;
        out<< cam.rot[0] << " " << cam.rot[1] << " " << cam.rot[2] << std::endl;
        out<< cam.rot[3] << " " << cam.rot[4] << " " << cam.rot[5] << std::endl;
        out<< cam.rot[6] << " " << cam.rot[7] << " " << cam.rot[8] << std::endl;
        out<< cam.trans[0] << " " << cam.trans[1] << " "
            << cam.trans[2] << std::endl;
    }

    for (std::size_t i = 0; i < points.size(); ++i)
    {
        FeaturePoint& p = points[i];
        out << p.pos[0] << " " << p.pos[1] << " " << p.pos[2] << std::endl;
        out << (int)p.color[0] << " " << (int)p.color[1]
            << " " << (int)p.color[2] << std::endl;

        out << p.refs.size();
        for (std::size_t j = 0; j < p.refs.size(); ++j)
        {
            out << " " << p.refs[j].img_id << " " << p.refs[j].feature_id
                << " " << p.refs[j].error;
        }
        out << std::endl;
    }

    out.close();
}

/* -------------------------------------------------------------- */

void
BundleFile::write_points_to_ply (std::string const& filename)
{
    std::ofstream out(filename.c_str());

    out << "ply" << std::endl
        << "format ascii 1.0" << std::endl
        << "element vertex " << points.size() << std::endl
        << "property float x" << std::endl
        << "property float y" << std::endl
        << "property float z" << std::endl
        << "property uchar r" << std::endl
        << "property uchar g" << std::endl
        << "property uchar b" << std::endl
        << "end_header" << std::endl;

    for (std::size_t i = 0; i < points.size(); ++i)
    {
        out << points[i].pos[0]
            << " " << points[i].pos[1]
            << " " << points[i].pos[2]
            << " " << (int)points[i].color[0]
            << " " << (int)points[i].color[1]
            << " " << (int)points[i].color[2] << std::endl;
    }

    out.close();
}

/* -------------------------------------------------------------- */

void
BundleFile::delete_camera (std::size_t index)
{
    /* Mark the deleted camera as invalid. */
    cameras[index].flen = 0.0f;

    /* Delete all SIFT features that are visible in that camera. */
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        typedef std::vector<FeaturePointRef> RefVector;
        RefVector& refs = points[i].refs;
        for (RefVector::iterator iter = refs.begin(); iter != refs.end();)
            if (iter->img_id == (int)index)
                refs.erase(iter);
            else
                iter++;
    }
}

/* -------------------------------------------------------------- */

std::size_t
BundleFile::get_byte_size (void) const
{
    std::size_t ret = 0;
    ret += this->cameras.capacity() * sizeof(CameraInfo);
    ret += this->points.capacity() * sizeof(FeaturePoint);
    for (std::size_t i = 0; i < this->points.size(); ++i)
        ret += this->points[i].refs.capacity() * sizeof(FeaturePointRef);
    return ret;
}

/* -------------------------------------------------------------- */

TriangleMesh::Ptr
BundleFile::get_points_mesh (int cam_id) const
{
    mve::TriangleMesh::Ptr mesh(mve::TriangleMesh::create());
    mve::TriangleMesh::VertexList& verts(mesh->get_vertices());
    mve::TriangleMesh::ColorList& colors(mesh->get_vertex_colors());

    for (std::size_t i = 0; i < this->points.size(); ++i)
    {
        FeaturePoint const& p(this->points[i]);
        if (cam_id >= 0 && !p.contains_view_id(cam_id))
            continue;

        verts.push_back(math::Vec3f(p.pos));
        math::Vec4f color(1.0f);
        for (int j = 0; j < 3; ++j)
            color[j] = float(p.color[j]) / 255.0f;
        colors.push_back(color);
    }

    return mesh;
}

/* -------------------------------------------------------------- */

bool
FeaturePoint::contains_view_id (std::size_t id) const
{
    for (std::size_t i = 0; i < this->refs.size(); ++i)
        if (this->refs[i].img_id == (int)id)
            return true;
    return false;
}

MVE_NAMESPACE_END
