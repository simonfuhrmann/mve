/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef RANGE_IMAGE_HEADER
#define RANGE_IMAGE_HEADER

#include <string>

#include "math/vector.h"
#include "math/matrix.h"

/**
 * Meta-information of a range image. It contains the filename to the range
 * image as well as the camera to world transformation. A vertex in camera
 * coordinates is transformed to world coordinates by:
 *
 *   v' = R * v + t
 */
struct RangeImage
{
    std::string filename;
    std::string fullpath;
    math::Vec3f translation;
    math::Matrix3f rotation;
    math::Vec3f campos;
    math::Vec3f viewdir;
};

#endif /* RANGE_IMAGE_HEADER */
