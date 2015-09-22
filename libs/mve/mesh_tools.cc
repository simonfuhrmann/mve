/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <vector>
#include <stdexcept>
#include <algorithm>
#include <list>
#include <limits>
#include <fstream>
#include <cerrno>
#include <cstring>

#include "math/algo.h"
#include "math/vector.h"
#include "mve/mesh_info.h"
#include "mve/mesh_tools.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

/** for-each functor: homogenous matrix-vector multiplication. */
template <typename T, int D>
struct foreach_hmatrix_mult
{
    typedef math::Matrix<T,D,D> MatrixType;
    typedef math::Vector<T,D-1> VectorType;
    typedef T ValueType;

    MatrixType mat;
    ValueType val;

    foreach_hmatrix_mult (MatrixType const& matrix, ValueType const& value)
        : mat(matrix), val(value) {}
    void operator() (VectorType& vec) { vec = mat.mult(vec, val); }
};

/* ---------------------------------------------------------------- */

void
mesh_transform (mve::TriangleMesh::Ptr mesh, math::Matrix3f const& rot)
{
    if (mesh == nullptr)
        throw std::invalid_argument("Null mesh given");

    mve::TriangleMesh::VertexList& verts(mesh->get_vertices());
    mve::TriangleMesh::NormalList& vnorm(mesh->get_vertex_normals());
    mve::TriangleMesh::NormalList& fnorm(mesh->get_face_normals());

    math::algo::foreach_matrix_mult<math::Matrix3f, math::Vec3f> func(rot);
    std::for_each(verts.begin(), verts.end(), func);
    std::for_each(fnorm.begin(), fnorm.end(), func);
    std::for_each(vnorm.begin(), vnorm.end(), func);
}

/* ---------------------------------------------------------------- */

void
mesh_transform (TriangleMesh::Ptr mesh, math::Matrix4f const& trans)
{
    if (mesh == nullptr)
        throw std::invalid_argument("Null mesh given");

    mve::TriangleMesh::VertexList& verts(mesh->get_vertices());
    mve::TriangleMesh::NormalList& vnorm(mesh->get_vertex_normals());
    mve::TriangleMesh::NormalList& fnorm(mesh->get_face_normals());

    foreach_hmatrix_mult<float, 4> vfunc(trans, 1.0f);
    foreach_hmatrix_mult<float, 4> nfunc(trans, 0.0f);
    std::for_each(verts.begin(), verts.end(), vfunc);
    std::for_each(fnorm.begin(), fnorm.end(), nfunc);
    std::for_each(vnorm.begin(), vnorm.end(), nfunc);
}

/* ---------------------------------------------------------------- */
// FIXME: Find a way to abstract from the mesh attributes.

void
mesh_merge (TriangleMesh::ConstPtr mesh1, TriangleMesh::Ptr mesh2)
{
    mve::TriangleMesh::VertexList const& verts1 = mesh1->get_vertices();
    mve::TriangleMesh::VertexList& verts2 = mesh2->get_vertices();
    mve::TriangleMesh::ColorList const& color1 = mesh1->get_vertex_colors();
    mve::TriangleMesh::ColorList& color2 = mesh2->get_vertex_colors();
    mve::TriangleMesh::ConfidenceList const& confs1 = mesh1->get_vertex_confidences();
    mve::TriangleMesh::ConfidenceList& confs2 = mesh2->get_vertex_confidences();
    mve::TriangleMesh::ValueList const& values1 = mesh1->get_vertex_values();
    mve::TriangleMesh::ValueList& values2 = mesh2->get_vertex_values();
    mve::TriangleMesh::NormalList const& vnorm1 = mesh1->get_vertex_normals();
    mve::TriangleMesh::NormalList& vnorm2 = mesh2->get_vertex_normals();
    mve::TriangleMesh::TexCoordList const& vtex1 = mesh1->get_vertex_texcoords();
    mve::TriangleMesh::TexCoordList& vtex2 = mesh2->get_vertex_texcoords();
    mve::TriangleMesh::NormalList const& fnorm1 = mesh1->get_face_normals();
    mve::TriangleMesh::NormalList& fnorm2 = mesh2->get_face_normals();
    mve::TriangleMesh::FaceList const& faces1 = mesh1->get_faces();
    mve::TriangleMesh::FaceList& faces2 = mesh2->get_faces();

    verts2.reserve(verts1.size() + verts2.size());
    color2.reserve(color1.size() + color2.size());
    confs2.reserve(confs1.size() + confs2.size());
    values2.reserve(values1.size() + values2.size());
    vnorm2.reserve(vnorm1.size() + vnorm2.size());
    vtex2.reserve(vtex1.size() + vtex2.size());
    fnorm2.reserve(fnorm1.size() + fnorm2.size());
    faces2.reserve(faces1.size() + faces2.size());

    std::size_t const offset = verts2.size();
    verts2.insert(verts2.end(), verts1.begin(), verts1.end());
    color2.insert(color2.end(), color1.begin(), color1.end());
    confs2.insert(confs2.end(), confs1.begin(), confs1.end());
    values2.insert(values2.end(), values1.begin(), values1.end());
    vnorm2.insert(vnorm2.end(), vnorm1.begin(), vnorm1.end());
    vtex2.insert(vtex2.end(), vtex1.begin(), vtex1.end());
    fnorm2.insert(fnorm2.end(), fnorm1.begin(), fnorm1.end());

    for (std::size_t i = 0; i < faces1.size(); ++i)
        faces2.push_back(faces1[i] + offset);
}

