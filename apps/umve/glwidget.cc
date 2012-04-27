#include <ctime>
#include <iostream>

#include "ogl/events.h"
#include "glwidget.h"

GLWidget::GLWidget (QWidget *parent)
    : QGLWidget(parent)
    , context(0)
    , gl_width(0)
    , gl_height(0)
    , cx_init(false)
{
    this->setFocusPolicy(Qt::ClickFocus);
}

/* ---------------------------------------------------------------- */

GLWidget::~GLWidget (void)
{
}

/* ---------------------------------------------------------------- */

void
GLWidget::initializeGL()
{
}

/* ---------------------------------------------------------------- */

void
GLWidget::resizeGL(int width, int height)
{
    this->gl_width = width;
    this->gl_height = height;
    if (this->context != 0)
        this->context->resize(width, height);
}

/* ---------------------------------------------------------------- */

void
GLWidget::paintGL()
{
    if (this->context == 0)
        return;

    /* Current context may need initialization. */
    if (this->cx_init)
    {
        if (this->init_set.find(this->context) == this->init_set.end())
        {
            this->context->init();
            this->context->resize(this->gl_width, this->gl_height);
            this->init_set.insert(this->context); // Mark initialized
        }
        this->cx_init = false;
    }

    /* Paint it! */
    this->context->paint();
}

/* ---------------------------------------------------------------- */

void
GLWidget::set_context (ogl::Context* context)
{
    this->context = context;
    this->cx_init = true;
}

/* ---------------------------------------------------------------- */

void
GLWidget::mousePressEvent (QMouseEvent *event)
{
    ogl::MouseEvent e;
    e.type = ogl::MOUSE_EVENT_PRESS;
    e.button = (ogl::MouseButton)event->button();
    e.button_mask = event->buttons();
    e.x = event->x();
    e.y = event->y();
    this->context->mouse_event(e);
    this->repaint();
}

/* ---------------------------------------------------------------- */

void
GLWidget::mouseReleaseEvent (QMouseEvent *event)
{
    ogl::MouseEvent e;
    e.type = ogl::MOUSE_EVENT_RELEASE;
    e.button = (ogl::MouseButton)event->button();
    e.button_mask = event->buttons();
    e.x = event->x();
    e.y = event->y();
    this->context->mouse_event(e);
    this->repaint();
}

/* ---------------------------------------------------------------- */

void
GLWidget::mouseMoveEvent (QMouseEvent *event)
{
    ogl::MouseEvent e;
    e.type = ogl::MOUSE_EVENT_MOVE;
    e.button = (ogl::MouseButton)event->button();
    e.button_mask = event->buttons();
    e.x = event->x();
    e.y = event->y();
    this->context->mouse_event(e);
    this->repaint();
}

/* ---------------------------------------------------------------- */

void
GLWidget::wheelEvent (QWheelEvent* event)
{
    ogl::MouseEvent e;
    if (event->delta() < 0)
        e.type = ogl::MOUSE_EVENT_WHEEL_DOWN;
    else
        e.type = ogl::MOUSE_EVENT_WHEEL_UP;
    e.button = ogl::MOUSE_BUTTON_NONE;
    e.button_mask = event->buttons();
    e.x = event->x();
    e.y = event->y();
    this->context->mouse_event(e);
    this->repaint();
}

/* ---------------------------------------------------------------- */

void
GLWidget::keyPressEvent (QKeyEvent* event)
{
    if (event->isAutoRepeat())
    {
        this->QWidget::keyPressEvent(event);
        return;
    }

    ogl::KeyboardEvent e;
    e.type = ogl::KEYBOARD_EVENT_PRESS;
    e.keycode = event->key();
    this->context->keyboard_event(e);
    this->repaint();
}

/* ---------------------------------------------------------------- */

void
GLWidget::keyReleaseEvent (QKeyEvent* event)
{
    if (event->isAutoRepeat())
    {
        this->QWidget::keyReleaseEvent(event);
        return;
    }

    ogl::KeyboardEvent e;
    e.type = ogl::KEYBOARD_EVENT_RELEASE;
    e.keycode = event->key();
    // FIXME: HACK
    if (e.keycode == -1)
        e.keycode = 0x01000020; // KEY_SHIFT
    this->context->keyboard_event(e);
    this->repaint();
}
