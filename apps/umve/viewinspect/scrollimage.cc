#include <iostream>

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
    this->scale_contents = false;
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
    imgsize.scale(this->maximumViewportSize(), Qt::KeepAspectRatio);
    this->image->resize(imgsize);
}

/* ---------------------------------------------------------------- */

void
ScrollImage::set_scale (double factor)
{
    double diff_factor = factor / this->get_scale();
    this->image->set_scale_factor(factor);

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

    if (!this->scale_contents)
        return;

    QSize imgsize(this->image->pixmap()->size());
    imgsize.scale(event->size(), Qt::KeepAspectRatio);
    this->image->resize(imgsize);
}

/* ---------------------------------------------------------------- */

void
ScrollImage::adjust_scrollbar (QScrollBar* bar, float factor)
{
    bar->setValue((int)(factor * bar->value()
        + ((factor - 1) * bar->pageStep() / 2)));
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
