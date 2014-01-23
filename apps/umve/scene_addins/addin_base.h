#ifndef UMVE_ADDIN_BASE_HEADER
#define UMVE_ADDIN_BASE_HEADER

#include <QWidget>
#include <QMessageBox>

#include "mve/scene.h"
#include "mve/view.h"
#include "mve/mesh.h"
#include "ogl/context.h"
#include "ogl/shader_program.h"

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
    struct State
    {
        GLWidget* gl_widget;
        ogl::ShaderProgram::Ptr surface_shader;
        ogl::ShaderProgram::Ptr wireframe_shader;
        ogl::ShaderProgram::Ptr texture_shader;
        mve::Scene::Ptr scene;
        mve::View::Ptr view;
    };

public:
    AddinBase (void);
    ~AddinBase (void);

    void set_state (State* state);
    virtual QWidget* get_sidebar_widget (void);

    /* Empty base class re-implementations. */
    virtual void init_impl (void);
    virtual void resize_impl (int old_width, int old_height);
    virtual void paint_impl (void);
    virtual void mouse_event (ogl::MouseEvent const& event);
    virtual void keyboard_event (ogl::KeyboardEvent const& event);

signals:
    void mesh_generated (std::string const& name, mve::TriangleMesh::Ptr mesh);

protected:
    void show_error_box (std::string const& title, std::string const& message);
    void show_info_box (std::string const& title, std::string const& message);

protected slots:
    void repaint (void);
    void request_context (void);

protected:
    State* state;
};

/* ---------------------------------------------------------------- */

inline
AddinBase::AddinBase (void)
{
    this->state = NULL;
}

inline
AddinBase::~AddinBase (void)
{
}

inline void
AddinBase::set_state (State* state)
{
    this->state = state;
}

inline QWidget*
AddinBase::get_sidebar_widget()
{
    return NULL;
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

inline void
AddinBase::mouse_event (ogl::MouseEvent const& /*event*/)
{
}

inline void
AddinBase::keyboard_event (ogl::KeyboardEvent const& /*event*/)
{
}

inline void
AddinBase::repaint (void)
{
    this->state->gl_widget->repaint();
}

inline void
AddinBase::request_context (void)
{
    this->state->gl_widget->makeCurrent();
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
