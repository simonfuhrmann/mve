/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>

#include "math/matrix_tools.h"
#include "ogl/opengl.h"
#include "ogl/camera_2d.h"

OGL_NAMESPACE_BEGIN

Cam2D::Cam2D(void)
{
    this->radius = 1.0f;
    this->center = math::Vec3f(0.0f);
    this->mousePos = math::Vec2f(0.0f);
    this->tocam = math::Vec3f(0.0f, 0.0f, 1.0f);
    this->upvec = math::Vec3f(0.0f, 1.0f, 0.0f);
}

void
Cam2D::consume_event(const MouseEvent &event)
{
    if (event.type == MOUSE_EVENT_PRESS)
    {
        if (event.button == MOUSE_BUTTON_LEFT)
        {
            this->mousePos[0] = event.x;
            this->mousePos[1] = event.y;
        }
    }
    else if (event.type == MOUSE_EVENT_MOVE)
    {
        // Center is translated by the position difference (in pixel)
        // TODO Make this Viewport dependent?
        // Conflict between projective/orthographic assumptions
        if (event.button_mask & MOUSE_BUTTON_LEFT)
        {
            this->center[0] += (this->mousePos[0] - event.x);
            this->center[1] += (this->mousePos[1] - event.y);
            this->mousePos[0] = event.x;
            this->mousePos[1] = event.y;
        }
    }
    else if (event.type == MOUSE_EVENT_WHEEL_UP)
    {
        this->radius += this->radius / 10.0f;
        this->radius = std::min(40.0f, this->radius);
    }
    else if (event.type == MOUSE_EVENT_WHEEL_DOWN)
    {
        this->radius -= this->radius / 10.0f;
        this->radius = std::max(0.01f, this->radius);
    }

}

void
Cam2D::consume_event(KeyboardEvent const& /*event*/)
{
}

math::Vec3f Cam2D::get_campos()
{
    return this->center + this->tocam * this->radius;
}

math::Vec3f Cam2D::get_viewdir()
{
    return -this->tocam;
}

math::Vec3f Cam2D::get_upvec()
{
    return this->upvec;
}

void Cam2D::set_camera (Camera *camera)
{
    this->cam = camera;
}

OGL_NAMESPACE_END
