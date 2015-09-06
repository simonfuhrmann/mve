/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>

#include <QFormLayout>
#include <QVBoxLayout>

#include "mve/image_base.h"
#include "mve/image.h"

#include "imageinspector.h"

ImageInspectorWidget::ImageInspectorWidget (void)
{
    this->label_clickpos = new QLabel();
    this->label_values = new QLabel();
    this->label_clickpos->setAlignment(Qt::AlignRight);
    this->label_values->setAlignment(Qt::AlignRight);
    this->inspect_x = -1;
    this->inspect_y = -1;

    this->image_widget1 = new ClickImage();
    this->image_widget2 = new ClickImage();
    this->image_widget3 = new ClickImage();
    this->reset_images();

    this->connect(this->image_widget1,
        SIGNAL(mouse_clicked(int, int, QMouseEvent*)),
        this, SLOT(on_large_image_clicked(int, int, QMouseEvent*)));
    this->connect(this->image_widget2,
        SIGNAL(mouse_clicked(int, int, QMouseEvent*)),
        this, SLOT(on_medium_image_clicked(int, int, QMouseEvent*)));
    this->connect(this->image_widget3,
        SIGNAL(mouse_clicked(int, int, QMouseEvent*)),
        this, SLOT(on_small_image_clicked(int, int, QMouseEvent*)));

    QFormLayout* info_layout = new QFormLayout();
    info_layout->setVerticalSpacing(0);
    info_layout->addRow("Position:", this->label_clickpos);
    info_layout->addRow("Values:", this->label_values);

    QVBoxLayout* main_layout = new QVBoxLayout();
    main_layout->addLayout(info_layout);
    main_layout->addWidget(this->image_widget1);
    main_layout->addWidget(this->image_widget2);
    main_layout->addWidget(this->image_widget3);
    main_layout->addSpacerItem(new QSpacerItem(0, 0,
        QSizePolicy::Minimum, QSizePolicy::Expanding));

    this->setLayout(main_layout);
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::set_image (mve::ByteImage::ConstPtr byte_image,
    mve::ImageBase::ConstPtr orig_image)
{
    this->byte_image = byte_image;
    this->orig_image = orig_image;
    if (this->byte_image->width() != this->orig_image->width()
        || this->byte_image->height() != this->orig_image->height())
        throw std::invalid_argument("Byte and original image don't match");

    int mag_x = this->inspect_x;
    int mag_y = this->inspect_y;
    this->reset_images();

    if (mag_x >= 0 && mag_y >= 0)
        this->magnify(mag_x, mag_y);
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::magnify (int x, int y)
{
    if (this->byte_image == nullptr || this->orig_image == nullptr)
        return;

    int const iw = this->byte_image->width();
    int const ih = this->byte_image->height();
    x = std::max(0, std::min(iw - 1, x));
    y = std::max(0, std::min(ih - 1, y));
    this->inspect_x = x;
    this->inspect_y = y;

    QImage image1 = this->get_magnified(x, y,
        MAGNIFY_LARGE_PATCH, MAGNIFY_LARGE_SCALE);
    QImage image2 = this->get_magnified(x, y,
        MAGNIFY_MEDIUM_PATCH, MAGNIFY_MEDIUM_SCALE);
    QImage image3 = this->get_magnified(x, y,
        MAGNIFY_SMALL_PATCH, MAGNIFY_SMALL_SCALE);

    this->update_value_label(x, y);

    this->image_widget1->setPixmap(QPixmap::fromImage(image1));
    this->image_widget2->setPixmap(QPixmap::fromImage(image2));
    this->image_widget3->setPixmap(QPixmap::fromImage(image3));
    this->label_clickpos->setText(tr("%1%2%3")
        .arg(tr("%1").arg(x), tr("x"), tr("%1").arg(y)));
}

/* ---------------------------------------------------------------- */

QImage
ImageInspectorWidget::get_magnified (int x, int y, int size, int scale)
{
    QImage img(size * scale, size * scale, QImage::Format_RGB32);
    img.fill(0xff00ff);

    int const iw = this->byte_image->width();
    int const ih = this->byte_image->height();
    int const left = x - size / 2;
    int const right = x + size / 2;
    int const top = y - size / 2;
    int const bottom = y + size / 2;

    int const safe_left = std::max(0, left);
    int const safe_right = std::min((int)(iw - 1), right);
    int const safe_top = std::max(0, top);
    int const safe_bottom = std::min((int)(ih - 1), bottom);

    for (int iy = safe_top; iy <= safe_bottom; ++iy)
        for (int ix = safe_left; ix <= safe_right; ++ix)
        {
            int rx = ix - left;
            int ry = iy - top;

            unsigned int color = this->get_image_color(ix, iy);
            for (int iiy = ry * scale; iiy < (ry + 1) * scale; ++iiy)
                for (int iix = rx * scale; iix < (rx + 1) * scale; ++iix)
                    img.setPixel(iix, iiy, color);
        }

    return img;
}

/* ---------------------------------------------------------------- */

unsigned int
ImageInspectorWidget::get_image_color (int x, int y)
{
    int const chans = this->byte_image->channels();
    unsigned int red, green, blue;
    if (chans == 1)
    {
        red = this->byte_image->at(x, y, 0);
        green = red;
        blue = red;
    }
    else if (chans == 2)
    {
        red = this->byte_image->at(x, y, 0);
        green = this->byte_image->at(x, y, 1);
        blue = 0;
    }
    else
    {
        red = this->byte_image->at(x, y, 0);
        green = this->byte_image->at(x, y, 1);
        blue = this->byte_image->at(x, y, 2);
    }
    return red << 16 | green << 8 | blue;
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::update_value_label (int x, int y)
{
    int const iw = this->orig_image->width();
    int const ic = this->orig_image->channels();
    int const off = y * iw * ic + x * ic;

    QString value_str;
    QString tooltip_str;
    if (this->orig_image->get_type() == mve::IMAGE_TYPE_FLOAT)
    {
        mve::FloatImage::ConstPtr img = std::dynamic_pointer_cast
            <mve::FloatImage const>(this->orig_image);
        for (int i = 0; i < ic; ++i)
        {
            if (ic <= 4 || i < 3)
                value_str += tr(" %1").arg(img->at(off + i), 0, 'f', 3);
            if (ic > 4 && i == 3)
                value_str += tr(" ...");
            if (ic > 4)
                tooltip_str += tr(" %1").arg(img->at(off + i), 0, 'f', 3);
        }
    }
    else if (this->orig_image->get_type() == mve::IMAGE_TYPE_UINT8)
    {
        mve::ByteImage::ConstPtr img = std::dynamic_pointer_cast
            <mve::ByteImage const>(this->orig_image);
        for (int i = 0; i < ic; ++i)
        {
            if (ic <= 4 || i < 3)
                value_str += tr(" %1").arg((int)img->at(off + i));
            if (ic > 4 && i == 3)
                value_str += tr(" ...");
            if (ic > 4)
                tooltip_str += tr(" %1").arg((int)img->at(off + i));
        }
    }
    else if (this->orig_image->get_type() == mve::IMAGE_TYPE_UINT16)
    {
        mve::RawImage::ConstPtr img = std::dynamic_pointer_cast
            <mve::RawImage const>(this->orig_image);
        for (int i = 0; i < ic; ++i)
        {
            if (ic <= 4 || i < 3)
                value_str += tr(" %1").arg((int)img->at(off + i));
            if (ic > 4 && i == 3)
                value_str += tr(" ...");
            if (ic > 4)
                tooltip_str += tr(" %1").arg((int)img->at(off + i));
        }
    }
    else
    {
        value_str = "unsupported";
        tooltip_str = "Unsupported image type!";
    }
    this->label_values->setText(value_str);
    this->label_values->setToolTip(tooltip_str);
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::reset_images (void)
{
    int size1 = MAGNIFY_SMALL_PATCH * MAGNIFY_SMALL_SCALE;
    int size2 = MAGNIFY_MEDIUM_PATCH * MAGNIFY_MEDIUM_SCALE;
    int size3 = MAGNIFY_LARGE_PATCH * MAGNIFY_LARGE_SCALE;
    QImage image1(size1, size1, QImage::Format_RGB32);
    QImage image2(size2, size2, QImage::Format_RGB32);
    QImage image3(size3, size3, QImage::Format_RGB32);
    image1.fill(0xff00ff);
    image2.fill(0xff00ff);
    image3.fill(0xff00ff);

    this->image_widget1->setPixmap(QPixmap::fromImage(image1));
    this->image_widget2->setPixmap(QPixmap::fromImage(image2));
    this->image_widget3->setPixmap(QPixmap::fromImage(image3));

    this->label_clickpos->setText(tr("--"));
    this->label_values->setText(tr("--"));
    this->inspect_x = -1;
    this->inspect_y = -1;
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::on_large_image_clicked (int x, int y, QMouseEvent* ev)
{
    this->image_click(x, y, ev, MAGNIFY_LARGE_SCALE, MAGNIFY_LARGE_PATCH);
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::on_medium_image_clicked (int x, int y, QMouseEvent* ev)
{
    this->image_click(x, y, ev, MAGNIFY_MEDIUM_SCALE, MAGNIFY_MEDIUM_PATCH);
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::on_small_image_clicked (int x, int y, QMouseEvent* ev)
{
    this->image_click(x, y, ev, MAGNIFY_SMALL_SCALE, MAGNIFY_SMALL_PATCH);
}

/* ---------------------------------------------------------------- */

void
ImageInspectorWidget::image_click (int x, int y,
    QMouseEvent* ev, int scale, int size)
{
    if (this->inspect_x < 0 || this->inspect_y < 0)
        return;
    if (this->byte_image == nullptr || this->orig_image == nullptr)
        return;

    //std::cout << "Click at " << x << "," << y << std::endl;

    int const off_x = (x / scale) - size / 2;
    int const off_y = (y / scale) - size / 2;
    if (ev->buttons() & Qt::RightButton)
    {
        this->magnify(this->inspect_x + off_x, this->inspect_y + off_y);
    }
    else
    {
        this->update_value_label(this->inspect_x + off_x,
            this->inspect_y + off_y);
    }
}
