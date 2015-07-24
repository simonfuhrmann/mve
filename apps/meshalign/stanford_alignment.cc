/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <cstring>
#include <cerrno>
#include <iostream>
#include <fstream>

#include "util/file_system.h"
#include "util/string.h"
#include "util/exception.h"
#include "util/tokenizer.h"
#include "mve/mesh.h"
#include "mve/mesh_io.h"
#include "mve/mesh_tools.h"
#include "math/quaternion.h"

#include "stanford_alignment.h"

void
StanfordAlignment::read_alignment (std::string const& conffile)
{
    std::string basepath = util::fs::dirname(conffile);

    std::ifstream in(conffile.c_str());
    if (!in.good())
        throw util::FileException(conffile, ::strerror(errno));

    math::Vec3f camera_pos(0.0f);
    math::Quat4f camera_rot(0.0f);
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
            camera_pos[0] = util::string::convert<float>(t[1]);
            camera_pos[1] = util::string::convert<float>(t[2]);
            camera_pos[2] = util::string::convert<float>(t[3]);
            camera_rot[0] = util::string::convert<float>(t[4]);
            camera_rot[1] = util::string::convert<float>(t[5]);
            camera_rot[2] = util::string::convert<float>(t[6]);
            camera_rot[3] = util::string::convert<float>(t[7]);
            valid_cam = true;
        }
        else if (t[0] == "bmesh" && t.size() == 9)
        {
            if (!valid_cam)
            {
                std::cerr << "WARNING: Using uninitialized global camera; "
                    "expect the unexpected!" << std::endl;
            }

            RangeImage ri;
            ri.filename = t[1];
            ri.fullpath = util::fs::join_path(basepath, t[1]);
            ri.translation[0] = util::string::convert<float>(t[2]);
            ri.translation[1] = util::string::convert<float>(t[3]);
            ri.translation[2] = util::string::convert<float>(t[4]);

            math::Quat4f temp_rotation;
            temp_rotation[0] = util::string::convert<float>(t[8]);
            temp_rotation[1] = util::string::convert<float>(t[5]);
            temp_rotation[2] = util::string::convert<float>(t[6]);
            temp_rotation[3] = util::string::convert<float>(t[7]);
            temp_rotation.to_rotation_matrix(ri.rotation.begin());

            /* This is a bit strange... */
            ri.campos = ri.rotation * camera_rot.rotate(camera_pos) + ri.translation;
            ri.viewdir = ri.rotation * camera_rot.rotate(math::Vec3f(0,0,1));
            this->images.push_back(ri);
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
StanfordAlignment::get_mesh (RangeImage const& ri)
{
    mve::TriangleMesh::Ptr ret;
    ret = mve::geom::load_mesh(ri.fullpath);
    mve::TriangleMesh::VertexList& verts(ret->get_vertices());
    for (std::size_t i = 0; i < verts.size(); ++i)
        verts[i] = ri.rotation * verts[i] + ri.translation;
    return ret;
}
