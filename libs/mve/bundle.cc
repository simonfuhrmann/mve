/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "mve/bundle.h"

MVE_NAMESPACE_BEGIN

bool
Bundle::Feature3D::contains_view_id (int id) const
{
    for (std::size_t i = 0; i < this->refs.size(); ++i)
        if (this->refs[i].view_id == id)
            return true;
    return false;
}

/* -------------------------------------------------------------- */

std::size_t
Bundle::get_byte_size (void) const
{
    std::size_t ret = 0;
    ret += this->cameras.capacity() * sizeof(CameraInfo);
    ret += this->features.capacity() * sizeof(Feature3D);
    for (std::size_t i = 0; i < this->features.size(); ++i)
        ret += this->features[i].refs.capacity() * sizeof(Feature2D);
    return ret;
}

/* -------------------------------------------------------------- */

std::size_t
Bundle::get_num_cameras (void) const
{
    return this->cameras.size();
}

/* -------------------------------------------------------------- */

std::size_t
Bundle::get_num_valid_cameras (void) const
{
    std::size_t ret = 0;
    for (std::size_t i = 0; i < this->cameras.size(); ++i)
        ret += (this->cameras[i].flen != 0.0f ? 1 : 0);
    return ret;
}

/* -------------------------------------------------------------- */

TriangleMesh::Ptr
Bundle::get_features_as_mesh (void) const
{
    TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    TriangleMesh::VertexList& verts = mesh->get_vertices();
    TriangleMesh::ColorList& colors = mesh->get_vertex_colors();
    for (std::size_t i = 0; i < this->features.size(); ++i)
    {
        Bundle::Feature3D const& f = this->features[i];
        verts.push_back(math::Vec3f(f.pos));
        colors.push_back(math::Vec4f(f.color[0], f.color[1], f.color[2], 1.0f));
    }
    return mesh;
}

/* -------------------------------------------------------------- */

void
Bundle::delete_camera (std::size_t index)
{
    if (index >= this->cameras.size())
        throw std::invalid_argument("Invalid camera index");

    /* Mark the deleted camera as invalid. */
    this->cameras[index].flen = 0.0f;

    /* Delete all SIFT features that are visible in this camera. */
    for (std::size_t i = 0; i < this->features.size(); ++i)
    {
        Feature3D& feature = this->features[i];
        typedef std::vector<Feature2D> FeatureRefs;
        FeatureRefs& refs = feature.refs;

        for (FeatureRefs::iterator iter = refs.begin(); iter != refs.end();)
            if (iter->view_id == static_cast<int>(index))
                iter = refs.erase(iter);
            else
                iter++;
    }
}

MVE_NAMESPACE_END
