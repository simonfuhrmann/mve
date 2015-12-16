/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef SCROLL_IMAGE_HEADER
#define SCROLL_IMAGE_HEADER

#include <stdexcept>
#include <QScrollArea>

#include "clickimage.h"

#define ZOOM_IN_FACTOR (3.0 / 2.0)
#define ZOOM_OUT_FACTOR (2.0 / 3.0)
#define MOUSE_ZOOM_IN_FACTOR (5.0 / 4.0)
#define MOUSE_ZOOM_OUT_FACTOR (4.0 / 5.0)

class ScrollImage : public QScrollArea
{
    Q_OBJECT

private:
    ClickImage* image;
    double scale_factor;
    bool scale_contents;
    QPoint mouse_pos;

protected:
    void resizeEvent (QResizeEvent* event);
    void update_image_size (void);
    void max_image_size (void);
    void adjust_scrollbar (QScrollBar* bar, float factor);
    void move_scrollbar (QScrollBar* bar, int delta);

private slots:
    void mouse_moved (int x, int y, QMouseEvent* event);
    void mouse_clicked (int x, int y, QMouseEvent* event);
    void mouse_zoomed (int x, int y, QWheelEvent* event);

public:
    ScrollImage (void);

    void set_pixmap (QPixmap const& pixmap);
    QPixmap const* get_pixmap (void) const;
    ClickImage* get_image (void);

    void set_auto_scale (bool value);
    void set_scale_and_center (double factor);
    void set_scale (double factor);
    void reset_scale (void);
    void zoom_in (void);
    void zoom_out (void);
    double get_scale (void) const;

    void save_image (std::string const& filename);
};

/* ---------------------------------------------------------------- */

inline void
ScrollImage::set_pixmap (QPixmap const& pixmap)
{
    this->image->setPixmap(pixmap);
    this->update_image_size();
}

inline void
ScrollImage::set_auto_scale (bool value)
{
    this->scale_contents = value;
    this->update_image_size();
}

inline void
ScrollImage::reset_scale (void)
{
    double diff_factor = 1.0 / this->get_scale();

    this->image->set_scale_factor(1.0);
    this->scale_contents = false;
    //this->scale_factor = 1.0;
    //this->image_label->adjustSize();

    this->adjust_scrollbar(this->horizontalScrollBar(), diff_factor);
    this->adjust_scrollbar(this->verticalScrollBar(), diff_factor);
}

inline void
ScrollImage::zoom_out (void)
{
    this->set_scale_and_center(this->get_scale() * ZOOM_OUT_FACTOR);
    this->scale_contents = false;
}

inline void
ScrollImage::zoom_in (void)
{
    this->set_scale_and_center(this->get_scale() * ZOOM_IN_FACTOR);
    this->scale_contents = false;
}

inline double
ScrollImage::get_scale (void) const
{
    return this->image->get_scale_factor();
}

inline void
ScrollImage::set_scale (double scale)
{
    return this->image->set_scale_factor(scale);
}

inline QPixmap const*
ScrollImage::get_pixmap (void) const
{
    return this->image->pixmap();
}

inline ClickImage*
ScrollImage::get_image (void)
{
    return this->image;
}

#endif /* SCROLL_IMAGE_HEADER */
