/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SFM_CORRESPONDENCE_HEADER
#define SFM_CORRESPONDENCE_HEADER

#include <vector>

#include "math/matrix.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN

struct Correspondence2D2D;
typedef std::vector<Correspondence2D2D> Correspondences2D2D;

struct Correspondence2D3D;
typedef std::vector<Correspondence2D3D> Correspondences2D3D;

/** The IDs of a matching feature pair in two images. */
typedef std::pair<int, int> CorrespondenceIndex;
/** A list of all matching feature pairs in two images. */
typedef std::vector<CorrespondenceIndex> CorrespondenceIndices;

/**
 * Two image coordinates which correspond to each other in terms of observing
 * the same point in the scene.
 * TODO: Rename this to Correspondence2D2D.
 */
struct Correspondence2D2D
{
    double p1[2];
    double p2[2];
};

/**
 * A 3D point and an image coordinate which correspond to each other in terms
 * of the image observing this 3D point in the scene.
 */
struct Correspondence2D3D
{
    double p3d[3];
    double p2d[2];
};

SFM_NAMESPACE_END

#endif  // SFM_CORRESPONDENCE_HEADER
