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
