/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <QVBoxLayout>

#include "selectedview.h"

SelectedView::SelectedView (void)
{
    this->viewname = new QLabel();
    this->image = new QLabel();
    this->image->setBaseSize(QSize(50, 50));
    this->viewinfo = new QLabel();
    //this->embeddings = new QComboBox();

    QVBoxLayout* label_box = new QVBoxLayout();
    label_box->setSpacing(0);
    label_box->addWidget(this->viewname, 0);
    label_box->addWidget(this->viewinfo, 0);
    //label_box->addStretch(1);
    //label_box->addWidget(this->embeddings);

    QHBoxLayout* image_labels_box = new QHBoxLayout();
    image_labels_box->addWidget(this->image, 0);
    image_labels_box->addLayout(label_box, 1);

    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(QMargins(0,0,0,0));
    main_layout->addLayout(image_labels_box);

    this->reset_view();
}

/* ---------------------------------------------------------------- */

void
SelectedView::set_view (mve::View::Ptr view)
{
    if (view == nullptr)
    {
        this->reset_view();
        return;
    }

    this->view = view;
    this->viewname->setText(("View: " + view->get_name()).c_str());
    std::stringstream ss;
    ss << view->get_images().size() << " images, ";
    ss << view->get_blobs().size() << " BLOBs.";
    this->viewinfo->setText(ss.str().c_str());

    mve::ByteImage::Ptr img(view->get_byte_image("thumbnail"));
    if (img != nullptr)
    {
        std::size_t iw(img->width());
        std::size_t ih(img->height());
        QImage qimg(iw, ih, QImage::Format_RGB32);
        for (std::size_t y = 0; y < ih; ++y)
            for (std::size_t x = 0; x < iw; ++x)
            {
                unsigned int vr = (unsigned int)(img->at(x, y, 0));
                unsigned int vg = (unsigned int)(img->at(x, y, 1));
                unsigned int vb = (unsigned int)(img->at(x, y, 2));
                unsigned int rgb = vr << 16 | vg << 8 | vb;
                qimg.setPixel(x, y, rgb);
            }

        this->image->setPixmap(QPixmap::fromImage(qimg));
    }

    emit this->view_selected(this->view);
}

/* ---------------------------------------------------------------- */

void
SelectedView::fill_embeddings (QComboBox& cb, mve::ImageType type,
    std::string const& default_name) const
{
    if (this->view == nullptr)
        return;

    cb.clear();
    cb.addItem("<none>");

    std::vector<std::string> names;
    mve::View::ImageProxies const& proxies = this->view->get_images();
    for (std::size_t i = 0; i < proxies.size(); ++i)
    {
        mve::View::ImageProxy const* proxy = this->view->get_image_proxy(proxies[i].name);
        if (proxy->type != type)
            continue;
        names.push_back(proxy->name);
    }
    std::sort(names.begin(), names.end());

    for (std::size_t i = 0; i < names.size(); ++i)
    {
        cb.addItem(names[i].c_str());
        if (names[i] == default_name)
            cb.setCurrentIndex(cb.count() - 1);
    }
}

/* ---------------------------------------------------------------- */

void
SelectedView::reset_view (void)
{
    this->view.reset();
    this->viewname->setText("<no view selected>");
    this->viewinfo->setText("");
    this->image->setPixmap(QPixmap(":/images/icon_broken.svg"));
    emit this->view_selected(this->view);
}
