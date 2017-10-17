/*
 * Copyright (C) 2015, Simon Fuhrmann, Nils Moehrle
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <fstream>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <map>

#include "util/strings.h"
#include "util/tokenizer.h"
#include "util/exception.h"
#include "util/file_system.h"
#include "mve/mesh_io_obj.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

namespace
{
    struct ObjVertex
    {
        unsigned int vertex_id;
        unsigned int texcoord_id;
        unsigned int normal_id;

        ObjVertex (void);
        bool operator< (ObjVertex const & other) const;
    };

    inline
    ObjVertex::ObjVertex (void)
        : vertex_id(0)
        , texcoord_id(0)
        , normal_id(0)
    {
    }

    inline bool
    ObjVertex::operator< (ObjVertex const & other) const
    {
        return std::lexicographical_compare(&vertex_id, &normal_id + 1,
            &other.vertex_id, &other.normal_id + 1);
    }
}

void
load_mtl_file (std::string const& filename,
    std::map<std::string, std::string>* result)
{
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    /* Open file. */
    std::ifstream input(filename.c_str(), std::ios::binary);
    if (!input.good())
        throw util::FileException(filename, std::strerror(errno));

    std::string material_name;
    std::string buffer;
    while (input.good())
    {
        std::getline(input, buffer);
        util::string::clip_newlines(&buffer);
        util::string::clip_whitespaces(&buffer);

        if (buffer.empty())
            continue;
        if (input.eof())
            break;

        if (buffer[0] == '#')
        {
            std::cout << "MTL Loader: " << buffer << std::endl;
            continue;
        }

        util::Tokenizer line;
        line.split(buffer);

        if (line[0] == "newmtl")
        {
            if (line.size() != 2)
                throw util::Exception("Invalid material specification");

            material_name = line[1];
        }
        else if (line[0] == "map_Kd")
        {
            if (line.size() != 2)
                throw util::Exception("Invalid diffuse map specification");
            if (material_name.empty())
                throw util::Exception("Unbound material property");

            std::string path = util::fs::join_path
                (util::fs::dirname(filename), line[1]);
            result->insert(std::make_pair(material_name, path));
            material_name.clear();
        }
        else
        {
            std::cout << "MTL Loader: Skipping unimplemented material property "
                << line[0] << std::endl;
        }
    }

    /* Close the file stream. */
    input.close();
}

mve::TriangleMesh::Ptr
load_obj_mesh (std::string const& filename)
{
    std::vector<ObjModelPart> obj_model_parts;
    load_obj_mesh(filename, &obj_model_parts);

    if (obj_model_parts.size() > 1)
        throw util::Exception("OBJ file contains multiple parts");

    return obj_model_parts[0].mesh;
}

