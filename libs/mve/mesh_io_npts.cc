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
#include <stdexcept>
#include <fstream>
#include <string>

#include "util/exception.h"
#include "math/vector.h"
#include "mve/mesh.h"
#include "mve/mesh_io_npts.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

TriangleMesh::Ptr
load_npts_mesh (std::string const& filename, bool format_binary)
{
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    std::ifstream input(filename.c_str());
    if (!input.good())
        throw util::FileException(filename, std::strerror(errno));

    TriangleMesh::Ptr mesh = TriangleMesh::create();
    TriangleMesh::VertexList& verts = mesh->get_vertices();
    TriangleMesh::NormalList& vnorm = mesh->get_vertex_normals();

    while (true)
    {
        math::Vec3f v, n;
        if (format_binary)
        {
            input.read(reinterpret_cast<char*>(*v), sizeof(float) * 3);
            input.read(reinterpret_cast<char*>(*n), sizeof(float) * 3);
        }
        else
        {
            for (int i = 0; i < 3; ++i)
                input >> v[i];
            for (int i = 0; i < 3; ++i)
                input >> n[i];
        }
        if (input.eof())
            break;

        verts.push_back(v);
        vnorm.push_back(n);
    }

    return mesh;
}

/* ---------------------------------------------------------------- */

void
save_npts_mesh (TriangleMesh::ConstPtr mesh,
    std::string const& filename, bool format_binary)
{
    if (mesh == nullptr || mesh->get_vertices().empty())
        throw std::invalid_argument("Input mesh is empty");
    if (filename.empty())
        throw std::invalid_argument("No filename given");
    if (mesh->get_vertex_normals().size() != mesh->get_vertices().size())
        throw std::invalid_argument("No vertex normals given");

    std::ofstream out(filename.c_str(), std::ios::binary);
    if (!out.good())
        throw util::FileException(filename, std::strerror(errno));

    TriangleMesh::VertexList const& verts = mesh->get_vertices();
    TriangleMesh::NormalList const& vnorm = mesh->get_vertex_normals();
    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        math::Vec3f const& v = verts[i];
        math::Vec3f const& n = vnorm[i];
        if (format_binary)
        {
            out.write(reinterpret_cast<char const*>(*v), sizeof(float) * 3);
            out.write(reinterpret_cast<char const*>(*n), sizeof(float) * 3);
        }
        else
        {
            out << v[0] << " " << v[1] << " " << v[2] << " "
                << n[0] << " " << n[1] << " " << n[2] << std::endl;
        }
    }
    out.close();
}

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END
