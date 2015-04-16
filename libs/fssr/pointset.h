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

    /** Reads all input samples in memory. */
    void read_file (std::string const& filename, SampleList* samples);

private:
    Options opts;
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

FSSR_NAMESPACE_END

#endif /* FSSR_POINTSET_HEADER */
