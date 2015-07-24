/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>
#include <cstring>
#include <cerrno>

#include "util/exception.h"
#include "util/tokenizer.h"
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
SampleIO::open_file (std::string const& filename)
{
    this->reset_stream_state();
    this->reset_samples_state(&this->samples);
    this->stream.filename = filename;
    this->stream.stream.open(filename.c_str(), std::ios::binary);
    if (!this->stream.stream.good())
        throw util::FileException(filename, std::strerror(errno));

    /* Read PLY signature. */
    std::string line;
    std::getline(this->stream.stream, line);
    if (line != "ply")
    {
        this->reset_stream_state();
        throw util::Exception("Invalid PLY signature");
    }

    /* Parse PLY headers. */
    bool parsing_vertex_props = false;
    this->stream.format = mve::geom::PLY_UNKNOWN;
    while (true)
    {
        std::getline(this->stream.stream, line);
        if (this->stream.stream.eof())
        {
            this->reset_stream_state();
            throw util::Exception("EOF while parsing headers");
        }

        if (!this->stream.stream.good())
        {
            this->reset_stream_state();
            throw util::Exception("Read error while parsing headers");
        }

        util::string::clip_newlines(&line);
        util::string::clip_whitespaces(&line);
        if (line == "end_header")
            break;

        util::Tokenizer tokens;
        tokens.split(line);
        if (tokens.empty())
            continue;

        if (tokens.size() == 3 && tokens[0] == "format")
        {
            /* Determine the format. */
            if (tokens[1] == "ascii")
                this->stream.format = mve::geom::PLY_ASCII;
            else if (tokens[1] == "binary_little_endian")
                this->stream.format = mve::geom::PLY_BINARY_LE;
            else if (tokens[1] == "binary_big_endian")
                this->stream.format = mve::geom::PLY_BINARY_BE;
            continue;
        }

        if (tokens.size() == 3 && tokens[0] == "element")
        {
            if (tokens[1] == "vertex")
            {
                parsing_vertex_props = true;
                this->stream.num_vertices
                    = util::string::convert<unsigned int>(tokens[2]);
                continue;
            }
            else
            {
                parsing_vertex_props = false;
                continue;
            }
        }

        if (parsing_vertex_props && tokens[0] == "property")
        {
            if (tokens[1] == "float" && tokens[2] == "x")
                this->stream.props.push_back(mve::geom::PLY_V_FLOAT_X);
            else if (tokens[1] == "float" && tokens[2] == "y")
                this->stream.props.push_back(mve::geom::PLY_V_FLOAT_Y);
            else if (tokens[1] == "float" && tokens[2] == "z")
                this->stream.props.push_back(mve::geom::PLY_V_FLOAT_Z);
            else if (tokens[1] == "float" && tokens[2] == "nx")
                this->stream.props.push_back(mve::geom::PLY_V_FLOAT_NX);
            else if (tokens[1] == "float" && tokens[2] == "ny")
                this->stream.props.push_back(mve::geom::PLY_V_FLOAT_NY);
            else if (tokens[1] == "float" && tokens[2] == "nz")
                this->stream.props.push_back(mve::geom::PLY_V_FLOAT_NZ);
            else if (tokens[1] == "float" && tokens[2] == "confidence")
                this->stream.props.push_back(mve::geom::PLY_V_FLOAT_CONF);
            else if (tokens[1] == "float" && tokens[2] == "value")
                this->stream.props.push_back(mve::geom::PLY_V_FLOAT_VALUE);
            else if (tokens[1] == "float" && (tokens[2] == "red"
                || tokens[2] == "diffuse_red" || tokens[2] == "r"))
                this->stream.props.push_back(mve::geom::PLY_V_FLOAT_R);
            else if (tokens[1] == "float" && (tokens[2] == "green"
                || tokens[2] == "diffuse_green" || tokens[2] == "g"))
                this->stream.props.push_back(mve::geom::PLY_V_FLOAT_G);
            else if (tokens[1] == "float" && (tokens[2] == "blue"
                || tokens[2] == "diffuse_blue" || tokens[2] == "b"))
                this->stream.props.push_back(mve::geom::PLY_V_FLOAT_B);
            else if (tokens[1] == "uchar" && (tokens[2] == "red"
                || tokens[2] == "diffuse_red" || tokens[2] == "r"))
                this->stream.props.push_back(mve::geom::PLY_V_UINT8_R);
            else if (tokens[1] == "uchar" && (tokens[2] == "green"
                || tokens[2] == "diffuse_green" || tokens[2] == "g"))
                this->stream.props.push_back(mve::geom::PLY_V_UINT8_G);
            else if (tokens[1] == "uchar" && (tokens[2] == "blue"
                || tokens[2] == "diffuse_blue" || tokens[2] == "b"))
                this->stream.props.push_back(mve::geom::PLY_V_UINT8_B);
            else if (tokens[1] == "float")
                this->stream.props.push_back(mve::geom::PLY_V_IGNORE_FLOAT);
            else if (tokens[1] == "uchar")
                this->stream.props.push_back(mve::geom::PLY_V_IGNORE_UINT8);
            else if (tokens[1] == "int")
                this->stream.props.push_back(mve::geom::PLY_V_IGNORE_UINT32);
            else
            {
                this->reset_stream_state();
                throw util::Exception("Unknown property type: " + tokens[1]);
            }

            continue;
        }

        if (!parsing_vertex_props && tokens[0] == "property")
            continue;

        if (tokens[0] == "comment")
            continue;

        std::cerr << "Warning: Unrecognized PLY header: " << line << std::endl;
    }

    /* Sanity check gathered data. */
    if (this->stream.format == mve::geom::PLY_UNKNOWN)
    {
        this->reset_stream_state();
        throw util::Exception("Unknown PLY file format");
    }

    /* If the PLY does not contain vertices, ignore properties. */
    if (this->stream.num_vertices == 0)
        return;

    std::vector<bool> prop_table;
    for (std::size_t i = 0; i < this->stream.props.size(); ++i)
    {
        mve::geom::PLYVertexProperty prop = this->stream.props[i];
        if (prop_table.size() < static_cast<std::size_t>(prop + 1))
            prop_table.resize(prop + 1, false);
        prop_table[prop] = true;
    }

    if (!prop_table[mve::geom::PLY_V_FLOAT_X]
        || !prop_table[mve::geom::PLY_V_FLOAT_Y]
        || !prop_table[mve::geom::PLY_V_FLOAT_Z])
    {
        this->reset_stream_state();
        throw util::Exception("Missing sample coordinates");
    }
    if (!prop_table[mve::geom::PLY_V_FLOAT_NX]
        || !prop_table[mve::geom::PLY_V_FLOAT_NY]
        || !prop_table[mve::geom::PLY_V_FLOAT_NZ])
    {
        this->reset_stream_state();
        throw util::Exception("Missing sample normals");
    }
    if (!prop_table[mve::geom::PLY_V_FLOAT_VALUE])
    {
        this->reset_stream_state();
        throw util::Exception("Missing sample scale");
    }
}

