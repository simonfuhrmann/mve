/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>
#include <QScrollBar>

#include "scrollimage.h"

ScrollImage::ScrollImage (void)
{
    /* Create image label. */
    this->image = new ClickImage();
    this->image->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    this->image->setScaledContents(true);

    /* Init scroll area. */
    this->setBackgroundRole(QPalette::Dark);
    this->setFrameStyle(QFrame::Box | QFrame::Sunken);
    this->setAlignment(Qt::AlignCenter);
    this->setWidget(this->image);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    /* Init members. */
    this->scale_contents = true;

    this->connect(this->image, SIGNAL(mouse_clicked(int, int, QMouseEvent*)),
        this, SLOT(mouse_clicked(int, int, QMouseEvent*)));
    this->connect(this->image, SIGNAL(mouse_moved(int, int, QMouseEvent*)),
        this, SLOT(mouse_moved(int, int, QMouseEvent*)));
    this->connect(this->image, SIGNAL(mouse_zoomed(int, int, QWheelEvent*)),
        this, SLOT(mouse_zoomed(int, int, QWheelEvent*)));
}

/* ---------------------------------------------------------------- */

void
ScrollImage::update_image_size (void)
{
    if (this->scale_contents)
        this->max_image_size();
    else
        this->image->update_size();
}

/* ---------------------------------------------------------------- */

void
ScrollImage::max_image_size (void)
{
    QSize imgsize(this->image->pixmap()->size());
    QSize newsize = imgsize.scaled(this->maximumViewportSize(),
        Qt::KeepAspectRatio);
    this->image->set_scale_factor(static_cast<float>(newsize.width())
        / imgsize.width());
    this->image->resize(newsize);
}

/* ---------------------------------------------------------------- */

void
ScrollImage::set_scale_and_center (double factor)
{
    double diff_factor = factor / this->get_scale();
    this->set_scale(factor);

    this->adjust_scrollbar(this->horizontalScrollBar(), diff_factor);
    this->adjust_scrollbar(this->verticalScrollBar(), diff_factor);
}

/* ---------------------------------------------------------------- */

void
ScrollImage::resizeEvent (QResizeEvent* event)
{
    this->QScrollArea::resizeEvent(event);
    //std::cout << "Resize Event: " << event->size().width() << "x"
    //    << event->size().height() << std::endl;

    if (this->scale_contents && this->image->pixmap())
        this->max_image_size();
}

/* ---------------------------------------------------------------- */

void
ScrollImage::mouse_clicked (int, int, QMouseEvent* event)
{
    if (event->button() & Qt::LeftButton)
        mouse_pos = event->pos();
}

/* ---------------------------------------------------------------- */

void
ScrollImage::mouse_moved (int, int, QMouseEvent* event)
{
    if (event->buttons() ^ Qt::LeftButton)
        return;
    QPoint diff = mouse_pos - event->pos();
    this->move_scrollbar(this->horizontalScrollBar(), diff.x());
    this->move_scrollbar(this->verticalScrollBar(), diff.y());
}

/* ---------------------------------------------------------------- */

void
ScrollImage::mouse_zoomed (int x, int y, QWheelEvent* event)
{
    if (!(event->modifiers() & Qt::ControlModifier))
    {
        event->ignore();
        return;
    }
    QPoint old_pnt(x, y);
    if (event->delta() > 0)
        this->set_scale(this->get_scale() * MOUSE_ZOOM_IN_FACTOR);
    else
        this->set_scale(this->get_scale() * MOUSE_ZOOM_OUT_FACTOR);

    QPoint new_pnt = this->image->get_image_coordinates(event->pos());
    QPoint diff = (old_pnt - new_pnt) * this->get_scale();
    this->move_scrollbar(this->horizontalScrollBar(), diff.x());
    this->move_scrollbar(this->verticalScrollBar(), diff.y());
    this->scale_contents = false;
}

/* ---------------------------------------------------------------- */

void
ScrollImage::adjust_scrollbar (QScrollBar* bar, float factor)
{
    bar->setValue((int)(factor * bar->value()
        + ((factor - 1) * bar->pageStep() / 2)));
}

void
ScrollImage::move_scrollbar (QScrollBar* bar, int delta)
{
    bar->setValue(bar->value() + delta);
}

/* ---------------------------------------------------------------- */

void
ScrollImage::save_image (std::string const& filename)
{
    QPixmap const* pm = this->image->pixmap();
    if (!pm)
        throw std::runtime_error("No image set");
    bool ret = pm->save(filename.c_str());
    if (!ret)
        throw std::runtime_error("Unable to save image");
}
