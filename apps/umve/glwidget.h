#ifndef UMVE_GL_WIDGET_HEADER
#define UMVE_GL_WIDGET_HEADER

#include <set>
#include <QGLWidget>
#include <QMouseEvent>

#include "ogl/context.h"

class GLWidget : public QGLWidget
{
    Q_OBJECT

public:
    GLWidget(QWidget* parent = NULL);
    ~GLWidget();

    void set_context (ogl::Context* context);

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

public slots:
    void repaint_gl (void);
    void gl_context (void);

public:
    static void debug_event (ogl::MouseEvent const& event);
    static void debug_event (ogl::KeyboardEvent const& event);

protected:
    void initializeGL (void);
    void paintGL (void);
    void resizeGL (int width, int height);

    void mousePressEvent (QMouseEvent *event);
    void mouseReleaseEvent (QMouseEvent *event);
    void mouseMoveEvent (QMouseEvent *event);
    void keyPressEvent (QKeyEvent* event);
    void keyReleaseEvent (QKeyEvent* event);
    void wheelEvent (QWheelEvent* event);

private:
    ogl::Context* context;
    int gl_width;
    int gl_height;
    bool cx_init;
    std::set<ogl::Context*> init_set;
};

/* ---------------------------------------------------------------- */

inline QSize
GLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

inline QSize
GLWidget::sizeHint() const
{
    return QSize(400, 400);
}

inline void
GLWidget::repaint_gl (void)
{
    this->updateGL();
    //this->repaint();
}

inline void
GLWidget::gl_context (void)
{
    this->makeCurrent();
}

#endif /* UMVE_GL_WIDGET_HEADER */
