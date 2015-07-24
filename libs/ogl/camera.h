/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef OGL_CAMERA_HEADER
#define OGL_CAMERA_HEADER

#include "math/vector.h"
#include "math/matrix.h"
#include "math/matrix_tools.h"
#include "ogl/defines.h"

OGL_NAMESPACE_BEGIN

/**
 * A camera class that manages viewing and projection matrices.
 *
 * The camera is specified by a viewing matrix that transforms from world
 * to camera coordinates, and a projection matrix that transforms from
 * camera to unit cube coordinates as defined by OpenGL.
 *
 * Viewing parameters are given by camera position, viewing direction and
 * up-vector. Projection parameters are z-near and far plane scalars as
 * well as top (also -bottom) and right (also -left) scalars, that define
 * the projection into the OpenGL unit cube.
 */
class Camera
{
public:

    /* --- Viewing matrix parameters --- */

    /** Position of the camera. */
    math::Vec3f pos;
    /** Viewing direction of the camera. */
    math::Vec3f viewing_dir;
    /** Up-vector of the camera. */
    math::Vec3f up_vec;

    /* --- Projection matrix parameters --- */

    /** Near clipping plane of the projection matrix. */
    float z_near;
    /** Far clipping plane of the projection matrix. */
    float z_far;
    /** Top and -Bottom clipping plane of the projection matrix. */
    float top;
    /** Right and -Left clipping plane of the projection matrix. */
    float right;

    /* --- Viewport parameters --- */

    /** The viewport width. */
    int width;
    /** The viewport height. */
    int height;

    /* --- Viewing and projection matrices --- */

    /** View matrix, use update_matrices() to calculate. */
    math::Matrix4f view;
    /** Inverse view matrix, use update_matrices() to calculate. */
    math::Matrix4f inv_view;
    /** Projection matrix, use update_matrices() to calculate. */
    math::Matrix4f proj;
    /** Inverse projection matrix, use update_matrices() to calculate. */
    math::Matrix4f inv_proj;

public:
    /** Inits with default values but does not calculate the matrices. */
    Camera (void);

    /** Sets the viewing parameters and calcualtes view matrices. */
    //void set_view_params (math::Vec3f const& position,
    //    math::Vec3f const& viewdir, math::Vec3f const& upvec);
    /** Sets the projection parameters and calcualtes the proj matrices. */
    //void set_proj_params (float znear, float zfar, float top, float right);

    /** Returns the aspect ratio, right / top, or, width / height. */
    float get_aspect (void) const;
    /** Returns the vertical FOV of the projection in RAD. */
    float get_vertical_fov (void) const;

    /** Updates view, projection and inverse matrices. */
    void update_matrices (void);
    /** Updates the view matrix from pos, viewdir and upvec. */
    void update_view_mat (void);
    /** Updates the inverse view matrix from pos, viewdir and upvec. */
    void update_inv_view_mat (void);
    /** Updates the projection matrix from znear, zfar and aspect. */
    void update_proj_mat (void);
    /** Updates inverse projection matrix from znear, zfar and aspect. */
    void update_inv_proj_mat (void);
};

/* ---------------------------------------------------------------- */

inline
Camera::Camera (void)
    : pos(0.0f, 0.0f, 5.0f)
    , viewing_dir(0.0f, 0.0f, -1.0f)
    , up_vec(0.0f, 1.0f, 0.0f)
    , z_near(0.1f)
    , z_far(500.0f)
    , top(0.1f)
    , right(0.1f)
    , width(0)
    , height(0)
    , view(0.0f)
    , inv_view(0.0f)
    , proj(0.0f)
    , inv_proj(0.0f)
{
}

inline float
Camera::get_aspect (void) const
{
    return right / top;
}

inline float
Camera::get_vertical_fov (void) const
{
    return 2.0f * std::atan(this->top / this->z_near);
}

inline void
Camera::update_matrices (void)
{
    this->update_view_mat();
    this->update_inv_view_mat();
    this->update_proj_mat();
    this->update_inv_proj_mat();
}

inline void
Camera::update_view_mat (void)
{
    this->view = math::matrix_viewtrans(this->pos,
        this->viewing_dir, this->up_vec);
}

inline void
Camera::update_inv_view_mat (void)
{
    this->inv_view = math::matrix_inverse_viewtrans(this->pos,
        this->viewing_dir, this->up_vec);
}

inline void
Camera::update_proj_mat (void)
{
    this->proj = math::matrix_gl_projection(this->z_near,
        this->z_far, this->top, this->right);
}

inline void
Camera::update_inv_proj_mat (void)
{
    this->inv_proj = math::matrix_inverse_gl_projection(this->z_near,
        this->z_far, this->top, this->right);
}

OGL_NAMESPACE_END

#endif /* OGL_CAMERA_HEADER */
