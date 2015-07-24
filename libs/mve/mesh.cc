/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>

#include "math/defines.h"
#include "math/functions.h"
#include "mve/mesh.h"

/*
 * Whether to use AWPN (angle-weighted pseudo normals)
 * or AWFN (area-weighted face normals).
 */
#define MESH_AWPN_NORMALS 1

MVE_NAMESPACE_BEGIN

void
TriangleMesh::recalc_normals (bool face, bool vertex)
{
    if (!face && !vertex)
        return;

    if (face)
    {
        this->face_normals.clear();
        this->face_normals.reserve(this->faces.size() / 3);
    }

    if (vertex)
    {
        this->vertex_normals.clear();
        this->vertex_normals.resize(this->vertices.size(), math::Vec3f(0.0f));
    }

    std::size_t zlfn = 0;
    std::size_t zlvn = 0;

    for (std::size_t i = 0; i < this->faces.size(); i += 3)
    {
        /* Face vertex indices. */
        std::size_t ia = this->faces[i + 0];
        std::size_t ib = this->faces[i + 1];
        std::size_t ic = this->faces[i + 2];

        /* Face vertices. */
        math::Vec3f const& a = this->vertices[ia];
        math::Vec3f const& b = this->vertices[ib];
        math::Vec3f const& c = this->vertices[ic];

        /* Face edges. */
        math::Vec3f ab = b - a;
        math::Vec3f bc = c - b;
        math::Vec3f ca = a - c;

        /* Face normal. */
        math::Vec3f fn = ab.cross(-ca);
        float fnl = fn.norm();

        /* Count zero-length face normals. */
        if (fnl == 0.0f)
            zlfn += 1;

#if MESH_AWPN_NORMALS

        /*
         * Calculate angle weighted pseudo normals by weighted
         * averaging adjacent face normals.
         */

        /* Normalize face normal. */
        if (fnl != 0.0f)
            fn /= fnl;

        /* Add (normalized) face normal. */
        if (face)
            this->face_normals.push_back(fn);

        /* Update adjacent vertex normals. */
        if (fnl != 0.0f && vertex)
        {
            float abl = ab.norm();
            float bcl = bc.norm();
            float cal = ca.norm();

            /*
             * Although (a.dot(b) / (alen * blen)) is more efficient,
             * (a / alen).dot(b / blen) is numerically more stable.
             */
            float ratio1 = (ab / abl).dot(-ca / cal);
            float ratio2 = (-ab / abl).dot(bc / bcl);
            float ratio3 = (ca / cal).dot(-bc / bcl);
            float angle1 = std::acos(math::clamp(ratio1, -1.0f, 1.0f));
            float angle2 = std::acos(math::clamp(ratio2, -1.0f, 1.0f));
            float angle3 = std::acos(math::clamp(ratio3, -1.0f, 1.0f));

            //float angle1 = std::acos(ab.dot(-ca) / (abl * cal));
            //float angle2 = std::acos((-ab).dot(bc) / (abl * bcl));
            //float angle3 = std::acos(ca.dot(-bc) / (cal * bcl));

            this->vertex_normals[ia] += fn * angle1;
            this->vertex_normals[ib] += fn * angle2;
            this->vertex_normals[ic] += fn * angle3;

            if (MATH_ISNAN(angle1) || MATH_ISNAN(angle2) || MATH_ISNAN(angle3))
            {
                std::cout << "NAN error in " << __FILE__ << ":" << __LINE__
                    << ": " << angle1 << " / " << angle2 << " / " << angle3
                    << " (" << abl << " / " << bcl << " / " << cal << ")"
                    << " [" << ratio1 << " / " << ratio2 << " / " << ratio3
                    << "]" << std::endl;
            }
        }

#else /* no MESH_AWPN_NORMALS */

        /*
         * Calculate simple vertex normals by averaging
         * area-weighted adjacent face normals.
         */

        if (face)
        {
            this->face_normals.push_back(fnl ? fn / fnl : fn);
        }

        if (fnl != 0.0f && vertex)
        {
            this->vertex_normals[ia] += fn;
            this->vertex_normals[ib] += fn;
            this->vertex_normals[ic] += fn;
        }

#endif /* MESH_AWPN_NORMALS */

    }

    /* Normalize all vertex normals. */
    if (vertex)
    {
        for (std::size_t i = 0; i < this->vertex_normals.size(); ++i)
        {
            float vnl = this->vertex_normals[i].norm();
            if (vnl > 0.0f)
                this->vertex_normals[i] /= vnl;
            else
                zlvn += 1;
        }
    }

    if (zlfn || zlvn)
        std::cout << "Warning: Zero-length normals detected: "
            << zlfn << " face normals, " << zlvn << " vertex normals"
            << std::endl;
}

