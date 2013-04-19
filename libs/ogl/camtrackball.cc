#include <iostream>

#include "math/matrixtools.h"
#include "math/quaternion.h"
#include "ogl/opengl.h"
#include "ogl/camtrackball.h"

OGL_NAMESPACE_BEGIN

CamTrackball::CamTrackball (void)
{
    this->cam = 0;
    this->tb_radius = 1.0f;
    this->tb_center = math::Vec3f(0.0f);
    this->tb_tocam = math::Vec3f(0.0f, 0.0f, 1.0f);
    this->tb_upvec = math::Vec3f(0.0f, 1.0f, 0.0f);
}

/* ---------------------------------------------------------------- */

void
CamTrackball::consume_event (MouseEvent const& event)
{
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
            math::Vec3f center = get_center(event.x, event.y);
            if (center != math::Vec3f(0.0f))
                this->tb_center = center;
        }
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
        }

        if (event.button_mask & MOUSE_BUTTON_MIDDLE)
        {
            int mouse_diff = this->zoom_mouse_y - event.y;
            float zoom_speed = this->zoom_tb_radius / 100.0f;
            float cam_diff = (float)mouse_diff * zoom_speed;
            float new_radius = this->zoom_tb_radius + cam_diff;
            this->tb_radius = math::algo::clamp(new_radius, this->cam->z_near, this->cam->z_far);
        }
    }
    else if (event.type == MOUSE_EVENT_WHEEL_UP)
    {
        this->tb_radius = this->tb_radius + this->tb_radius / 10.0f;
        this->tb_radius = std::min(this->cam->z_far, this->tb_radius);
    }
    else if (event.type == MOUSE_EVENT_WHEEL_DOWN)
    {
        this->tb_radius = this->tb_radius - this->tb_radius / 10.0f;
        this->tb_radius = std::max(this->cam->z_near, this->tb_radius);
    }
}

/* ---------------------------------------------------------------- */

void
CamTrackball::consume_event (KeyboardEvent const& /*event*/)
{
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
    //math::Vec3f axis = bn_now.cross(bn_start);
    math::Vec3f axis = bn_start.cross(bn_now);
    float angle = std::acos(bn_start.dot(bn_now));

    /* Rotate axis to world coords. Build inverse viewing matrix from
     * values stored at the time of mouse click.
     */
    math::Matrix4f cam_to_world;
    math::Vec3f campos = this->tb_center + this->rot_tb_tocam * this->tb_radius;
    math::Vec3f viewdir = -this->rot_tb_tocam;
    cam_to_world = math::matrix_viewtrans(campos, viewdir, this->rot_tb_upvec);
    cam_to_world = math::matrix_invert_trans(cam_to_world);
    axis = cam_to_world.mult(axis, 0.0f);
    axis.normalize();

    /* Rotate camera and up vector around axis. */
    math::Quat4f rot(axis, angle);
    this->tb_tocam = rot.rotate(this->rot_tb_tocam);
    this->tb_upvec = rot.rotate(this->rot_tb_upvec);
}

/* ---------------------------------------------------------------- */

math::Vec3f
CamTrackball::get_center (int x, int y)
{
    /* Read depth value from depth buffer. */
    // Is this always safe in this context (QT key event, no GL context)?

    // maximum patchsize should be odd and bigger than one
    #define patchsize 9
    float depth = 1.0f, deptharr[patchsize][patchsize];

    int minx = x - patchsize/2;
    int miny = this->cam->height-1 - y - patchsize/2;

    if(minx >= 0 && minx + patchsize < this->cam->width &&
       miny >= 0 && miny + patchsize < this->cam->height)
    {
        glReadPixels(minx, miny, patchsize, patchsize, GL_DEPTH_COMPONENT, GL_FLOAT, &deptharr);
        //searches for depth values in a growing region(square), if multiple values
        //are found the average value will be taken
        for(int i = 0; i <= patchsize/2; i++)
        {
            int num = 0;
            float sum = 0;
            for(int x = patchsize/2 - i; x <= patchsize/2 + i; x++)
                for(int y = patchsize/2 - i; y <= patchsize/2 + 1; y++)
                {
                    if(deptharr[x][y] != 1.0f) //Discard if MaxValue
                    {
                        sum += deptharr[x][y];
                        num++;
                    }
                }

            if(num != 0)
            {
                depth = sum / num;
                break;
            }
        }
    }

    /* Exit if depth value is not set. */
    if (depth == 1.0f)
        return math::Vec3f(0.0f); // TODO: Better "error" reporting

    float fx = (float)x;
    float fy = (float)y;
    float fw = (float)this->cam->width;
    float fh = (float)this->cam->height;

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
        //std::cout << "Distance value: " << dist << std::endl;
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

    //std::cout << "Ray: " << ray << ", new center: "
    //    << (this->cam->pos + ray) << std::endl;

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
