/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 *
 * Kneips original code is available here:
 * http://www.laurentkneip.de/research.html
 */

#ifndef SFM_POSE_P3P_HEADER
#define SFM_POSE_P3P_HEADER

#include <vector>

#include "math/matrix.h"
#include "sfm/defines.h"

SFM_NAMESPACE_BEGIN

/**
 * Implementation of the perspective three point (P3P) algorithm. The
 * algorithm computes the pose of a camera given three 2D-3D correspondences.
 * The implementation closely follows the implementation of Kneip et al.
 * and is described in:
 *
 *   "A Novel Parametrization of the Perspective-Three-Point Problem for a
 *   Direct Computation of Absolute Camera Position and Orientation",
 *   by Laurent Kneip, Davide Scaramuzza and Roland Siegwart, CVPR 2011.
 *   http://www.laurentkneip.de/research.html
 *
 * The algorithm assumes a given camera calibration and takes as input
 * three 3D points 'p' and three 2D points. Instead of 2D points, the three
 * directions 'f' to the given points computed in the camera frame. Four
 * solutions [R | t] are returned. If the points are co-linear, no solution
 * is returned. The correct solution can be found by back-projecting a
 * forth point in the camera.
 */
void
pose_p3p_kneip (
    math::Vec3d p1, math::Vec3d p2, math::Vec3d p3,
    math::Vec3d f1, math::Vec3d f2, math::Vec3d f3,
    std::vector<math::Matrix<double, 3, 4> >* solutions);

SFM_NAMESPACE_END

#endif /* SFM_POSE_P3P_HEADER */
