/*
 * Copyright (C) 2015, Ronny Klowsky, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef DMRECON_MVS_TOOLS_H
#define DMRECON_MVS_TOOLS_H

#include <iostream>

#include "math/matrix.h"
#include "math/vector.h"
#include "mve/image.h"
#include "dmrecon/defines.h"
#include "dmrecon/single_view.h"

MVS_NAMESPACE_BEGIN

/** interpolate color and derivative at given sample positions */
void colAndExactDeriv(mve::ByteImage const& img,
    PixelCoords const& imgPos, PixelCoords const& gradDir,
    Samples& color, Samples& deriv);

/** get color at given pixel positions (no interpolation) */
void getXYZColorAtPix(mve::ByteImage const& img,
    std::vector<math::Vec2i> const& imgPos, Samples* color);

/** interpolate only color at given sample positions */
void getXYZColorAtPos(mve::ByteImage const& img,
    PixelCoords const& imgPos, Samples* color);

/** Computes the parallax between two views with respect to some 3D point p */
float parallax(math::Vec3f p, mvs::SingleView::Ptr v1, mvs::SingleView::Ptr v2);

/** Turns a parallax value (0 <= p <= 180) into a weight according
    to a bilateral Gaussian (see [Furukawa 2010] for details) */
float parallaxToWeight(float p);

/* ------------------------- Implementation ----------------------- */

inline float
parallax(math::Vec3f p, mvs::SingleView::Ptr v1, mvs::SingleView::Ptr v2)
{
    math::Vec3f dir1 = (p - v1->camPos).normalized();
    math::Vec3f dir2 = (p - v2->camPos).normalized();
    float dp = std::max(std::min(dir1.dot(dir2), 1.f), -1.f);
    float plx = std::acos(dp) * 180.f / pi;
    return plx;
}

inline float
parallaxToWeight(float p)
{
    if (p < 0.f || p > 180.f) {
        std::cerr << "ERROR: invalid parallax value." << std::endl;
        return 0.f;
    }
    float sigma;
    if (p <= 20.f)
        sigma = 5.f;
    else
        sigma = 15.f;
    float mean = 20.f;
    return exp(- sqr(p - mean) / (2 * sigma * sigma));
}

MVS_NAMESPACE_END

#endif
