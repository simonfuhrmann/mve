/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef UMVE_GL_WIDGET_HEADER
#define UMVE_GL_WIDGET_HEADER

#include <set>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QTimer>

#include "ogl/context.h"

class GLWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    GLWidget(QWidget* parent = nullptr);
    ~GLWidget();

    void set_context (ogl::Context* context);

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

public slots:
    void repaint_gl (void);
    void repaint_async (void);
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
    qreal device_pixel_ratio;
    bool cx_init;
    std::set<ogl::Context*> init_set;
    QTimer* repaint_timer;
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
}

inline void
GLWidget::repaint_async (void)
{
    /* Don't issue an immediate repaint but let the timer trigger
     * a repaint after all events have been processed. */

    if (this->repaint_timer->isActive())
        return;

    this->repaint_timer->start();
}

inline void
GLWidget::gl_context (void)
{
    this->makeCurrent();
}

#endif /* UMVE_GL_WIDGET_HEADER */