bool
SampleIO::next_sample (Sample* sample)
{
    while (this->next_sample_intern(sample))
        if (this->process_sample(sample, &this->samples))
            return true;
    return false;
}

bool
SampleIO::next_sample_intern (Sample* sample)
{
    if (this->stream.filename.empty())
        throw std::runtime_error("Sample stream not initialized");
    if (!this->stream.stream.good())
        throw std::runtime_error("Sample stream broken");
    if (this->stream.props.empty())
        throw std::runtime_error("Invalid sample stream state");

    sample->confidence = 1.0f;
    sample->color = math::Vec3f(-1.0f);

    if (this->stream.current_vertex == this->stream.num_vertices)
    {
        this->print_samples_state(&this->samples);
        this->reset_samples_state(&this->samples);
        this->reset_stream_state();
        return false;
    }

    for (std::size_t i = 0; i < this->stream.props.size(); ++i)
    {
        mve::geom::PLYVertexProperty property = this->stream.props[i];
        switch (property)
        {
            case mve::geom::PLY_V_FLOAT_X:
                this->ply_read(&sample->pos[0]);
                break;
            case mve::geom::PLY_V_FLOAT_Y:
                this->ply_read(&sample->pos[1]);
                break;
            case mve::geom::PLY_V_FLOAT_Z:
                this->ply_read(&sample->pos[2]);
                break;
            case mve::geom::PLY_V_FLOAT_NX:
                this->ply_read(&sample->normal[0]);
                break;
            case mve::geom::PLY_V_FLOAT_NY:
                this->ply_read(&sample->normal[1]);
                break;
            case mve::geom::PLY_V_FLOAT_NZ:
                this->ply_read(&sample->normal[2]);
                break;
            case mve::geom::PLY_V_FLOAT_R:
                this->ply_read(&sample->color[0]);
                break;
            case mve::geom::PLY_V_FLOAT_G:
                this->ply_read(&sample->color[1]);
                break;
            case mve::geom::PLY_V_FLOAT_B:
                this->ply_read(&sample->color[2]);
                break;
            case mve::geom::PLY_V_UINT8_R:
                this->ply_read_convert(&sample->color[0]);
                break;
            case mve::geom::PLY_V_UINT8_G:
                this->ply_read_convert(&sample->color[1]);
                break;
            case mve::geom::PLY_V_UINT8_B:
                this->ply_read_convert(&sample->color[2]);
                break;
            case mve::geom::PLY_V_FLOAT_VALUE:
                this->ply_read(&sample->scale);
                break;
            case mve::geom::PLY_V_FLOAT_CONF:
                this->ply_read(&sample->confidence);
                break;
            case mve::geom::PLY_V_IGNORE_FLOAT:
            {
                float dummy;
                this->ply_read(&dummy);
                break;
            }
            case mve::geom::PLY_V_IGNORE_UINT8:
            {
                uint8_t dummy;
                this->ply_read(&dummy);
                break;
            }
            default:
                this->reset_stream_state();
                throw std::runtime_error("Invalid sample attribute");
        }

        if (this->stream.stream.eof())
        {
            this->reset_stream_state();
            throw util::FileException(this->stream.filename, "Unexpected EOF");
        }
    }

    this->stream.current_vertex += 1;
    return true;
}

void
SampleIO::ply_read (float* value)
{
    *value = mve::geom::ply_read_value<float>
        (this->stream.stream, this->stream.format);
}

void
SampleIO::ply_read (uint8_t* value)
{
    *value = mve::geom::ply_read_value<uint8_t>
        (this->stream.stream, this->stream.format);
}

void
SampleIO::ply_read_convert (float* value)
{
    uint8_t temp;
    this->ply_read(&temp);
    *value = static_cast<float>(temp) / 255.0f;
}

void
SampleIO::reset_stream_state (void)
{
    this->stream.filename.clear();
    this->stream.stream.close();
    this->stream.props.clear();
    this->stream.format = mve::geom::PLY_UNKNOWN;
    this->stream.num_vertices = 0;
    this->stream.current_vertex = 0;
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
