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
#include "math/quaternion.h"
#include "math/functions.h"
#include "ogl/opengl.h"
#include "ogl/camera_trackball.h"

OGL_NAMESPACE_BEGIN

CamTrackball::CamTrackball (void)
{
    this->cam = nullptr;
    this->tb_radius = 1.0f;
    this->tb_center = math::Vec3f(0.0f);
    this->tb_tocam = math::Vec3f(0.0f, 0.0f, 1.0f);
    this->tb_upvec = math::Vec3f(0.0f, 1.0f, 0.0f);
}

/* ---------------------------------------------------------------- */

bool
CamTrackball::consume_event (MouseEvent const& event)
{
    bool is_handled = false;

    if (event.type == MOUSE_EVENT_PRESS)
    {
        if (event.button == MOUSE_BUTTON_LEFT)
        {
            this->rot_mouse_x = event.x;
            this->rot_mouse_y = event.y;
            this->rot_tb_tocam = this->tb_tocam;
            this->rot_tb_upvec = this->tb_upvec;
        }
        else if (event.button == MOUSE_BUTTON_MIDDLE)
        {
            this->zoom_mouse_y = event.y;
            this->zoom_tb_radius = this->tb_radius;
        }
        else if (event.button == MOUSE_BUTTON_RIGHT)
        {
            math::Vec3f center = this->get_center(event.x, event.y);
            if (center != math::Vec3f(0.0f))
                this->tb_center = center;
        }
        is_handled = true;
    }
    else if (event.type == MOUSE_EVENT_MOVE)
    {
        if (event.button_mask & MOUSE_BUTTON_LEFT)
        {
            if (event.x == this->rot_mouse_x && event.y == this->rot_mouse_y)
            {
                this->tb_tocam = this->rot_tb_tocam;
                this->tb_upvec = this->rot_tb_upvec;
            }
            else
            {
                this->handle_tb_rotation(event.x, event.y);
            }
            is_handled = true;
        }

        if (event.button_mask & MOUSE_BUTTON_MIDDLE)
        {
            int mouse_diff = this->zoom_mouse_y - event.y;
            float zoom_speed = this->zoom_tb_radius / 100.0f;
            float cam_diff = (float)mouse_diff * zoom_speed;
            float new_radius = this->zoom_tb_radius + cam_diff;
            this->tb_radius = math::clamp(new_radius, this->cam->z_near, this->cam->z_far);
            is_handled = true;
        }
    }
    else if (event.type == MOUSE_EVENT_WHEEL_UP)
    {
        this->tb_radius = this->tb_radius + this->tb_radius / 10.0f;
        this->tb_radius = std::min(this->cam->z_far, this->tb_radius);
        is_handled = true;
    }
    else if (event.type == MOUSE_EVENT_WHEEL_DOWN)
    {
        this->tb_radius = this->tb_radius - this->tb_radius / 10.0f;
        this->tb_radius = std::max(this->cam->z_near, this->tb_radius);
        is_handled = true;
    }

    return is_handled;
}

/* ---------------------------------------------------------------- */

bool
CamTrackball::consume_event (KeyboardEvent const& /*event*/)
{
    return false;
}

/* ---------------------------------------------------------------- */

void
CamTrackball::handle_tb_rotation (int x, int y)
{
    /* Get ball normals. */
    math::Vec3f bn_start = this->get_ball_normal
        (this->rot_mouse_x, this->rot_mouse_y);
    math::Vec3f bn_now = this->get_ball_normal(x, y);

    /* Rotation axis and angle. */
    math::Vec3f axis = bn_now.cross(bn_start);
    float angle = std::acos(bn_now.dot(bn_start));

    /* Rotate axis to world coords. Build inverse viewing matrix from
     * values stored at the time of mouse click.
     */
    math::Matrix4f cam_to_world;
    math::Vec3f campos = this->tb_center + this->rot_tb_tocam * this->tb_radius;
    math::Vec3f viewdir = -this->rot_tb_tocam;
    cam_to_world = math::matrix_inverse_viewtrans(
        campos, viewdir, this->rot_tb_upvec);
    axis = cam_to_world.mult(axis, 0.0f);
    axis.normalize();

    /* Rotate camera and up vector around axis. */
    math::Matrix3f rot = matrix_rotation_from_axis_angle(axis, angle);
    this->tb_tocam = rot * this->rot_tb_tocam;
    this->tb_upvec = rot * this->rot_tb_upvec;
}

