/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef OGL_CONTEXT_HEADER
#define OGL_CONTEXT_HEADER

#include <algorithm>

#include "ogl/defines.h"
#include "ogl/events.h"
#include "ogl/camera.h"
#include "ogl/camera_trackball.h"
#include "ogl/camera_2d.h"

OGL_NAMESPACE_BEGIN

/**
 * Abstraction of a rendering context/viewport that displays renderings.
 *
 * This class is to abstract from the actual system that creates the
 * OpenGL context and delivers the events, for example GLX/X11, QT,
 * GTK, etc. Creating the OpenGL rendering context is out of scope
 * of this class. Also, creating the event loop / main loop is task
 * of the controlling system.
 *
 * The context can be init'ed, resized and repeatedly painted. Mouse and
 * keyboard events can be injected for the implementation to react on.
 */
class Context
{
public:
    virtual ~Context (void);

    /** Initializes the context. */
    void init (void);
    /** Resizes the context. */
    void resize (int new_width, int new_height);
    /** Paints the frame. */
    void paint (void);

    /**
     * Injects a mouse event to the context.
     * Default implementation prints debug information only.
     */
    virtual bool mouse_event (MouseEvent const& event);

    /**
     * Injects a keyboard event to the context.
     * Default implementation prints debug information only.
     */
    virtual bool keyboard_event (KeyboardEvent const& event);

    /** Returns the width of the viewport. */
    int get_width (void) const;
    /** Returns the height of the viewport. */
    int get_height (void) const;

protected:
    /** Overwrite to define actions on init. */
    virtual void init_impl (void) = 0;
    /** Overwrite to define actions on resize. */
    virtual void resize_impl (int old_width, int old_height) = 0;
    /** Overwrite to define actions on paint. */
    virtual void paint_impl (void) = 0;

protected:
    int width;
    int height;
};

/* ---------------------------------------------------------------- */

template <typename CTRL> class CameraContext;
typedef CameraContext<CamTrackball> CameraTrackballContext;
typedef CameraContext<Cam2D> CameraPlanarContext;

/**
 * A simple context that does some of the common annoying work.
 * This context handles OpenGL resize events and calls OpenGL
 * viewport commands and updates the projection matrix.
 *
 * This context gets a controller as template parameter. The controller
 * receives events and provides viewport parameters to update the camera.
 * The controller is required to support the following operations:
 *
 * - void  CTRL::set_camera (ogl::Camera*)
 * - void  CTRL::consume_event (MouseEvent const&)
 * - void  CTRL::consume_event (KeyboardEvent const&)
 * - Vec3f CTRL::get_campos (void)
 * - Vec3f CTRL::get_viewdir (void)
 * - Vec3f CTRL::get_upvec (void)
 *
 * TODO: Update camera in controller?
 */
template <typename CTRL>
class CameraContext : public Context
{
protected:
    Camera camera;
    CTRL controller;

protected:
    void resize_impl (int old_width, int old_height);
    void update_camera (void);

public:
    CameraContext (void);
    ~CameraContext (void);
    bool keyboard_event (KeyboardEvent const& event);
    bool mouse_event (MouseEvent const& event);
};

/* ------------------------- Implementation ----------------------- */

inline
Context::~Context (void)
{
}

inline void
Context::init (void)
{
    this->init_impl();
}

inline void
Context::resize (int new_width, int new_height)
{
    std::swap(new_width, this->width);
    std::swap(new_height, this->height);
    this->resize_impl(new_height, new_height);
}

inline void
Context::paint (void)
{
    this->paint_impl();
}

inline bool
Context::mouse_event (MouseEvent const& event)
{
    ogl::event_debug_print(event);
    return true;
}

inline bool
Context::keyboard_event (KeyboardEvent const& event)
{
    ogl::event_debug_print(event);
    return true;
}

inline int
Context::get_width (void) const
{
    return this->width;
}

inline int
Context::get_height (void) const
{
    return this->height;
}

/* ---------------------------------------------------------------- */

template <typename CTRL>
inline
CameraContext<CTRL>::CameraContext (void)
{
    this->controller.set_camera(&this->camera);
}

template <typename CTRL>
inline
CameraContext<CTRL>::~CameraContext (void)
{
}

template <typename CTRL>
void
CameraContext<CTRL>::resize_impl (int /*old_width*/, int /*old_height*/)
{
    /* Always use full viewport. */
    glViewport(0, 0, this->width, this->height);
    this->camera.width = this->width;
    this->camera.height = this->height;

    /* Calculate proper top/bottom and left/right planes. */
    float aspect = (float)this->width / (float)this->height;
    float minside = 0.05f;
    if (this->width > this->height)
    {
        this->camera.top = minside;
        this->camera.right = minside * aspect;
    }
    else
    {
        this->camera.top = minside / aspect;
        this->camera.right = minside;
    }

    /* Make sure the camera gets recent values. */
    this->camera.update_proj_mat();
    this->camera.update_inv_proj_mat();
}

template <typename CTRL>
void
CameraContext<CTRL>::update_camera (void)
{
    /* Set camera viewing matrix values. */
    this->camera.pos = this->controller.get_campos();
    this->camera.viewing_dir = this->controller.get_viewdir();
    this->camera.up_vec = this->controller.get_upvec();
    this->camera.update_view_mat();
    this->camera.update_inv_view_mat();
    //this->camera.update_matrices();
}

template <typename CTRL>
bool
CameraContext<CTRL>::mouse_event (MouseEvent const& event)
{
    bool is_handled = this->controller.consume_event(event);
    this->update_camera();
    return is_handled;
}

template <typename CTRL>
bool
CameraContext<CTRL>::keyboard_event (KeyboardEvent const& event)
{
    bool is_handled = this->controller.consume_event(event);
    this->update_camera();
    return is_handled;
}

OGL_NAMESPACE_END

#endif /* OGL_CONTEXT_HEADER */
