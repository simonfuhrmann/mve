/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <algorithm>

#include "math/vector.h"
#include "math/matrix.h"
#include "math/matrix_tools.h"
#include "math/functions.h"
#include "fssr/basis_function.h"

FSSR_NAMESPACE_BEGIN

void
evaluate (math::Vec3f const& pos, Sample const& sample,
    double* value, double* weight,
    math::Vector<double, 3>* value_deriv,
    math::Vector<double, 3>* weight_deriv)
{
    /* Rotate voxel position into the sample's LCS. */
    math::Matrix3f rot;
    rotation_from_normal(sample.normal, &rot);
    math::Vec3f tpos = rot * (pos - sample.pos);

    /* Evaluate basis and weight functions. */
    (*value) = fssr_basis<double>(sample.scale, tpos, value_deriv);
    (*weight) = fssr_weight<double>(sample.scale, tpos, weight_deriv);

    if (value_deriv == nullptr && weight_deriv == nullptr)
        return;

    /* Rotate derivative back to original coordinate system. */
    math::Matrix3f irot = rot.transposed();
    if (value_deriv != nullptr)
        *value_deriv = irot.mult(*value_deriv);
    if (weight_deriv != nullptr)
        *weight_deriv = irot.mult(*weight_deriv);
}

/*
 * Rotation from normal in 3D using two axis orthogonal to the normal.
 */
void
rotation_from_normal (math::Vec3f const& normal, math::Matrix3f* rot)
{
    math::Vec3f const ref(1.0f, 0.0f, 0.0f);
    if (normal.is_similar(ref, 0.001f))
    {
        math::matrix_set_identity(rot);
        return;
    }

    math::Vec3f const mirror(-1.0f, 0.0f, 0.0f);
    if (normal.is_similar(mirror, 0.001f))
    {
        /* 180 degree rotation around the z-axis. */
        (*rot)[0] = -1.0f; (*rot)[1] = 0.0f;  (*rot)[2] = 0.0f;
        (*rot)[3] = 0.0f;  (*rot)[4] = -1.0f; (*rot)[5] = 0.0f;
        (*rot)[6] = 0.0f;  (*rot)[7] = 0.0f;  (*rot)[8] = 1.0f;
        return;
    }

    math::Vec3f const axis1 = normal.cross(ref).normalized();
    math::Vec3f const axis2 = normal.cross(axis1);
    std::copy(normal.begin(), normal.end(), rot->begin() + 0);
    std::copy(axis1.begin(), axis1.end(), rot->begin() + 3);
    std::copy(axis2.begin(), axis2.end(), rot->begin() + 6);
}

/*
 * The 2D rotation matrix where cos(angle) and sin(angle) is directly
 * taken from the normal. The reference normal is oriented toward the
 * positive x-axis.
 */
void
rotation_from_normal (math::Vec2f const& normal, math::Matrix2f* rot)
{
    (*rot)[0] = normal[0];
    (*rot)[1] = normal[1];
    (*rot)[2] = -normal[1];
    (*rot)[3] = normal[0];
}

FSSR_NAMESPACE_END
