#include <cstring>
#include <cerrno>
#include <iostream>
#include <fstream>

#include "util/filesystem.h"
#include "util/string.h"
#include "util/exception.h"
#include "util/tokenizer.h"
#include "mve/meshtools.h"

#include "stanford.h"

DMFUSION_NAMESPACE_BEGIN

void
StanfordDataset::read_config (std::string const& conffile)
{
    std::string basepath = util::fs::get_path_component(conffile);
    basepath += "/";

    std::ifstream in(conffile.c_str());
    if (!in.good())
        throw util::FileException(conffile, ::strerror(errno));

    math::Vec3f camera(0.0f);
    math::Quat4f camquat(0.0f);
    bool valid_cam = false;

    while (!in.eof() && in.good())
    {
        std::string buf;
        std::getline(in, buf);
        util::Tokenizer t;
        t.split(buf);

        if (t.empty())
            continue;

        if (t[0] == "camera" && t.size() == 8)
        {
            camera[0] = util::string::convert<float>(t[1]);
            camera[1] = util::string::convert<float>(t[2]);
            camera[2] = util::string::convert<float>(t[3]);
            camquat[0] = util::string::convert<float>(t[4]);
            camquat[1] = util::string::convert<float>(t[5]);
            camquat[2] = util::string::convert<float>(t[6]);
            camquat[3] = util::string::convert<float>(t[7]);
            valid_cam = true;

            //std::cout << "Camera: " << camera << std::endl;
            //std::cout << "Camera quat: " << camquat
            //    << " (len " << camquat.norm() << ")" << std::endl;
        }
        else if (t[0] == "bmesh" && t.size() == 9)
        {
            if (!valid_cam)
            {
                std::cerr << "WARNING: Using uninitialized camera; "
                    "expect the unexpected!" << std::endl;
            }

            StanfordRangeImage ri;
            ri.filename = t[1];
            ri.fullpath = basepath + t[1];
            ri.translation = math::Vec3f(util::string::convert<float>(t[2]),
                util::string::convert<float>(t[3]),
                util::string::convert<float>(t[4]));
            ri.rotation[0] = /*-*/util::string::convert<float>(t[8]);
            ri.rotation[1] = util::string::convert<float>(t[5]);
            ri.rotation[2] = util::string::convert<float>(t[6]);
            ri.rotation[3] = util::string::convert<float>(t[7]);
            ri.campos = ri.rotation.rotate(camquat.rotate(camera)) + ri.translation;
            ri.viewdir = ri.rotation.rotate(camquat.rotate(math::Vec3f(0,0,1)));
            this->images.push_back(ri);

            // TODO Write XF files.

            // http://en.wikipedia.org/wiki/Rotation_matrix
            // http://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#Pseudo-code_for_rotating_using_a_quaternion_in_3D_space
            // http://de.wikipedia.org/wiki/Quaternion#Konjugation_und_Betrag

            /*
            math::Vec3f test_axis;
            float test_angle;
            ri.rotation.get_axis_angle(*test_axis, test_angle);
            std::cout << std::endl;
            std::cout << "Mesh: " << ri.fullpath << std::endl;
            std::cout << "Rotation: " << test_axis << " (len " << test_axis.norm() << "), angle: " << test_angle << std::endl;
            std::cout << "Translation: " << ri.translation << std::endl;
            */
        }
        else
        {
            std::cout << "Line not recognized: " << buf << std::endl;
        }
    }
    in.close();
}

/* ---------------------------------------------------------------- */

mve::TriangleMesh::Ptr
StanfordDataset::get_mesh (StanfordRangeImage const& ri)
{
    mve::TriangleMesh::Ptr ret;
    ret = mve::geom::load_mesh(ri.fullpath);
    mve::TriangleMesh::VertexList& verts(ret->get_vertices());
    for (std::size_t i = 0; i < verts.size(); ++i)
        verts[i] = ri.rotation.rotate(verts[i]) + ri.translation;
    return ret;
}

/* ---------------------------------------------------------------- */

mve::FloatImage::Ptr
StanfordDataset::get_depth_image (StanfordRangeImage const& ri)
{
    /*
     * Code to read stanford range images and ouput a depth map.
     */

    std::ifstream in(ri.fullpath.c_str());
    if (!in.good())
        throw std::runtime_error("Cannot open input file");

    std::string line;
    std::getline(in, line);
    if (line != "ply")
    {
        in.close();
        throw std::runtime_error("Invalid PLY file");
    }

    line.clear();

    std::size_t w = MATH_MAX_SIZE_T;
    std::size_t h = MATH_MAX_SIZE_T;
    std::size_t num_verts = MATH_MAX_SIZE_T;

    while (line != "end_header" && !in.eof())
    {
        std::getline(in, line);
        util::Tokenizer tok;
        tok.split(line);

        if (tok[0] == "obj_info" && tok[1] == "num_cols")
            w = util::string::convert<std::size_t>(tok[2]);
        else if (tok[0] == "obj_info" && tok[1] == "num_rows")
            h = util::string::convert<std::size_t>(tok[2]);
        else if (tok[0] == "element" && tok[1] == "vertex")
            num_verts = util::string::convert<std::size_t>(tok[2]);
        else if (tok[0] == "element" && tok[1] == "range_grid"
            && util::string::convert<std::size_t>(tok[2]) != w * h)
        {
            in.close();
            throw std::runtime_error("Invalid range grid specs");
        }
    }

    if (w == MATH_MAX_SIZE_T || h == MATH_MAX_SIZE_T || num_verts == MATH_MAX_SIZE_T)
    {
        in.close();
        throw std::runtime_error("Invalid PLY file format");
    }

    std::vector<math::Vec3f> verts;
    verts.resize(num_verts);
    for (std::size_t i = 0; i < num_verts; ++i)
    {
        in >> verts[i][0] >> verts[i][1] >> verts[i][2];
        verts[i] = ri.rotation.rotate(verts[i]) + ri.translation;
    }

    mve::FloatImage::Ptr img(mve::FloatImage::create(w, h, 1));
    for (std::size_t y = 0; y < h; ++y)
        for (std::size_t x = 0; x < w; ++x)
        {
            int has_id;
            unsigned int id;
            in >> has_id;
            if (has_id)
                in >> id;

            img->at(x, h - y - 1, 0) = has_id
                ? (ri.campos - verts[id]).norm()
                : 0.0f;
        }

    in.close();

    return img;
}

DMFUSION_NAMESPACE_END
