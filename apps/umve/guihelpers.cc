/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <QApplication>
#include <QFileDialog>
#include <QFormLayout>
#include <QPainter>
#include <QStyleFactory>

#include "util/file_system.h"

#include "guihelpers.h"

QPixmap
get_pixmap_from_image (mve::ByteImage::ConstPtr img)
{
    std::size_t iw = img->width();
    std::size_t ih = img->height();
    std::size_t ic = img->channels();
    bool is_gray = (ic == 1 || ic == 2);
    bool has_alpha = (ic == 2 || ic == 4);

    QImage img_qimage(iw, ih, QImage::Format_ARGB32);
    {
        QPainter painter(&img_qimage);
        std::size_t inpos = 0;
        for (std::size_t y = 0; y < ih; ++y)
            for (std::size_t x = 0; x < iw; ++x)
            {
                unsigned char alpha = has_alpha
                    ? img->at(inpos + 1 + !is_gray * 2)
                    : 255;
                unsigned int argb
                    = (alpha << 24)
                    | (img->at(inpos) << 16)
                    | (img->at(inpos + !is_gray * 1) << 8)
                    | (img->at(inpos + !is_gray * 2) << 0);

                img_qimage.setPixel(x, y, argb);
                inpos += ic;
            }
    }

    return QPixmap::fromImage(img_qimage);
}

/* ---------------------------------------------------------------- */

QWidget*
get_separator (void)
{
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    return line;
}

/* ---------------------------------------------------------------- */

QWidget*
get_expander (void)
{
    QWidget* expander = new QWidget();
    expander->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return expander;
}

/* ---------------------------------------------------------------- */

void
set_qt_style (char const* style_name)
{
#if 0
    /* Output all QT styles as string... */
    std::cout << "Available QT styles: ";
    QStringList styles = QStyleFactory::keys();
    for (int i = 0; i < styles.size(); ++i)
    {
        if (i != 0) std::cout << ", ";
        std::cout << styles[i].toStdString();
    }
    std::cout << std::endl;
#endif

    QStyle* style = QStyleFactory::create(style_name);
    if (style != nullptr)
        QApplication::setStyle(style);
}

/* ---------------------------------------------------------------- */

QWidget*
get_wrapper (QLayout* layout, int margin)
{
    layout->setContentsMargins(margin, margin, margin, margin);
    QWidget* ret = new QWidget();
    ret->setLayout(layout);
    return ret;
}

/* ---------------------------------------------------------------- */

PlyExportDialog::PlyExportDialog (mve::View::Ptr view, QWidget* parent)
    : QDialog(parent)
{
    this->selected_view.set_view(view);

    this->selected_view.fill_embeddings(this->depthmap_cb,
        mve::IMAGE_TYPE_FLOAT, "depthmap");
    this->selected_view.fill_embeddings(this->confidence_cb,
        mve::IMAGE_TYPE_FLOAT, "confidence");
    this->selected_view.fill_embeddings(this->colorimage_cb,
        mve::IMAGE_TYPE_UINT8, "undistorted");

    QLabel* message = new QLabel(tr("Please enter the names of the "
        "view embeddings for export. Leave the text fields empty "
        "to not incorporate the data in the export. Only the "
        "depthmap is requried."));
    message->setWordWrap(true);

    QPushButton* cancel_but = new QPushButton
        (/*QIcon::fromTheme("process-stop"),*/ tr("Cancel"));
    QPushButton* ok_but = new QPushButton
        (/*QIcon::fromTheme("go-next"),*/ tr("Ok"));
    QBoxLayout* buttons = new QHBoxLayout();
    buttons->addWidget(cancel_but);
    buttons->addWidget(ok_but);
    this->connect(cancel_but, SIGNAL(clicked()), this, SLOT(reject()));
    this->connect(ok_but, SIGNAL(clicked()), this, SLOT(accept()));

    QFormLayout* form = new QFormLayout(this);
    form->addRow(&this->selected_view);
    form->addRow(message);
    form->addRow("Depthmap:", &this->depthmap_cb);
    form->addRow("Confidence:", &this->confidence_cb);
    form->addRow("Color image:", &this->colorimage_cb);
    form->addRow(buttons);
    this->setWindowTitle("Export PLY");
}

