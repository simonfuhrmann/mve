/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef OPENGL_CAMERA_2D_HEADER
#define OPENGL_CAMERA_2D_HEADER

#include "math/vector.h"
#include "ogl/defines.h"
#include "ogl/events.h"
#include "ogl/camera.h"

OGL_NAMESPACE_BEGIN

class Cam2D
{
public:
    Cam2D(void);

    void set_camera (Camera* camera);
    void consume_event (MouseEvent const& event);
    void consume_event(KeyboardEvent const& event);
    math::Vec3f get_campos(void);
    math::Vec3f get_viewdir(void);
    math::Vec3f get_upvec(void);

private:
    /* Camera information. */
    Camera* cam;

    float radius;
    math::Vec3f center;
    math::Vec2f mousePos;
    math::Vec3f tocam;
    math::Vec3f upvec;
};

OGL_NAMESPACE_END

#endif  /* OPENGL_CAMERA_2D_HEADER */
