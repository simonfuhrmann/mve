/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include <cerrno>

#include "util/exception.h"
#include "util/string.h"
#include "util/file_system.h"
#include "util/tokenizer.h"
#include "mve/mesh_io.h"

#include "meshlab_alignment.h"

namespace
{
    /* A getline function that ignores empty lines and comments. */
    void
    my_getline (std::istream& in, std::string* out)
    {
        while (in.good())
        {
            std::getline(in, *out);
            util::string::clip_newlines(out);
            util::string::clip_whitespaces(out);
            if (out->empty())
                continue;
            if (out->at(0) == '#')
                continue;
            break;
        }
    }
}

void
MeshlabAlignment::read_alignment (std::string const& filename)
{
    this->images.clear();
    std::string basepath = util::fs::dirname(filename);

    std::ifstream in(filename.c_str());
    if (!in.good())
        throw util::FileException(filename, ::strerror(errno));

    /* Read number of range images. */
    std::string line;
    my_getline(in, &line);
    int const num_images = util::string::convert<int>(line);

    /* Read data for each range image. */
    for (int i = 0; i < num_images; ++i)
    {
        RangeImage ri;
        util::Tokenizer tok;

        my_getline(in, &line);
        ri.filename = line;
        ri.fullpath = util::fs::join_path(basepath, ri.filename);

        my_getline(in, &line);
        tok.split(line);
        ri.rotation[0] = tok.get_as<float>(0);
        ri.rotation[1] = tok.get_as<float>(1);
        ri.rotation[2] = tok.get_as<float>(2);
        ri.translation[0] = tok.get_as<float>(3);

        my_getline(in, &line);
        tok.split(line);
        ri.rotation[3] = tok.get_as<float>(0);
        ri.rotation[4] = tok.get_as<float>(1);
        ri.rotation[5] = tok.get_as<float>(2);
        ri.translation[1] = tok.get_as<float>(3);

        my_getline(in, &line);
        tok.split(line);
        ri.rotation[6] = tok.get_as<float>(0);
        ri.rotation[7] = tok.get_as<float>(1);
        ri.rotation[8] = tok.get_as<float>(2);
        ri.translation[2] = tok.get_as<float>(3);

        my_getline(in, &line);

        ri.campos = ri.translation;
        ri.viewdir = ri.rotation * math::Vec3f(0.0f, 0.0f, -1.0f);
        this->images.push_back(ri);
    }

    in.close();
}

mve::TriangleMesh::Ptr
MeshlabAlignment::get_mesh (RangeImage const& ri) const
{
    mve::TriangleMesh::Ptr ret;
    ret = mve::geom::load_mesh(ri.fullpath);
    mve::TriangleMesh::VertexList& verts(ret->get_vertices());
    for (std::size_t i = 0; i < verts.size(); ++i)
        verts[i] = ri.rotation * verts[i] + ri.translation;
    return ret;
}
