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
    /** Default constructor doing nothing. */
    PointSet (void);
    /** Reads the input file with default settings. */
    PointSet (std::string const& filename);

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

    /** Reset to defaults and release points. */
    void clear (void);

private:
    float scale_factor;
    int num_skip;
    SampleList samples;
};

/* ---------------------------------------------------------------- */

inline
PointSet::PointSet (void)
{
    this->clear();
}

inline
PointSet::PointSet (std::string const& filename)
{
    this->clear();
    this->read_from_file(filename);
}

inline void
PointSet::set_scale_factor (float factor)
{
    this->scale_factor = factor;
}

inline void
PointSet::set_skip_samples (int num_skip)
{
    this->num_skip = num_skip;
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

inline void
PointSet::clear (void)
{
    this->scale_factor = 1.0f;
    this->num_skip = 0;
    this->samples.clear();
}

FSSR_NAMESPACE_END

#endif /* FSSR_POINTSET_HEADER */
