/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef OGL_CAM_TRACKBALL_HEADER
#define OGL_CAM_TRACKBALL_HEADER

#include "math/vector.h"
#include "ogl/defines.h"
#include "ogl/events.h"
#include "ogl/camera.h"

OGL_NAMESPACE_BEGIN

/**
 * A trackball camera control that consumes mouse events
 * and delivers viewing parameters for the camera.
 */
class CamTrackball
{
public:
    CamTrackball (void);

    void set_camera (Camera* camera);
    bool consume_event (MouseEvent const& event);
    bool consume_event (KeyboardEvent const& event);

    void set_camera_params (math::Vec3f const& center,
        math::Vec3f const& lookat, math::Vec3f const& upvec);

    math::Vec3f get_campos (void) const;
    math::Vec3f get_viewdir (void) const;
    math::Vec3f const& get_upvec (void) const;

private:
    math::Vec3f get_center (int x, int y);
    void handle_tb_rotation (int x, int y);
    math::Vec3f get_ball_normal (int x, int y);

private:
    /* Camera information. */
    Camera* cam;

    /* Current trackball configuration. */
    float tb_radius;
    math::Vec3f tb_center;
    math::Vec3f tb_tocam;
    math::Vec3f tb_upvec;

    /* Variables to calculate rotation and zoom. */
    int rot_mouse_x;
    int rot_mouse_y;
    math::Vec3f rot_tb_tocam;
    math::Vec3f rot_tb_upvec;

    float zoom_tb_radius;
    int zoom_mouse_y;
};

/* ---------------------------------------------------------------- */

inline void
CamTrackball::set_camera (Camera* camera)
{
    this->cam = camera;
}

inline math::Vec3f
CamTrackball::get_campos (void) const
{
    return this->tb_center + this->tb_tocam * this->tb_radius;
}

inline math::Vec3f
CamTrackball::get_viewdir (void) const
{
    return -this->tb_tocam;
}

inline math::Vec3f const&
CamTrackball::get_upvec (void) const
{
    return this->tb_upvec;
}

OGL_NAMESPACE_END

#endif /* OGL_CAM_TRACKBALL_HEADER */
