/*
 * This file is part of the Floating Scale Surface Reconstruction software.
 * Written by Simon Fuhrmann.
 */

#ifndef FSSR_POINTSET_HEADER
#define FSSR_POINTSET_HEADER

#include <string>
#include <vector>

#include "fssr/defines.h"
#include "fssr/sample.h"

FSSR_NAMESPACE_BEGIN

/**
 * Reads a point set from file and converts it to samples.
 */
class PointSet
{
public:
    typedef std::vector<Sample> SampleList;

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
    PointSet (Options const& opts);

    /** Sets the factor multiplied with sample scale. */
    void set_scale_factor (float factor);
    /** Sets how many samples are skipped, defaults to 0. */
    void set_skip_samples (int num_skip);

    /** Reads the input file. Options need to be set before calling this. */
    void read_from_file (std::string const& filename);

    /* Returns the list of samples. */
    SampleList const& get_samples (void) const;
    /* Returns the list of samples. */
    SampleList& get_samples (void);

private:
    Options opts;
    SampleList samples;
};

/* ---------------------------------------------------------------- */

inline
PointSet::Options::Options (void)
    : scale_factor(1.0f)
    , min_scale(-1.0f)
    , max_scale(-1.0f)
{
}

inline
PointSet::PointSet (Options const& opts)
    : opts(opts)
{
}

inline PointSet::SampleList const&
PointSet::get_samples (void) const
{
    return this->samples;
}

inline PointSet::SampleList&
PointSet::get_samples (void)
{
    return this->samples;
}

FSSR_NAMESPACE_END

#endif /* FSSR_POINTSET_HEADER */
