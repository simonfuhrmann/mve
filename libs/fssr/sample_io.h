/*
 * This file is part of the Floating Scale Surface Reconstruction software.
 * Written by Simon Fuhrmann.
 */

#ifndef FSSR_SAMPLE_IO_HEADER
#define FSSR_SAMPLE_IO_HEADER

#include <string>

#include "mve/mesh_io_ply.h"
#include "fssr/defines.h"
#include "fssr/sample.h"

FSSR_NAMESPACE_BEGIN

/**
 * Reads samples from a PLY file.
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

private:
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

private:
    Options opts;
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
