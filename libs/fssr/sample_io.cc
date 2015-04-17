/*
 * This file is part of the Floating Scale Surface Reconstruction software.
 * Written by Simon Fuhrmann.
 */

#include <iostream>

#include "mve/mesh_io_ply.h"
#include "fssr/sample_io.h"

FSSR_NAMESPACE_BEGIN

void
SampleIO::read_file (std::string const& filename,  SampleList* samples)
{
    /* Load point set from PLY file. */
    mve::TriangleMesh::Ptr mesh = mve::geom::load_ply_mesh(filename);
    mve::TriangleMesh::VertexList const& verts = mesh->get_vertices();
    if (verts.empty())
    {
        std::cout << "WARNING: No samples in file, skipping." << std::endl;
        return;
    }

    /* Check if the points have the required attributes. */
    mve::TriangleMesh::NormalList const& vnormals = mesh->get_vertex_normals();
    if (!mesh->has_vertex_normals())
        throw std::invalid_argument("Vertex normals missing!");

    mve::TriangleMesh::ValueList const& vvalues = mesh->get_vertex_values();
    if (!mesh->has_vertex_values())
        throw std::invalid_argument("Vertex scale missing!");

    mve::TriangleMesh::ConfidenceList& vconfs = mesh->get_vertex_confidences();
    if (!mesh->has_vertex_confidences())
    {
        std::cout << "INFO: No confidences given, setting to 1." << std::endl;
        vconfs.resize(verts.size(), 1.0f);
    }
    mve::TriangleMesh::ColorList& vcolors = mesh->get_vertex_colors();
    if (!mesh->has_vertex_colors())
        vcolors.resize(verts.size(), math::Vec4f(-1.0f));

    /* Add samples to the list. */
    SamplesState state;
    this->reset_samples_state(&state);
    samples->reserve(verts.size());
    for (std::size_t i = 0; i < verts.size(); i += 1)
    {
        Sample sample;
        sample.pos = verts[i];
        sample.normal = vnormals[i];
        sample.scale = vvalues[i];
        sample.confidence = vconfs[i];
        sample.color = math::Vec3f(*vcolors[i]);

        if (this->process_sample(&sample, &state))
            samples->push_back(sample);
    }
    this->print_samples_state(&state);
}

void
SampleIO::reset_samples_state (SamplesState* state)
{
    state->num_skipped_zero_normal = 0;
    state->num_skipped_invalid_confidence = 0;
    state->num_skipped_invalid_scale = 0;
    state->num_skipped_large_scale = 0;
    state->num_unnormalized_normals = 0;
}

bool
SampleIO::process_sample (Sample* sample, SamplesState* state)
{
    /* Skip invalid samples. */
    if (sample->scale <= 0.0f)
    {
        state->num_skipped_invalid_scale += 1;
        return false;
    }
    if (sample->confidence <= 0.0f)
    {
        state->num_skipped_invalid_confidence += 1;
        return false;
    }
    if (sample->normal.square_norm() == 0.0f)
    {
        state->num_skipped_zero_normal += 1;
        return false;
    }

    /* Process sample scale if requested. */
    if (this->opts.max_scale > 0.0f && sample->scale > this->opts.max_scale)
    {
        state->num_skipped_large_scale += 1;
        return false;
    }

    if (this->opts.min_scale > 0.0f)
        sample->scale = std::max(this->opts.min_scale, sample->scale);
    sample->scale *= this->opts.scale_factor;

    /* Normalize normals. */
    if (!MATH_EPSILON_EQ(1.0f, sample->normal.square_norm(), 1e-5f))
    {
        sample->normal.normalize();
        state->num_unnormalized_normals += 1;
    }

    return true;
}

void
SampleIO::print_samples_state (SamplesState* state)
{
    if (state->num_skipped_invalid_scale > 0)
    {
        std::cout << "WARNING: Skipped "
            << state->num_skipped_invalid_scale
            << " samples with invalid scale." << std::endl;
    }
    if (state->num_skipped_invalid_confidence > 0)
    {
        std::cout << "WARNING: Skipped "
            << state->num_skipped_invalid_confidence
            << " samples with zero confidence." << std::endl;
    }
    if (state->num_skipped_zero_normal > 0)
    {
        std::cout << "WARNING: Skipped "
            << state->num_skipped_zero_normal
            << " samples with zero-length normal." << std::endl;
    }
    if (state->num_skipped_large_scale > 0)
    {
        std::cout << "WARNING: Skipped "
            << state->num_skipped_large_scale
            << " samples with too large scale." << std::endl;
    }
    if (state->num_unnormalized_normals > 0)
    {
        std::cout << "WARNING: Normalized "
            << state->num_unnormalized_normals
            << " normals with non-unit length." << std::endl;
    }
}

FSSR_NAMESPACE_END
