#include <iostream>
#include <sstream>
#include <algorithm>

#include "math/matrixtools.h"
#include "camera.h"

MVE_NAMESPACE_BEGIN

CameraInfo::CameraInfo (void)
{
    this->flen = 0.0f;
    this->paspect = 1.0f;
    std::fill(this->ppoint, this->ppoint + 2, 0.5f);
    std::fill(this->dist, this->dist + 2, 0.0f);

    std::fill(this->trans, this->trans + 3, 0.0f);
    math::matrix_set_identity(this->rot, 3);
}

/* ---------------------------------------------------------------- */

void
CameraInfo::fill_camera_pos (float* pos) const
{
    pos[0] = -rot[0] * trans[0] - rot[3] * trans[1] - rot[6] * trans[2];
    pos[1] = -rot[1] * trans[0] - rot[4] * trans[1] - rot[7] * trans[2];
    pos[2] = -rot[2] * trans[0] - rot[5] * trans[1] - rot[8] * trans[2];
}

/* ---------------------------------------------------------------- */

void
CameraInfo::fill_camera_translation (float* trans) const
{
    std::copy(this->trans, this->trans + 3, trans);
}

/* ---------------------------------------------------------------- */

void
CameraInfo::fill_viewing_direction (float* viewdir) const
{
    for (int i = 0; i < 3; ++i)
        viewdir[i] = -this->rot[6 + i];
}

/* ---------------------------------------------------------------- */

void
CameraInfo::fill_world_to_cam (float* mat) const
{
    mat[0]  = rot[0]; mat[1]  = rot[1]; mat[2]  = rot[2]; mat[3]  = trans[0];
    mat[4]  = rot[3]; mat[5]  = rot[4]; mat[6]  = rot[5]; mat[7]  = trans[1];
    mat[8]  = rot[6]; mat[9]  = rot[7]; mat[10] = rot[8]; mat[11] = trans[2];
    mat[12] = 0.0f;   mat[13] = 0.0f;   mat[14] = 0.0f;   mat[15] = 1.0f;
}

/* ---------------------------------------------------------------- */

void
CameraInfo::fill_cam_to_world (float* mat) const
{
    mat[0]  = rot[0]; mat[1] = rot[3]; mat[2]  = rot[6];
    mat[4]  = rot[1]; mat[5] = rot[4]; mat[6]  = rot[7];
    mat[8]  = rot[2]; mat[9] = rot[5]; mat[10] = rot[8];
    mat[3]  = -(rot[0] * trans[0] + rot[3] * trans[1] + rot[6] * trans[2]);
    mat[7]  = -(rot[1] * trans[0] + rot[4] * trans[1] + rot[7] * trans[2]);
    mat[11] = -(rot[2] * trans[0] + rot[5] * trans[1] + rot[8] * trans[2]);
    mat[12] = 0.0f;   mat[13] = 0.0f;   mat[14] = 0.0f;   mat[15] = 1.0f;
}

/* ---------------------------------------------------------------- */

void
CameraInfo::fill_world_to_cam_rot (float* mat) const
{
    std::copy(this->rot, this->rot + 9, mat);
}

/* ---------------------------------------------------------------- */

void
CameraInfo::fill_cam_to_world_rot (float* mat) const
{
    mat[0]  = rot[0]; mat[1] = rot[3]; mat[2] = rot[6];
    mat[3]  = rot[1]; mat[4] = rot[4]; mat[5] = rot[7];
    mat[6]  = rot[2]; mat[7] = rot[5]; mat[8] = rot[8];
}

/* ---------------------------------------------------------------- */

void
CameraInfo::set_transformation (float const* mat)
{
    rot[0] = mat[0]; rot[1] = mat[1]; rot[2] = mat[2];  trans[0] = mat[3];
    rot[3] = mat[4]; rot[4] = mat[5]; rot[5] = mat[6];  trans[1] = mat[7];
    rot[6] = mat[8]; rot[7] = mat[9]; rot[8] = mat[10]; trans[2] = mat[11];
}

/* ---------------------------------------------------------------- */

