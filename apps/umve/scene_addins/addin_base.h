/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef UMVE_ADDIN_BASE_HEADER
#define UMVE_ADDIN_BASE_HEADER

#include <QWidget>
#include <QMessageBox>

#include "mve/mesh.h"
#include "ogl/context.h"

#include "scene_addins/addin_state.h"
#include "glwidget.h"

/*
 * Base class for scene inspect addins. An addin provides UI elements
 * and OpenGL methods to render parts of the scene. Addins are registered
 * in the AddinManager which sets the state after initialization.
 */
class AddinBase : public QObject, public ogl::Context
{
    Q_OBJECT

public:
    AddinBase (void);
    ~AddinBase (void);

    void set_state (AddinState* state);
    virtual QWidget* get_sidebar_widget (void);

    /* Empty base class re-implementations. */
    virtual void init_impl (void);
    virtual void resize_impl (int old_width, int old_height);
    virtual void paint_impl (void);
    virtual bool mouse_event (ogl::MouseEvent const& event);
    virtual bool keyboard_event (ogl::KeyboardEvent const& event);

    virtual void redraw_gui (void);

signals:
    void mesh_generated (std::string const& name, mve::TriangleMesh::Ptr mesh);

protected:
    void show_error_box (std::string const& title, std::string const& message);
    void show_info_box (std::string const& title, std::string const& message);

protected slots:
    void repaint (void);
    void request_context (void);

protected:
    AddinState* state;
};

/* ---------------------------------------------------------------- */

inline
AddinBase::AddinBase (void)
{
    this->state = nullptr;
}

inline
AddinBase::~AddinBase (void)
{
}

inline void
AddinBase::set_state (AddinState* state)
{
    this->state = state;
}

inline QWidget*
AddinBase::get_sidebar_widget()
{
    return nullptr;
}

inline void
AddinBase::init_impl (void)
{
}

inline void
AddinBase::resize_impl (int /*old_width*/, int /*old_height*/)
{
}

inline void
AddinBase::paint_impl (void)
{
}

inline bool
AddinBase::mouse_event (ogl::MouseEvent const& /*event*/)
{
    return false;
}

inline bool
AddinBase::keyboard_event (ogl::KeyboardEvent const& /*event*/)
{
    return false;
}

inline void
AddinBase::redraw_gui (void)
{
}

inline void
AddinBase::repaint (void)
{
    this->state->repaint();
}

inline void
AddinBase::request_context (void)
{
    this->state->make_current_context();
}

inline void
AddinBase::show_error_box (std::string const& title, std::string const& message)
{
    QMessageBox::critical(this->state->gl_widget,
        title.c_str(), message.c_str());
}

inline void
AddinBase::show_info_box (std::string const& title, std::string const& message)
{
    QMessageBox::information(this->state->gl_widget,
        title.c_str(), message.c_str());
}

#endif /* UMVE_ADDIN_BASE_HEADER */
