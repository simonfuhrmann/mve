/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef FSSR_SAMPLE_HEADER
#define FSSR_SAMPLE_HEADER

#include <vector>

#include "math/vector.h"
#include "fssr/defines.h"

FSSR_NAMESPACE_BEGIN

/** Representation of a point sample. */
struct Sample
{
    math::Vec3f pos;
    math::Vec3f normal;
    math::Vec3f color;
    float scale;
    float confidence;
};

/** Representation of a list of samples. */
typedef std::vector<Sample> SampleList;

/** Comparator that orders samples according to scale. */
bool
sample_scale_compare (Sample const* s1, Sample const* s2);

FSSR_NAMESPACE_END

/* ------------------------- Implementation ---------------------------- */

FSSR_NAMESPACE_BEGIN

inline bool
sample_scale_compare (Sample const* s1, Sample const* s2)
{
    return s1->scale < s2->scale;
}

FSSR_NAMESPACE_END

#endif // FSSR_SAMPLE_HEADER
