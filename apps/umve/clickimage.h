/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef CLICK_IMAGE_HEADER
#define CLICK_IMAGE_HEADER

#include <QLabel>
#include <QMouseEvent>

/* Scalable image that features click signals. */
class ClickImage : public QLabel
{
    Q_OBJECT

private:
    double scale_factor;

protected:
    void mousePressEvent (QMouseEvent* event);
    void mouseMoveEvent (QMouseEvent* event);
    void mouseReleaseEvent (QMouseEvent* event);
    void wheelEvent (QWheelEvent* event);

signals:
    void mouse_clicked (int x, int y, QMouseEvent* event);
    void mouse_released (int x, int y, QMouseEvent* event);
    void mouse_moved (int x, int y, QMouseEvent* event);
    void mouse_zoomed (int x, int y, QWheelEvent* event);

public:
    ClickImage (void);
    ~ClickImage (void);

    double get_scale_factor (void) const;
    void set_scale_factor (double factor);
    void update_size (void);
    QPoint get_image_coordinates (QPoint pnt) const;
};

/* ---------------------------------------------------------------- */

inline
ClickImage::ClickImage (void)
{
    this->scale_factor = 1.0;
}

inline
ClickImage::~ClickImage (void)
{
}

inline double
ClickImage::get_scale_factor (void) const
{
    return this->scale_factor;
}

inline void
ClickImage::set_scale_factor (double factor)
{
    this->scale_factor = factor;
    if (this->scale_factor == 1.0)
        this->adjustSize();
    else
        this->resize(this->scale_factor * this->pixmap()->size());
}

inline void
ClickImage::update_size (void)
{
    this->set_scale_factor(this->scale_factor);
}

inline void
ClickImage::mousePressEvent (QMouseEvent* event)
{
    QPoint const& pnt = this->get_image_coordinates(event->pos());
    emit this->mouse_clicked(pnt.x(), pnt.y(), event);
}

inline void
ClickImage::mouseReleaseEvent (QMouseEvent* event)
{
    QPoint const& pnt = this->get_image_coordinates(event->pos());
    emit this->mouse_released(pnt.x(), pnt.y(), event);
}

inline void
ClickImage::mouseMoveEvent (QMouseEvent* event)
{
    QPoint const& pnt = this->get_image_coordinates(event->pos());
    emit this->mouse_moved(pnt.x(), pnt.y(), event);
}

inline void
ClickImage::wheelEvent (QWheelEvent* event)
{
    QPoint const& pnt = this->get_image_coordinates(event->pos());
    emit this->mouse_zoomed(pnt.x(), pnt.y(), event);
}

inline QPoint
ClickImage::get_image_coordinates (QPoint pnt) const
{
    if (this->hasScaledContents())
    {
        QSize ps(this->pixmap()->size());
        QSize ws(this->size());

        float scale_x = (float)ps.width() / (float)ws.width();
        float scale_y = (float)ps.height() / (float)ws.height();

        pnt.setX((int)((float)pnt.x() * scale_x));
        pnt.setY((int)((float)pnt.y() * scale_y));
    }

    return pnt;
}

#endif /* CLICK_IMAGE_HEADER */