void
CameraInfo::fill_projection (float* mat, std::size_t w, std::size_t h) const
{
    float aspect = (float)w / (float)h; // ratio between width and height
    float iaspect = aspect * this->paspect; // aspect ratio of the image

    /* mx and my are the number of pixels per unit distance. */
    float mx = (iaspect < 1 ? (float)h / this->paspect : (float)w);
    float my = (iaspect < 1 ? (float)h : (float)w * this->paspect);
    float ax = this->flen * mx; // horizontal focal length in pixels
    float ay = this->flen * my; // vertical focal length in pixels

    mat[0] =  -ax; mat[1] = 0.0f; mat[2] = -(float)w * this->ppoint[0];
    mat[3] = 0.0f; mat[4] =   ay; mat[5] = -(float)h * this->ppoint[1];
    mat[6] = 0.0f; mat[7] = 0.0f; mat[8] = -1.0f;
}

/* ---------------------------------------------------------------- */

void
CameraInfo::fill_inverse_projection (float* mat,
    std::size_t w, std::size_t h) const
{
    float aspect = (float)w / (float)h;
    float iaspect = aspect * this->paspect;

    float mx = (iaspect < 1 ? (float)h / this->paspect : (float)w);
    float my = (iaspect < 1 ? (float)h : (float)w * this->paspect);
    float ax = this->flen * mx;
    float ay = this->flen * my;

    mat[0] = -1.0f / ax; mat[1] = 0.0f;
    mat[2] = (float)w * this->ppoint[0] / ax;
    mat[3] = 0.0f; mat[4] = 1.0f / ay;
    mat[5] = -(float)h * this->ppoint[1] / ay;
    mat[6] = 0.0f; mat[7] = 0.0f; mat[8] = -1.0f;
}

/* ---------------------------------------------------------------- */

std::string
CameraInfo::to_ext_string (void) const
{
    std::stringstream ss;
    for (int i = 0; i < 3; ++i)
        ss << this->trans[i] << " ";
    for (int i = 0; i < 9; ++i)
        ss << this->rot[i] << (i < 8 ? " " : "");
    return ss.str();
}

/* ---------------------------------------------------------------- */

void
CameraInfo::from_ext_string (std::string const& str)
{
    std::stringstream ss(str);
    for (int i = 0; i < 3; ++i)
        ss >> this->trans[i];
    for (int i = 0; i < 9; ++i)
        ss >> this->rot[i];
}

/* ---------------------------------------------------------------- */

std::string
CameraInfo::to_int_string (void) const
{
    bool default_rd = (this->dist[0] == 0.0f && this->dist[1] == 0.0f);
    bool default_pa = (this->paspect == 1.0f);
    bool default_pp = (this->ppoint[0] == 0.5f && this->ppoint[1] == 0.5f);

    std::stringstream ss;
    ss << this->flen;

    if (!default_rd || !default_pa || !default_pp)
        ss << " " << this->dist[0] << " " << this->dist[1];
    if (!default_pa || !default_pp)
        ss << " " << this->paspect;
    if (!default_pp)
        ss << " " << this->ppoint[0] << " " << this->ppoint[1];

    return ss.str();
}

/* ---------------------------------------------------------------- */

void
CameraInfo::from_int_string (std::string const& str)
{
    std::stringstream ss(str);
    ss >> this->flen;
    if (ss.peek() && !ss.eof())
        ss >> this->dist[0];
    if (ss.peek() && !ss.eof())
        ss >> this->dist[1];
    if (ss.peek() && !ss.eof())
        ss >> this->paspect;
    if (ss.peek() && !ss.eof())
        ss >> this->ppoint[0];
    if (ss.peek() && !ss.eof())
        ss >> this->ppoint[1];
}

/* ---------------------------------------------------------------- */

void
CameraInfo::debug_print (void) const
{
    std::cout << "Extrinsic camera parameters:" << std::endl
        << "  Trans: " << math::Vec3f(this->trans) << std::endl
        << "  Rot: " << math::Vec3f(&this->rot[0]) << std::endl
        << "       " << math::Vec3f(&this->rot[3]) << std::endl
        << "       " << math::Vec3f(&this->rot[6]) << std::endl
        << "Intrinsic camera parameters:" << std::endl
        << "  Focal Length: " << this->flen << std::endl
        << "  Principal Point: " << math::Vec2f(this->ppoint) << std::endl
        << "  Pixel aspect: " << this->paspect << std::endl
        << "  Radial distortion: " << math::Vec2f(this->dist) << std::endl
        << std::endl;
}

MVE_NAMESPACE_END
