/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cstring>
#include <cerrno>

#include "math/vector.h"
#include "util/exception.h"
#include "mve/mesh_io_off.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

TriangleMesh::Ptr
load_off_mesh (std::string const& filename)
{
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    /* Open file. */
    std::fstream input(filename.c_str());
    if (input.fail())
        throw util::FileException(filename, std::strerror(errno));

    /* Start parsing. */
    std::string buffer;
    bool parse_normals = false;

    /* Read "OFF" file signature. */
    input >> buffer;
    if (buffer == "NOFF")
    {
        parse_normals = true;
    }
    else if (buffer != "OFF")
    {
        input.close();
        throw util::Exception("File not recognized as OFF model");
    }

    /* Create a new triangle mesh. */
    TriangleMesh::Ptr mesh = TriangleMesh::create();
    TriangleMesh::VertexList& vertices = mesh->get_vertices();
    TriangleMesh::FaceList& faces = mesh->get_faces();
    TriangleMesh::NormalList& vertex_normals = mesh->get_vertex_normals();

    /* Clear the model data and init some values. */
    std::size_t num_vertices = 0;
    std::size_t num_faces = 0;
    std::size_t num_edges = 0;

    /* Read vertex, face and edge information. */
    input >> num_vertices >> num_faces >> num_edges;

    vertices.reserve(num_vertices);
    faces.reserve(num_faces * 3);
    vertex_normals.reserve(num_vertices);

    /* Read vertices. */
    for (std::size_t i = 0; i < num_vertices; ++i)
    {
        float x, y, z;
        input >> x >> y >> z;
        vertices.push_back(math::Vec3f(x, y, z));

        /* Also read vertex normals if present. */
        if (parse_normals)
        {
            input >> x >> y >> z;
            vertex_normals.push_back(math::Vec3f(x, y, z));
        }
    }

    /* Read faces. */
    for (std::size_t i = 0; i < num_faces; ++i)
    {
        std::size_t n_vertices;
        input >> n_vertices;

        if (n_vertices == 3)
        {
            /* Polygon is a triangle. */
            unsigned int vidx[3];
            bool indices_good = true;
            for (int j = 0; j < 3; ++j)
            {
                input >> vidx[j];
                if (vidx[j] >= num_vertices)
                {
                    std::cout << "OFF Loader: Warning: Face " << i
                        << " has invalid vertex " << vidx[j]
                        << ", skipping face." << std::endl;
                    indices_good = false;
                }
            }

            if (indices_good)
                for (int j = 0; j < 3; ++j)
                    faces.push_back(vidx[j]);
        }
        else if (n_vertices == 4)
        {
            /* Polygon is a quad and converted to 2 triangles. */
            unsigned int vidx[4];
            bool indices_good = true;
            for (int j = 0; j < 4; ++j)
            {
                input >> vidx[j];
                if (vidx[j] >= num_vertices)
                {
                    std::cout << "OFF Loader: Warning: Face " << i
                        << " has invalid vertex " << vidx[j]
                        << ", skipping face." << std::endl;
                    indices_good = false;
                }
            }

            if (indices_good)
            {
                for (int j = 0; j < 3; ++j)
                    faces.push_back(vidx[j]);
                for (int j = 0; j < 3; ++j)
                    faces.push_back(vidx[(j + 2) % 4]);
            }
        }
        else
        {
            std::cout << "Warning: Line " << (2 + num_vertices + i)
                << ": Polygon with " << n_vertices << "vertices, "
                << "Skipping face!" << std::endl;

            for (std::size_t j = 0; j < n_vertices; ++j)
            {
                float tmp;
                input >> tmp;
            }
        }
    }

  /* Close file stream. */
  input.close();

  return mesh;
}

/* ---------------------------------------------------------------- */

void
save_off_mesh (TriangleMesh::ConstPtr mesh, std::string const& filename)
{
    if (mesh == nullptr)
        throw std::invalid_argument("Null mesh given");
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    /* Open file for writing. */
    std::ofstream out(filename.c_str(), std::ios::binary);
    if (out.fail())
        throw util::FileException(filename, std::strerror(errno));

    TriangleMesh::VertexList const& vertices = mesh->get_vertices();
    TriangleMesh::FaceList const& faces = mesh->get_faces();
    std::size_t num_verts = vertices.size();
    std::size_t num_faces = faces.size() / 3;

    /* Write the header. */
    out << "OFF" << std::endl;

    /* Write number of vertices and faces (and 0 for edges). */
    out << num_verts << " " << num_faces << " 0" << std::endl;

    /* Write at least 7 digits for float values. */
    out << std::fixed << std::setprecision(7);

    /* Write vertices. */
    for (std::size_t i = 0; i < num_verts; ++i)
    {
        out << vertices[i][0] << " " << vertices[i][1]
            << " " << vertices[i][2] << std::endl;
    }

    /* Write faces. */
    for (std::size_t i = 0; i < num_faces * 3; i += 3)
    {
        out << "3 " << faces[i + 0] << " "
            << faces[i + 1] << " " << faces[i + 2] << std::endl;
    }

    /* Close file stream. */
    out.close();
}

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END