/* ---------------------------------------------------------------- */

void
mesh_components (TriangleMesh::Ptr mesh, std::size_t vertex_threshold)
{
    MeshInfo mesh_info(mesh);
    std::size_t const num_vertices = mesh->get_vertices().size();
    std::vector<int> component_per_vertex(num_vertices, -1);
    int current_component = 0;
    for (std::size_t i = 0; i < num_vertices; ++i)
    {
        /* Start with a vertex that has no component yet. */
        if (component_per_vertex[i] >= 0)
            continue;

        /* Starting from this face, collect faces and assign component ID. */
        std::list<std::size_t> queue;
        queue.push_back(i);
        while (!queue.empty())
        {
            std::size_t vid = queue.front();
            queue.pop_front();

            /* Skip vertices with a componenet already assigned. */
            if (component_per_vertex[vid] >= 0)
                continue;
            component_per_vertex[vid] = current_component;

            /* Add all adjacent vertices to queue. */
            MeshInfo::AdjacentVertices const& adj_verts = mesh_info[vid].verts;
            queue.insert(queue.end(), adj_verts.begin(), adj_verts.end());
        }
        current_component += 1;
    }
    mesh_info.clear();

    /* Create a list of components and count vertices per component. */
    std::vector<std::size_t> components_size(current_component, 0);
    for (std::size_t i = 0; i < component_per_vertex.size(); ++i)
        components_size[component_per_vertex[i]] += 1;

    /* Mark vertices to be deleted if part of a small component. */
    TriangleMesh::DeleteList delete_list(num_vertices, false);
    for (std::size_t i = 0; i < component_per_vertex.size(); ++i)
        if (components_size[component_per_vertex[i]] <= vertex_threshold)
            delete_list[i] = true;

    /* Delete vertices and faces indexing deleted vertices. */
    mesh->delete_vertices_fix_faces(delete_list);
}

/* ---------------------------------------------------------------- */

void
mesh_scale_and_center (TriangleMesh::Ptr mesh, bool scale, bool center)
{
    if (mesh == nullptr)
        throw std::invalid_argument("Null mesh given");

    TriangleMesh::VertexList& verts(mesh->get_vertices());

    if (verts.empty() || (!scale && !center))
        return;

    /* Calculate the AABB of the model. */
    math::Vec3f min(std::numeric_limits<float>::max());
    math::Vec3f max(-std::numeric_limits<float>::max());
    for (std::size_t i = 0; i < verts.size(); ++i)
        for (std::size_t j = 0; j < 3; ++j)
        {
            min[j] = std::min(min[j], verts[i][j]);
            max[j] = std::max(max[j], verts[i][j]);
        }

    math::Vec3f move((min + max) / 2.0);

    /* Prepare scaling of model to fit in the einheits cube. */
    math::Vec3f sizes(max - min);
    float max_xyz = sizes.maximum();

    /* Translate and scale. */
    if (scale && center)
    {
        for (std::size_t i = 0; i < verts.size(); ++i)
            verts[i] = (verts[i] - move) / max_xyz;
    }
    else if (scale && !center)
    {
        for (std::size_t i = 0; i < verts.size(); ++i)
            verts[i] /= max_xyz;
    }
    else if (!scale && center)
    {
        for (std::size_t i = 0; i < verts.size(); ++i)
            verts[i] -= move;
    }
}

/* ---------------------------------------------------------------- */

void
mesh_invert_faces (TriangleMesh::Ptr mesh)
{
    if (mesh == nullptr)
        throw std::invalid_argument("Null mesh given");

    TriangleMesh::FaceList& faces(mesh->get_faces());

    for (std::size_t i = 0; i < faces.size(); i += 3)
        std::swap(faces[i + 1], faces[i + 2]);
    mesh->recalc_normals(true, true);
}

/* ---------------------------------------------------------------- */

void
mesh_find_aabb (TriangleMesh::ConstPtr mesh,
    math::Vec3f& aabb_min, math::Vec3f& aabb_max)
{
    if (mesh == nullptr)
        throw std::invalid_argument("Null mesh given");

    TriangleMesh::VertexList const& verts(mesh->get_vertices());
    if (verts.empty())
        throw std::invalid_argument("Mesh without vertices given");

    aabb_min = math::Vec3f(std::numeric_limits<float>::max());
    aabb_max = math::Vec3f(-std::numeric_limits<float>::max());
    for (std::size_t i = 0; i < verts.size(); ++i)
        for (int j = 0; j < 3; ++j)
        {
            if (verts[i][j] < aabb_min[j])
                aabb_min[j] = verts[i][j];
            if (verts[i][j] > aabb_max[j])
                aabb_max[j] = verts[i][j];
        }
}

/* ---------------------------------------------------------------- */

std::size_t
mesh_delete_unreferenced (TriangleMesh::Ptr mesh)
{
    if (mesh == nullptr)
        throw std::invalid_argument("Null mesh given");

    MeshInfo mesh_info(mesh);
    TriangleMesh::DeleteList dlist(mesh_info.size(), false);
    std::size_t num_deleted = 0;
    for (std::size_t i = 0; i < mesh_info.size(); ++i)
    {
        if (mesh_info[i].vclass == MeshInfo::VERTEX_CLASS_UNREF)
        {
            dlist[i] = true;
            num_deleted += 1;
        }
    }

    mesh->delete_vertices_fix_faces(dlist);
    return num_deleted;
}

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END