void
load_obj_mesh (std::string const& filename,
    std::vector<ObjModelPart>* obj_model_parts)
{
    /* Precondition checks. */
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    /* Open file. */
    std::ifstream input(filename.c_str(), std::ios::binary);
    if (!input.good())
        throw util::FileException(filename, std::strerror(errno));

    std::vector<math::Vec3f> global_vertices;
    std::vector<math::Vec3f> global_normals;
    std::vector<math::Vec2f> global_texcoords;
    std::map<std::string, std::string> materials;

    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& vertices = mesh->get_vertices();
    mve::TriangleMesh::NormalList& normals = mesh->get_vertex_normals();
    mve::TriangleMesh::TexCoordList& texcoords = mesh->get_vertex_texcoords();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();

    typedef std::map<ObjVertex, unsigned int> VertexIndexMap;
    VertexIndexMap vertex_map;
    std::string material_name;
    std::string new_material_name;
    std::string buffer;
    while (input.good())
    {
        std::getline(input, buffer);
        util::string::clip_newlines(&buffer);
        util::string::clip_whitespaces(&buffer);

        if (input.eof() || !new_material_name.empty())
        {
            if (!texcoords.empty() && texcoords.size() != vertices.size())
                throw util::Exception("Invalid number of texture coords");
            if (!normals.empty() && normals.size() != vertices.size())
                throw util::Exception("Invalid number of vertex normals");

            if (!vertices.empty())
            {
                ObjModelPart obj_model_part;
                obj_model_part.mesh = mve::TriangleMesh::create();
                std::swap(vertices, obj_model_part.mesh->get_vertices());
                std::swap(texcoords, obj_model_part.mesh->get_vertex_texcoords());
                std::swap(normals, obj_model_part.mesh->get_vertex_normals());
                std::swap(faces, obj_model_part.mesh->get_faces());
                obj_model_part.texture_filename = materials[material_name];
                obj_model_parts->push_back(obj_model_part);
            }

            mesh->clear();
            vertex_map.clear();

            material_name.swap(new_material_name);
            new_material_name.clear();
        }

        if (buffer.empty())
            continue;
        if (input.eof())
            break;

        if (buffer[0] == '#')
        {
            /* Print all comments to STDOUT and forget data. */
            std::cout << "OBJ Loader: " << buffer << std::endl;
            continue;
        }

        util::Tokenizer line;
        line.split(buffer);

        if (line[0] == "v")
        {
            if (line.size() != 4 && line.size() != 5)
                throw util::Exception("Invalid vertex coordinate specification");

            math::Vec3f vertex(0.0f);
            for (int i = 0; i < 3; ++i)
                vertex[i] = util::string::convert<float>(line[1 + i]);

            /* Convert homogeneous coordinates. */
            if (line.size() == 5)
                vertex /= util::string::convert<float>(line[4]);

            global_vertices.push_back(vertex);
        }
        else if (line[0] == "vt")
        {
            if (line.size() != 3 && line.size() != 4)
                throw util::Exception("Invalid texture coords specification");

            math::Vec2f texcoord(0.0f);
            for (int i = 0; i < 2; ++i)
                texcoord[i] = util::string::convert<float>(line[1 + i]);

            /* Convert homogeneous coordinates. */
            if (line.size() == 4)
                texcoord /= util::string::convert<float>(line[3]);

            /* Invert y coordinate */
            texcoord[1] = 1.0f - texcoord[1];

            global_texcoords.push_back(texcoord);
        }
        else if (line[0] == "vn")
        {
            if (line.size() != 4 && line.size() != 5)
                util::Exception("Invalid vertex normal specification");

            math::Vec3f normal(0.0f);
            for (int i = 0; i < 3; ++i)
                normal[i] = util::string::convert<float>(line[1 + i]);

            /* Convert homogeneous coordinates. */
            if (line.size() == 5)
                normal /= util::string::convert<float>(line[4]);

            global_normals.push_back(normal);
        }
        else if (line[0] == "f")
        {
            if (line.size() != 4)
                throw util::Exception("Only triangles supported");

            for (int i = 0; i < 3; ++i)
            {
                util::Tokenizer tok;
                tok.split(line[1 + i], '/');

                if (tok.size() > 3)
                    throw util::Exception("Invalid face specification");

                ObjVertex v;
                v.vertex_id = util::string::convert<unsigned int>(tok[0]);
                if (tok.size() > 2 && !tok[1].empty())
                    v.texcoord_id = util::string::convert<unsigned int>(tok[1]);
                if (tok.size() == 3 && !tok[2].empty())
                    v.normal_id = util::string::convert<unsigned int>(tok[2]);

                if (v.vertex_id > global_vertices.size()
                    || v.texcoord_id > global_texcoords.size()
                    || v.normal_id > global_normals.size())
                    throw util::Exception("Invalid index in: " + buffer);

                VertexIndexMap::const_iterator iter = vertex_map.find(v);
                if (iter != vertex_map.end())
                {
                    faces.push_back(iter->second);
                }
                else
                {
                    vertices.push_back(global_vertices[v.vertex_id - 1]);
                    if (v.texcoord_id != 0)
                        texcoords.push_back(global_texcoords[v.texcoord_id - 1]);
                    if (v.normal_id != 0)
                        normals.push_back(global_normals[v.normal_id - 1]);
                    faces.push_back(vertices.size() - 1);
                    vertex_map[v] = vertices.size() - 1;
                }
            }
        }
        else if (line[0] == "usemtl")
        {
            if (line.size() != 2)
                throw util::Exception("Invalid usemtl specification");
            new_material_name = line[1];
        }
        else if (line[0] == "mtllib")
        {
            if (line.size() != 2)
                throw util::Exception("Invalid material library specification");

            std::string dir = util::fs::dirname(filename);
            load_mtl_file(util::fs::join_path(dir, line[1]), &materials);
        }
        else
        {
            std::cout << "OBJ Loader: Skipping unsupported element: "
                << line[0] << std::endl;
        }
    }

    /* Close the file stream. */
    input.close();
}

void
save_obj_mesh (TriangleMesh::ConstPtr mesh, std::string const& filename)
{
    if (mesh == nullptr)
        throw std::invalid_argument("Null mesh given");
    if (filename.empty())
        throw std::invalid_argument("No filename given");

    mve::TriangleMesh::VertexList const& verts(mesh->get_vertices());
    mve::TriangleMesh::FaceList const& faces(mesh->get_faces());

    if (faces.size() % 3 != 0)
        throw std::invalid_argument("Triangle indices not divisible by 3");

    /* Open output file. */
    std::ofstream out(filename.c_str(), std::ios::binary);
    if (!out.good())
        throw util::FileException(filename, std::strerror(errno));

    out << "# Export generated by libmve\n";
    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        out << "v " << verts[i][0] << " " << verts[i][1]
            << " "  << verts[i][2] << "\n";
    }

    for (std::size_t i = 0; i < faces.size(); i += 3)
    {
        out << "f " << (faces[i + 0] + 1)
            << " " << (faces[i + 1] + 1)
            << " " << (faces[i + 2] + 1) << "\n";
    }

    /* Close output file. */
    out.close();
}

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END