void
PlyExportDialog::accept (void)
{
    this->depthmap = this->depthmap_cb.currentText().toStdString();
    this->confidence = this->confidence_cb.currentText().toStdString();
    this->colorimage = this->colorimage_cb.currentText().toStdString();
    this->QDialog::accept();
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

QCollapsible::QCollapsible (QString title, QWidget* content)
    : content(content)
{
    QLabel* label = new QLabel(title);
    this->collapse_but = new QPushButton();
    this->collapse_but->setIcon(QIcon(":/images/icon_large_minus.png"));
    this->collapse_but->setIconSize(QSize(13, 13));
    this->collapse_but->setFlat(true);
    this->collapse_but->setMinimumSize(17, 17);
    this->collapse_but->setMaximumSize(17, 17);

    QHBoxLayout* header_layout = new QHBoxLayout();
    header_layout->setContentsMargins(QMargins(0, 0, 0, 0));
    header_layout->setSpacing(5);
    header_layout->addWidget(collapse_but, 0);
    header_layout->addWidget(get_separator(), 1);
    header_layout->addWidget(label, 0);

    this->content_indent = new QWidget();
    this->content_indent->setFixedSize(0, 0);
    QHBoxLayout* content_layout = new QHBoxLayout();
    content_layout->setContentsMargins(QMargins(0, 0, 0, 0));
    content_layout->setSpacing(0);
    content_layout->addWidget(this->content_indent);
    content_layout->addWidget(this->content);
    this->content_wrapper = new QWidget();
    this->content_wrapper->setLayout(content_layout);

    QVBoxLayout* main_layout = new QVBoxLayout();
    main_layout->setContentsMargins(QMargins(0, 0, 0, 0));
    main_layout->setSpacing(5);
    main_layout->addLayout(header_layout);
    main_layout->addWidget(this->content_wrapper);

    this->setLayout(main_layout);
    this->connect(collapse_but, SIGNAL(clicked()),
        this, SLOT(on_toggle_collapse()));
}

void
QCollapsible::on_toggle_collapse (void)
{
    this->set_collapsed(this->content_wrapper->isVisible() ? true : false);
}

void
QCollapsible::set_collapsed (bool value)
{
    this->content_wrapper->setVisible(!value);
    this->collapse_but->setIcon(value
        ? QIcon(":/images/icon_large_plus.png")
        : QIcon(":/images/icon_large_minus.png"));
}

void
QCollapsible::set_collapsible (bool value)
{
    if (!value)
        this->set_collapsed(false);
    this->collapse_but->setEnabled(value);
}

void
QCollapsible::set_content_indent(int pixels)
{
    this->content_indent->setFixedSize(pixels, 1);
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

FileSelector::FileSelector (QWidget* parent)
    : QPushButton(parent)
{
    this->dironly = false;
    this->setIconSize(QSize(18, 18));
    this->setText("<none>");
    this->setIcon(QIcon(":/images/icon_open_file.svg"));
    this->setStyleSheet("text-align: left");
    this->connect(this, SIGNAL(clicked()),
        this, SLOT(on_clicked()));
}

void
FileSelector::set_directory_mode (void)
{
    this->dironly = true;
}

void
FileSelector::set_ellipsize (std::size_t chars)
{
    this->ellipsize = chars;
}

std::string const&
FileSelector::get_filename (void) const
{
    return this->filename;
}

void
FileSelector::on_clicked (void)
{
    QString filename;
    if (this->dironly)
        filename = QFileDialog::getExistingDirectory(this, "Select directory...");
    else
        filename = QFileDialog::getOpenFileName(this, "Select file...");

    if (filename.isEmpty())
        return;

    this->filename = filename.toStdString();
    std::string fname = util::fs::basename(this->filename);
    if (this->ellipsize > 0)
        fname = util::string::ellipsize(fname, this->ellipsize, 1);

    this->setText(QString(fname.c_str()));
    if (this->dironly)
        this->setIcon(QIcon(":/images/icon_folder.svg"));
    else
        this->setIcon(QIcon(":/images/icon_file.svg"));
}