/* ---------------------------------------------------------------- */

math::Vec3f
CamTrackball::get_center (int x, int y)
{
    /* Try to find a valid depth value in a spiral around the click point. */
    float depth = 1.0f;
    {
        /* Patchsize should be odd and larger than one. */
        int const patch_size = 9;
        int const patch_halfsize = patch_size / 2;
        int const screen_width = this->cam->width;
        int const screen_height = this->cam->height;
        int const center_x = x;
        int const center_y = y;
        int dx = 1;
        int dy = 0;
        int radius = 0;
        while (radius <= patch_halfsize)
        {
            if (x >= 0 && x < screen_width && y > 0 && y < screen_height)
                glReadPixels(x, screen_height - y - 1, 1, 1,
                    GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
            if (depth != 1.0f)
                break;

            x += dx;
            y += dy;
            if (x > center_x + radius)
            {
                radius += 1;
                dx = 0;
                dy = -1;
            }
            if (y <= center_y - radius)
            {
                dx = -1;
                dy = 0;
            }
            if (x <= center_x - radius)
            {
                dx = 0;
                dy = 1;
            }
            if (y >= center_y + radius)
            {
                dx = 1;
                dy = 0;
            }
        }
    }

    /* Exit if depth value is not set. */
    if (depth == 1.0f)
        return math::Vec3f(0.0f); // TODO: Better "error" reporting

    float const fx = static_cast<float>(x);
    float const fy = static_cast<float>(y);
    float const fw = static_cast<float>(this->cam->width);
    float const fh = static_cast<float>(this->cam->height);

    /* Calculate camera-to-surface distance (orthographic). */
    float dist = (this->cam->z_far * this->cam->z_near)
        / ((this->cam->z_near - this->cam->z_far) * depth + this->cam->z_far);

    /* Fix distance value caused by projection. */
    {
        /* Create point on near plane corresponding to click coords. */
        math::Vec3f pnp((2.0f * fx / (fw - 1.0f) - 1.0f) * this->cam->right,
            (1.0f - 2.0f * fy / (fh - 1.0f)) * this->cam->top,
            this->cam->z_near);
        float cosangle = pnp.normalized()[2];
        dist /= cosangle;
    }

    /* Create a point in unit cube corresponding to the click coords. */
    math::Vec3f ray(2.0f * fx / (fw - 1.0f) - 1.0f,
        1.0f - 2.0f * fy / (fh - 1.0f), 0.0f);
    /* Convert cube click coords to ray in camera corrds. */
    ray = this->cam->inv_proj.mult(ray, 1.0f);
    /* Ray to new camera center in camera coords. */
    ray = ray.normalized() * dist;
    /* Ray to new camera center in world coords. */
    ray = this->cam->inv_view.mult(ray, 0.0f);

    return this->cam->pos + ray;
}

/* ---------------------------------------------------------------- */

math::Vec3f
CamTrackball::get_ball_normal (int x, int y)
{
    /* Calculate normal on unit sphere. */
    math::Vec3f sn;
    sn[0] = 2.0f * (float)x / (float)(this->cam->width - 1) - 1.0f;
    sn[1] = 1.0f - 2.0f * (float)y / (float)(this->cam->height - 1);
    float z2 = 1.0f - sn[0] * sn[0] - sn[1] * sn[1];
    sn[2] = z2 > 0.0f ? std::sqrt(z2) : 0.0f;

    return sn.normalize();
}

/* ---------------------------------------------------------------- */

void
CamTrackball::set_camera_params (math::Vec3f const& center,
    math::Vec3f const& lookat, math::Vec3f const& upvec)
{
    this->tb_radius = (center - lookat).norm();
    this->tb_center = lookat;
    this->tb_tocam = (center - lookat).normalized();
    this->tb_upvec = upvec;
}

OGL_NAMESPACE_END