/* ---------------------------------------------------------------- */

void
TriangleMesh::ensure_normals (bool face, bool vertex)
{
    bool need_face_recalc = (face
        && this->face_normals.size() != this->faces.size() / 3);
    bool need_vertex_recalc = (vertex
        && this->vertex_normals.size() != this->vertices.size());
    this->recalc_normals(need_face_recalc, need_vertex_recalc);
}

/* ---------------------------------------------------------------- */

void
TriangleMesh::delete_vertices (DeleteList const& delete_list)
{
    if (delete_list.size() != this->vertices.size())
        throw std::invalid_argument("Delete list does not match vertex list");

    if (this->has_vertex_normals())
        math::algo::vector_clean(delete_list, &this->vertex_normals);
    if (this->has_vertex_colors())
        math::algo::vector_clean(delete_list, &this->vertex_colors);
    if (this->has_vertex_confidences())
        math::algo::vector_clean(delete_list, &this->vertex_confidences);
    if (this->has_vertex_values())
        math::algo::vector_clean(delete_list, &this->vertex_values);
    if (this->has_vertex_texcoords())
        math::algo::vector_clean(delete_list, &this->vertex_texcoords);
    math::algo::vector_clean(delete_list, &this->vertices);
}

/* ---------------------------------------------------------------- */

void
TriangleMesh::delete_vertices_fix_faces (DeleteList const& dlist)
{
    if (dlist.size() != this->vertices.size())
        throw std::invalid_argument("Delete list does not match vertex list");

    /* Each vertex is shifted to the left. This list tracks the distance. */
    std::vector<std::size_t> idxshift(this->vertices.size(), 0);
    std::size_t num_deleted = 0;
    for (std::size_t i = 0; i < this->vertices.size(); ++i)
    {
        idxshift[i] = num_deleted;
        if (dlist[i] == true)
            num_deleted += 1;
    }

    /* Invalidate faces referencing deleted vertices and fix vertex IDs. */
    for (std::size_t i = 0; i < this->faces.size(); i += 3)
    {
        if (dlist[faces[i + 0]] || dlist[faces[i + 1]] || dlist[faces[i + 2]])
        {
            faces[i + 0] = 0;
            faces[i + 1] = 0;
            faces[i + 2] = 0;
        }
        else
        {
            faces[i + 0] -= idxshift[faces[i + 0]];
            faces[i + 1] -= idxshift[faces[i + 1]];
            faces[i + 2] -= idxshift[faces[i + 2]];
        }
    }

    /* Compact vertex and attribute vectors, remove invalid faces. */
    this->delete_vertices(dlist);
    this->delete_invalid_faces();
}

/* ---------------------------------------------------------------- */

namespace
{
    bool
    is_valid_triangle (TriangleMesh::VertexID const* ids)
    {
        return ids[0] != ids[1] || ids[0] != ids[2];
    }
}

void
TriangleMesh::delete_invalid_faces (void)
{
    /* Valid and invalid iterator. */
    std::size_t ii = 0;
    std::size_t vi = this->faces.size();
    while (vi > ii)
    {
        /* Search the next invalid face. */
        while (ii < this->faces.size() && is_valid_triangle(&this->faces[ii]))
            ii += 3;
        /* Search the last valid face. */
        vi -= 3;
        while (vi > ii && !is_valid_triangle(&this->faces[vi]))
            vi -= 3;
        if (ii >= vi)
            break;
        /* Swap valid and invalid face. */
        std::swap(this->faces[vi + 0], this->faces[ii + 0]);
        std::swap(this->faces[vi + 1], this->faces[ii + 1]);
        std::swap(this->faces[vi + 2], this->faces[ii + 2]);
    }
    this->faces.resize(ii);
}

/* ---------------------------------------------------------------- */

std::size_t
TriangleMesh::get_byte_size (void) const
{
    std::size_t s_verts = this->vertices.capacity() * sizeof(math::Vec3f);
    std::size_t s_faces = this->faces.capacity() * sizeof(VertexID);
    std::size_t s_vnorm = this->vertex_normals.capacity() * sizeof(math::Vec3f);
    std::size_t s_fnorm = this->face_normals.capacity() * sizeof(math::Vec3f);
    std::size_t s_color = this->vertex_colors.capacity() * sizeof(math::Vec3f);
    return s_verts + s_faces + s_vnorm + s_fnorm + s_color;
}

MVE_NAMESPACE_END
