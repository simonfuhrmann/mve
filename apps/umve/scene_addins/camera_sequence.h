/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SEQUENCE_HEADER
#define SEQUENCE_HEADER

#include <string>
#include <vector>
#include "math/vector.h"
#include "math/bspline.h"
#include "ogl/camera.h"

struct CameraSpline
{
    std::string name;
    int length;
    std::vector<math::Vec3f> camera;
    std::vector<math::Vec3f> lookat;
    math::BSpline<math::Vec3f> cs;
    math::BSpline<math::Vec3f> ls;

    CameraSpline (void);
};

/*
 * File format for camera path:
 */
class CameraSequence
{
public:
    typedef std::vector<CameraSpline> Splines;

private:
    int fps;
    int frame;
    Splines seq;

    /* Per-frame information. */
    int time;
    math::Vec3f campos;
    math::Vec3f lookat;
    math::Vec3f upvec;

public:
    CameraSequence (void);
    void read_file (std::string const& fname);
    void write_file (std::string const& fname);

    void transform (math::Matrix4f const& transf);

    void set_fps (int fps);
    int get_fps (void) const;

    math::Vec3f const& get_campos (void) const;
    math::Vec3f const& get_lookat (void) const;
    math::Vec3f const& get_upvec (void) const;

    Splines const& get_splines (void) const;

    /*
     * Every time next frame is called, the sequence is advanced,
     * and new camera parameters are applied.
     */
    bool next_frame (void);

    /* Returns the current frame number. */
    int get_frame (void) const;

    /* Applies viewing parameters to camera. */
    void apply_camera (ogl::Camera& camera);

    /* Resets the sequence. */
    void reset (void);
};

/* ----------------------------------------------------------------------- */

inline
CameraSpline::CameraSpline (void)
{
    this->length = 0;
}

inline
CameraSequence::CameraSequence (void)
{
    this->fps = 25;
    this->frame = 0;
    this->time = 0;
    this->upvec = math::Vec3f(0.0f, 1.0f, 0.0f);
}

inline void
CameraSequence::set_fps (int fps)
{
    this->fps = fps;
}

inline int
CameraSequence::get_fps (void) const
{
    return this->fps;
}

inline CameraSequence::Splines const&
CameraSequence::get_splines (void) const
{
    return this->seq;
}

inline int
CameraSequence::get_frame (void) const
{
    return this->frame;
}

inline math::Vec3f const&
CameraSequence::get_campos (void) const
{
    return this->campos;
}

inline math::Vec3f const&
CameraSequence::get_lookat (void) const
{
    return this->lookat;
}

inline math::Vec3f const&
CameraSequence::get_upvec (void) const
{
    return this->upvec;
}

inline void
CameraSequence::reset (void)
{
    this->frame = 0;
    this->time = 0;
}

#endif /* SEQUENCE_HEADER */
