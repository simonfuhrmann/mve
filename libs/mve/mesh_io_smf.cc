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
#include <sstream>
#include <cerrno>
#include <cstring>

#include "util/exception.h"
#include "mve/mesh_io_smf.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

TriangleMesh::Ptr
load_smf_mesh (std::string const& filename)
{
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    TriangleMesh::Ptr mesh = TriangleMesh::create();
    TriangleMesh::VertexList& verts(mesh->get_vertices());
    TriangleMesh::FaceList& faces(mesh->get_faces());

    std::ifstream in(filename.c_str());
    if (!in.good())
        throw util::FileException(filename, std::strerror(errno));

    while (!in.eof())
    {
        char row_type;
        in >> row_type;
        if (in.eof())
            break;

        switch (row_type)
        {
            case 'v':
            {
                math::Vec3f v;
                in >> v[0] >> v[1] >> v[2];
                verts.push_back(v);
                break;
            }

            case 'f':
            {
                unsigned int vid;
                in >> vid;
                faces.push_back(vid - 1);
                in >> vid;
                faces.push_back(vid - 1);
                in >> vid;
                faces.push_back(vid - 1);
                break;
            }

            default:
                std::cerr << "Unknown element type '" << row_type
                    << "'. Skipping." << std::endl;
                break;
        }
    }
    in.close();

    return mesh;
}

void
save_smf_mesh (mve::TriangleMesh::ConstPtr mesh, std::string const& filename)
{
    if (mesh == nullptr)
        throw std::invalid_argument("Null mesh given");
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    TriangleMesh::VertexList const& verts(mesh->get_vertices());
    TriangleMesh::FaceList const& faces(mesh->get_faces());

    /* Open output file. */
    std::ofstream out(filename.c_str(), std::ios::binary);
    if (!out.good())
        throw util::FileException(filename, std::strerror(errno));

    std::cout << "Writing SMF: " << verts.size() << " verts..." << std::flush;
    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        math::Vec3f const& v = verts[i];
        out << "v " << v[0] << " " << v[1] << " " << v[2] << "\n";
    }

    std::cout << " " << (faces.size() / 3) << " faces..." << std::flush;
    for (std::size_t i = 0; i < faces.size(); i += 3)
    {
        out << "f " << faces[i + 0] + 1 << " "
            << faces[i + 1] + 1  << " "
            << faces[i + 2] + 1  << "\n";
    }
    std::cout << " done." << std::endl;

    out.close();
}

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END
