/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef FSSR_SAMPLE_IO_HEADER
#define FSSR_SAMPLE_IO_HEADER

#include <fstream>
#include <string>

#include "mve/mesh_io_ply.h"
#include "fssr/defines.h"
#include "fssr/sample.h"

FSSR_NAMESPACE_BEGIN

/**
 * Reads samples from a PLY file. Two input types are supported:
 * Reading the whole file at once using the MVE PLY file reader, and a
 * streaming reader which reads one sample at at time.
 */
class SampleIO
{
public:
    struct Options
    {
        Options (void);

        float scale_factor;
        float min_scale;
        float max_scale;
    };

public:
    /** Default constructor setting options. */
    SampleIO (Options const& opts);

    /** Reads all input samples in memory. */
    void read_file (std::string const& filename, SampleList* samples);

    /** Opens the input file for stream reading. */
    void open_file (std::string const& filename);
    /** Reads one sample, returns false if there are no more samples. */
    bool next_sample (Sample* sample);

private:
    struct StreamState
    {
        std::string filename;
        std::ifstream stream;
        std::vector<mve::geom::PLYVertexProperty> props;
        mve::geom::PLYFormat format;
        unsigned int num_vertices;
        unsigned int current_vertex;
    };

    struct SamplesState
    {
        unsigned int num_skipped_zero_normal;
        unsigned int num_skipped_invalid_confidence;
        unsigned int num_skipped_invalid_scale;
        unsigned int num_skipped_large_scale;
        unsigned int num_unnormalized_normals;
    };

private:
    bool process_sample (Sample* sample, SamplesState* state);
    void reset_samples_state (SamplesState* state);
    void print_samples_state (SamplesState* state);

    void ply_read (float* value);
    void ply_read (uint8_t* value);
    void ply_read_convert (float* value);
    bool next_sample_intern (Sample* sample);
    void reset_stream_state (void);

private:
    Options opts;
    StreamState stream;
    SamplesState samples;
};

/* ---------------------------------------------------------------- */

inline
SampleIO::Options::Options (void)
    : scale_factor(1.0f)
    , min_scale(-1.0f)
    , max_scale(-1.0f)
{
}

inline
SampleIO::SampleIO (Options const& opts)
    : opts(opts)
{
}

FSSR_NAMESPACE_END

#endif /* FSSR_SAMPLE_IO_HEADER */
