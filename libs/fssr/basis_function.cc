/*
 * This file is part of the Floating Scale Surface Reconstruction software.
 * Written by Simon Fuhrmann.
 */

#include "math/vector.h"
#include "math/matrix.h"
#include "math/matrix_tools.h"
#include "math/functions.h"
#include "fssr/basis_function.h"

FSSR_NAMESPACE_BEGIN

void
evaluate (math::Vec3f const& pos, Sample const& sample,
    math::Vec4d* value, math::Vec4d* weight)
{
    math::Matrix3f rot, inv_rot;
    rotation_from_normal(sample.normal, &rot);
    inv_rot = rot.transposed();

    math::Vec3f tpos = rot * (pos - sample.pos);
    math::Vec4d v = fssr_basis_deriv<double>(sample.scale, tpos);
    math::Vec4d w = fssr_weight_deriv<double>(sample.scale, tpos);

    (*value)[0] = v[0];
    (*value)[1] = inv_rot[0] * v[1] + inv_rot[1] * v[2] + inv_rot[2] * v[3];
    (*value)[2] = inv_rot[3] * v[1] + inv_rot[4] * v[2] + inv_rot[5] * v[3];
    (*value)[3] = inv_rot[6] * v[1] + inv_rot[7] * v[2] + inv_rot[8] * v[3];
    (*weight)[0] = w[0];
    (*weight)[1] = inv_rot[0] * w[1] + inv_rot[1] * w[2] + inv_rot[2] * w[3];
    (*weight)[2] = inv_rot[3] * w[1] + inv_rot[4] * w[2] + inv_rot[5] * w[3];
    (*weight)[3] = inv_rot[6] * w[1] + inv_rot[7] * w[2] + inv_rot[8] * w[3];
}

/*
 * Rotation from normal in 3D.
 * The rotation axis is determined using cross product between the reference
 * normal and the given normal. The rotation angle is obtained using the
 * dot product. MVE is used to obtain the 3x3 rotation matrix.
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
        (*rot)[6] = 0.0f; (*rot)[7] = 0.0f;   (*rot)[8] = 1.0f;
        return;
    }

    math::Vec3f const axis = normal.cross(ref).normalized();
    float const cos_alpha = math::clamp(ref.dot(normal), -1.0f, 1.0f);
    float const angle = std::acos(cos_alpha);
    *rot = math::matrix_rotation_from_axis_angle(axis, angle);
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
