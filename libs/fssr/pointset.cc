/*
 * This file is part of the Floating Scale Surface Reconstruction software.
 * Written by Simon Fuhrmann.
 */

#include <iostream>

#include "mve/mesh_io_ply.h"
#include "fssr/pointset.h"

FSSR_NAMESPACE_BEGIN

void
PointSet::read_from_file (std::string const& filename)
{
    /* Load or generate point set. */
    mve::TriangleMesh::Ptr mesh = mve::geom::load_ply_mesh(filename);

    mve::TriangleMesh::VertexList const& verts = mesh->get_vertices();
    if (verts.empty())
        throw std::invalid_argument("Point set is empty!");

    mve::TriangleMesh::NormalList const& vnormals = mesh->get_vertex_normals();
    if (!mesh->has_vertex_normals())
        throw std::invalid_argument("Vertex normals missing!");

    mve::TriangleMesh::ValueList const& vvalues = mesh->get_vertex_values();
    if (!mesh->has_vertex_values())
        throw std::invalid_argument("Vertex scale missing!");

    mve::TriangleMesh::ConfidenceList& vconfs = mesh->get_vertex_confidences();
    if (!mesh->has_vertex_confidences())
        vconfs.resize(verts.size(), 1.0f);

    mve::TriangleMesh::ColorList& vcolors = mesh->get_vertex_colors();
    if (!mesh->has_vertex_colors())
        vcolors.resize(verts.size(), math::Vec4f(-1.0f));

    std::size_t num_skipped_zero_normal = 0;
    std::size_t num_unnormalized_normals = 0;
    std::size_t num_skipped_invalid_confidence = 0;
    std::size_t num_skipped_invalid_scale = 0;
    this->samples.reserve(verts.size());
    for (std::size_t i = 0; i < verts.size(); i += (1 + this->num_skip))
    {
        Sample s;
        s.pos = verts[i];
        s.normal = vnormals[i];
        s.scale = vvalues[i] * this->scale_factor;
        s.confidence = vconfs[i];
        s.color = math::Vec3f(*vcolors[i]);

        if (s.scale <= 0.0f)
        {
            num_skipped_invalid_scale += 1;
            continue;
        }

        if (s.confidence <= 0.0f)
        {
            num_skipped_invalid_confidence += 1;
            continue;
        }

        if (s.normal.square_norm() == 0.0f)
        {
            num_skipped_zero_normal += 1;
            continue;
        }

        if (!MATH_EPSILON_EQ(1.0f, s.normal.square_norm(), 1e-5f))
        {
            s.normal.normalize();
            num_unnormalized_normals += 1;
        }

        this->samples.push_back(s);
    }

    if (num_skipped_invalid_scale > 0)
    {
        std::cout << "WARNING: Skipped " << num_skipped_invalid_scale
            << " samples with invalid scale." << std::endl;
    }
    if (num_skipped_invalid_confidence > 0)
    {
        std::cout << "WARNING: Skipped " << num_skipped_invalid_confidence
            << " samples with invalid confidence." << std::endl;
    }
    if (num_skipped_zero_normal > 0)
    {
        std::cout << "WARNING: Skipped " << num_skipped_zero_normal
            << " samples with zero-length normal." << std::endl;
    }
    if (num_unnormalized_normals > 0)
    {
        std::cout << "WARNING: Normalized " << num_unnormalized_normals
            << " normals with non-unit length." << std::endl;
    }
}

FSSR_NAMESPACE_END
