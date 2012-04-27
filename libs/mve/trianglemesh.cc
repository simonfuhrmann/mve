#include <iostream>

#include "math/defines.h"

#include "trianglemesh.h"

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
            float angle1 = std::acos(math::algo::clamp(ratio1, -1.0f, 1.0f));
            float angle2 = std::acos(math::algo::clamp(ratio2, -1.0f, 1.0f));
            float angle3 = std::acos(math::algo::clamp(ratio3, -1.0f, 1.0f));

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
TriangleMesh::delete_vertices (DeleteList const& dlist)
{
    if (this->vertex_normals.size() == this->vertices.size())
        math::algo::vector_clean(this->vertex_normals, dlist);
    if (this->vertex_colors.size() == this->vertices.size())
        math::algo::vector_clean(this->vertex_colors, dlist);
    math::algo::vector_clean(this->vertices, dlist);
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
