/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef DMRECON_SETTINGS_H
#define DMRECON_SETTINGS_H

#include <stdexcept>
#include <string>
#include <limits>

#include "math/vector.h"
#include "dmrecon/defines.h"

MVS_NAMESPACE_BEGIN

struct Settings
{
    /** The reference view ID to reconstruct. */
    std::size_t refViewNr = 0;

    /** Input image emebdding. */
    std::string imageEmbedding = "undistorted";

    /** Size of the patch is width * width, defaults to 5x5. */
    unsigned int filterWidth = 5;
    float minNCC = 0.3f;
    float minParallax = 10.0f;
    float acceptNCC = 0.6f;
    float minRefineDiff = 0.001f;
    unsigned int maxIterations = 20;
    unsigned int nrReconNeighbors = 4;
    unsigned int globalVSMax = 20;
    int scale = 0;
    bool useColorScale = true;
    bool writePlyFile = false;

    /** Features outside the AABB are ignored. */
    math::Vec3f aabbMin = math::Vec3f(-std::numeric_limits<float>::max());
    math::Vec3f aabbMax = math::Vec3f(std::numeric_limits<float>::max());

    std::string plyPath;

    bool keepDzMap = false;
    bool keepConfidenceMap = false;
    bool quiet = false;
};

MVS_NAMESPACE_END

#endif /* DMRECON_SETTINGS_H */
