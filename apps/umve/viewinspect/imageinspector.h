/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef IMAGE_INSPECTOR_HEADER
#define IMAGE_INSPECTOR_HEADER

#include "mve/image.h"

#include "clickimage.h"

#if 0
#   define MAGNIFY_LARGE_PATCH 35
#   define MAGNIFY_LARGE_SCALE 3
#   define MAGNIFY_MEDIUM_PATCH 15
#   define MAGNIFY_MEDIUM_SCALE 7
#   define MAGNIFY_SMALL_PATCH 5
#   define MAGNIFY_SMALL_SCALE 21
#else
#   define MAGNIFY_LARGE_PATCH 55
#   define MAGNIFY_LARGE_SCALE 3
#   define MAGNIFY_MEDIUM_PATCH 15
#   define MAGNIFY_MEDIUM_SCALE 11
#   define MAGNIFY_SMALL_PATCH 5
#   define MAGNIFY_SMALL_SCALE 33
#endif

class ImageInspectorWidget : public QWidget
{
    Q_OBJECT

private:
    mve::ByteImage::ConstPtr byte_image;
    mve::ImageBase::ConstPtr orig_image;

    QLabel* label_dimension;
    QLabel* label_channels;
    QLabel* label_memory;
    QLabel* label_clickpos;
    QLabel* label_values;

    ClickImage* image_widget1;
    ClickImage* image_widget2;
    ClickImage* image_widget3;

    int inspect_x, inspect_y;

private slots:
    void on_large_image_clicked (int x, int y, QMouseEvent* event);
    void on_medium_image_clicked (int x, int y, QMouseEvent* event);
    void on_small_image_clicked (int x, int y, QMouseEvent* event);

private:
    QImage get_magnified (int x, int y, int size, int scale);
    unsigned int get_image_color (int x, int y);
    void update_value_label (int x, int y);
    void reset_images (void);
    void image_click (int x, int y, QMouseEvent* event, int scale, int size);

public:
    ImageInspectorWidget (void);

    void set_image (mve::ByteImage::ConstPtr byte_image,
        mve::ImageBase::ConstPtr orig_image);
    void magnify (int x, int y);
    void reset (void);
};

/* ---------------------------------------------------------------- */

inline void
ImageInspectorWidget::reset (void)
{
    this->reset_images();
    this->byte_image.reset();
    this->orig_image.reset();
}

#endif /* IMAGE_INSPECTOR_HEADER */
