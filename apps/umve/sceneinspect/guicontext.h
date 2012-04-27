#ifndef UMVE_GUICONTEXT_HEADER
#define UMVE_GUICONTEXT_HEADER

#include <string>
#include <QtGui>

#include "mve/scene.h"
#include "mve/view.h"

#include "glwidget.h"

class GuiContext : public QWidget
{
    Q_OBJECT

private:
    GLWidget* glw;

public:
    GuiContext (void);
    virtual ~GuiContext (void);

    void set_gl_widget (GLWidget* glwidget);

    /**
     * Called when creating a notebook tab for the context.
     * Return NULL if no GUI tab should be created.
     */
    virtual char const* get_gui_name (void) const;

    /** Called when "Reload Shaders" GUI button is pressed. */
    virtual void reload_shaders (void);

    /** When opening a file, it is passed to the current context. */
    virtual void load_file (std::string const& filename);

    /** When a new scene is loaded, it is passed to all contexts. */
    virtual void set_scene (mve::Scene::Ptr scene);

    /** When a view is selected, it is passed to all contexts. */
    virtual void set_view (mve::View::Ptr view);

    /** Resets all references to the scene and views. */
    virtual void reset (void);

public slots:
    /** Requests update of the GL widget. */
    void update_gl (void);

    /** Requests GL context from associated GL widget. */
    void request_context (void);
};

/* ---------------------------------------------------------------- */

inline
GuiContext::GuiContext (void)
{
}

inline
GuiContext::~GuiContext (void)
{
}

inline void
GuiContext::set_gl_widget (GLWidget* glwidget)
{
    this->glw = glwidget;
}

inline char const*
GuiContext::get_gui_name (void) const
{
    return 0;
}

inline void
GuiContext::reload_shaders (void)
{
}

inline void
GuiContext::load_file (std::string const& /*filename*/)
{
}

inline void
GuiContext::set_scene (mve::Scene::Ptr /*scene*/)
{
}

inline void
GuiContext::set_view (mve::View::Ptr /*view*/)
{
}

inline void
GuiContext::update_gl (void)
{
    this->glw->repaint();
    //this->glw->paintGL();
}

inline void
GuiContext::request_context (void)
{
    this->glw->makeCurrent();
}

inline void
GuiContext::reset (void)
{
}

#endif /* UMVE_GUICONTEXT_HEADER */
