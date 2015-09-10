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
#include <stdexcept>
#include <cerrno>
#include <cstring>

#include "util/exception.h"
#include "mve/mesh_io_pbrt.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

void
save_pbrt_mesh (TriangleMesh::ConstPtr mesh, std::string const& filename)
{
    if (mesh == nullptr)
        throw std::invalid_argument("Null mesh given");
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    mve::TriangleMesh::VertexList const& verts(mesh->get_vertices());
    mve::TriangleMesh::NormalList const& vnormals(mesh->get_vertex_normals());
    mve::TriangleMesh::FaceList const& faces(mesh->get_faces());

    /* Open output file. */
    std::ofstream out(filename.c_str(), std::ios::binary);
    if (!out.good())
        throw util::FileException(filename, std::strerror(errno));

    out << "Translate 0 0 0" << std::endl;
    out << "Shape \"trianglemesh\"" << std::endl;

    /* Issue vertices. */
    out << "\"point P\" [" << std::endl;
    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        out << "  "
            << verts[i][0] << " "
            << verts[i][1] << " "
            << verts[i][2] << std::endl;
    }
    out << "]" << std::endl << std::endl;

    /* Issue normals. */
    if (vnormals.size() == verts.size())
    {
        out << "\"normal N\" [" << std::endl;
        for (std::size_t i = 0; i < vnormals.size(); ++i)
        {
            out << "  "
                << vnormals[i][0] << " "
                << vnormals[i][1] << " "
                << vnormals[i][2] << std::endl;
        }
        out << "]" << std::endl << std::endl;
    }

    /* Issue face indices. */
    out << "\"integer indices\" [" << std::endl;
    for (std::size_t i = 0 ; i < faces.size() ; ++i)
    {
        if (!(i % 3))
            out << "  ";
        else
            out << " ";
        out << faces[i];
        if ((i % 3) == 2)
            out << std::endl;
    }
    out << "]" << std::endl;

    /* Close output file. */
    out.close();
}


MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END
