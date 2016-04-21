/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>
#include <limits>
#include <QAction>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "mve/view.h"
#include "mve/image.h"
#include "mve/mesh_io_ply.h"
#include "mve/image_exif.h"
#include "mve/image_io.h"
#include "mve/image_tools.h"

#include "guihelpers.h"
#include "scenemanager.h"
#include "viewinspect.h"

#if QT_VERSION >= 0x050000
#   include <QWindow>
#endif

ViewInspect::ViewInspect (QWidget* parent)
    : MainWindowTab(parent)
{
    this->scroll_image = new ScrollImage();
    this->embeddings = new QComboBox();
    this->embeddings->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    this->embeddings->setEditable(false);
    this->embeddings->setEnabled(false);

    this->label_name = new QLabel("");
    this->label_dimension = new QLabel("--");
    this->label_memory = new QLabel("--");

    /* Create GUIs, actions & menus. */
    this->create_detail_frame();
    this->create_actions();
    this->update_actions();
    this->create_menus();

    /* Connect signals. */
    this->connect(this->embeddings, SIGNAL(activated(QString const&)),
        this, SLOT(on_embedding_selected(QString const&)));
    this->connect(this->scroll_image->get_image(),
        SIGNAL(mouse_clicked(int, int, QMouseEvent*)),
        this, SLOT(on_image_clicked(int, int, QMouseEvent*)));

    this->connect(this, SIGNAL(tab_activated()), SLOT(on_tab_activated()));

    this->connect(&SceneManager::get(), SIGNAL(scene_selected(mve::Scene::Ptr)),
        this, SLOT(on_scene_selected(mve::Scene::Ptr)));
    this->connect(&SceneManager::get(), SIGNAL(view_selected(mve::View::Ptr)),
        this, SLOT(on_view_selected(mve::View::Ptr)));

    /* Setup layout. */
    QHBoxLayout* head_layout = new QHBoxLayout();
    head_layout->addWidget(this->embeddings);
    head_layout->addSpacerItem(new QSpacerItem(10, 0));
    head_layout->addWidget(this->label_name);
    head_layout->addSpacerItem(new QSpacerItem(0, 0,
        QSizePolicy::Expanding, QSizePolicy::Minimum));
    head_layout->addWidget(new QLabel(tr("Dimension:")));
    head_layout->addWidget(this->label_dimension);
    head_layout->addSpacerItem(new QSpacerItem(10, 0));
    head_layout->addWidget(new QLabel(tr("Memory:")));
    head_layout->addWidget(this->label_memory);

    QVBoxLayout* image_layout = new QVBoxLayout();
    image_layout->addWidget(this->toolbar);
    image_layout->addLayout(head_layout);
    image_layout->addWidget(this->scroll_image, 1);

    QHBoxLayout* main_layout = new QHBoxLayout(this);
    main_layout->addLayout(image_layout, 1);
    main_layout->addWidget(this->image_details);
}

/* ---------------------------------------------------------------- */

void
ViewInspect::create_detail_frame (void)
{
    this->operations = new ImageOperationsWidget();
    this->inspector = new ImageInspectorWidget();
    this->tone_mapping = new ToneMapping();
    this->exif_viewer = new QTextEdit();
    this->exif_viewer->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    this->populate_exif_viewer();

    this->connect(this->tone_mapping, SIGNAL(tone_mapping_changed()),
		this, SLOT(on_image_changed()));
    this->connect(this->operations, SIGNAL(signal_reload_embeddings()),
        this, SLOT(on_reload_embeddings()));
    this->connect(this->operations, SIGNAL(signal_select_embedding
        (QString const&)), this, SLOT(on_embedding_selected(QString const&)));

    this->image_details = new QTabWidget();
    this->image_details->setTabPosition(QTabWidget::East);
    this->image_details->addTab(this->operations, tr("Operations"));
    this->image_details->addTab(this->inspector, tr("Image Inspector"));
    this->image_details->addTab(this->tone_mapping, tr("Tone Mapping"));
    this->image_details->addTab(this->exif_viewer, tr("EXIF"));
    this->image_details->hide();
}

/* ---------------------------------------------------------------- */

void
ViewInspect::create_actions (void)
{
    this->action_open = new QAction(QIcon(":/images/icon_open_file.svg"),
        tr("Open view/image"), this);
    this->connect(this->action_open, SIGNAL(triggered()),
        this, SLOT(on_open()));

    this->action_reload = new QAction(QIcon(":/images/icon_revert.svg"),
        tr("Reload file"), this);
    this->connect(this->action_reload, SIGNAL(triggered()),
        this, SLOT(on_view_reload()));

    this->action_save_view = new QAction(QIcon(":/images/icon_save.svg"),
        tr("Save view"), this);
    this->connect(this->action_save_view, SIGNAL(triggered()),
        this, SLOT(on_save_view()));

    this->action_export_ply = new QAction
        (QIcon(":/images/icon_export_ply.svg"),
        tr("Export Scanalize PLY"), this);
    this->connect(this->action_export_ply, SIGNAL(triggered()),
        this, SLOT(on_ply_export()));

    this->action_export_image = new QAction(QIcon
        (":/images/icon_screenshot.svg"), tr("Export Image"), this);
    this->connect(this->action_export_image, SIGNAL(triggered()),
        this, SLOT(on_image_export()));

    this->action_zoom_in = new QAction(QIcon(":/images/icon_zoom_in.svg"),
        tr("Zoom &In"), this);
    this->action_zoom_in->setShortcut(tr("Ctrl++"));
    //this->action_zoom_in->setEnabled(false);
    this->connect(this->action_zoom_in, SIGNAL(triggered()),
        this, SLOT(on_zoom_in()));

    this->action_zoom_out = new QAction(QIcon(":/images/icon_zoom_out.svg"),
        tr("Zoom &Out"), this);
    this->action_zoom_out->setShortcut(tr("Ctrl+-"));
    //this->action_zoom_out->setEnabled(false);
    this->connect(this->action_zoom_out, SIGNAL(triggered()),
        this, SLOT(on_zoom_out()));

    this->action_zoom_reset = new QAction(QIcon(":/images/icon_zoom_reset.svg"),
        tr("&Reset Size"), this);
    this->action_zoom_reset->setShortcut(tr("Ctrl+0"));
    this->connect(this->action_zoom_reset, SIGNAL(triggered()),
        this, SLOT(on_normal_size()));

    this->action_zoom_fit = new QAction(QIcon(":/images/icon_zoom_page.svg"),
        tr("&Fit to Window"), this);
    this->action_zoom_fit->setShortcut(tr("Ctrl+1"));
    this->connect(this->action_zoom_fit, SIGNAL(triggered()),
        this, SLOT(on_fit_to_window()));

    this->action_show_details = new QAction
        (QIcon(":/images/icon_toolbox.svg"),
        tr("Show &Details"), this);
    this->action_show_details->setCheckable(true);
    this->connect(this->action_show_details, SIGNAL(triggered()),
        this, SLOT(on_details_toggled()));

    this->action_copy_embedding = new QAction
        (QIcon(":/images/icon_copy.svg"),
        tr("&Copy Embedding"), this);
    this->connect(this->action_copy_embedding, SIGNAL(triggered()),
        this, SLOT(on_copy_embedding()));

    this->action_del_embedding = new QAction
        (QIcon(":/images/icon_delete.svg"),
        tr("Delete Embedding"), this);
    this->connect(this->action_del_embedding, SIGNAL(triggered()),
        this, SLOT(on_del_embedding()));
}

/* ---------------------------------------------------------------- */

void
ViewInspect::create_menus (void)
{
    this->toolbar = new QToolBar("Viewer tools");
    this->toolbar->addAction(this->action_open);
    this->toolbar->addAction(this->action_reload);
    this->toolbar->addAction(this->action_save_view);
    this->toolbar->addAction(this->action_export_ply);
    this->toolbar->addAction(this->action_export_image);
    this->toolbar->addSeparator();
    this->toolbar->addAction(this->action_zoom_in);
    this->toolbar->addAction(this->action_zoom_out);
    this->toolbar->addAction(this->action_zoom_reset);
    this->toolbar->addAction(this->action_zoom_fit);
    this->toolbar->addSeparator();
    this->toolbar->addAction(this->action_copy_embedding);
    this->toolbar->addAction(this->action_del_embedding);
    //this->toolbar->addSeparator();
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->toolbar->addWidget(get_expander());
    this->toolbar->addAction(this->action_show_details);
}

/* ---------------------------------------------------------------- */

void
ViewInspect::show_details (bool show)
{
    this->image_details->setVisible(show);
}

/* ---------------------------------------------------------------- */

void
ViewInspect::update_actions (void)
{
    bool active = this->scroll_image->get_pixmap() != nullptr;
    this->action_zoom_fit->setEnabled(active);
    this->action_zoom_in->setEnabled(active);
    this->action_zoom_out->setEnabled(active);
    this->action_zoom_reset->setEnabled(active);
}

/* ---------------------------------------------------------------- */

void
ViewInspect::load_file (QString filename)
{
    /* Check extension. Handle ".mve" files manually. */
    if (filename.rightRef(4) == ".mve")
        this->load_mve_file(filename);
    else if (filename.rightRef(4) == ".ply")
        this->load_ply_file(filename);
    else
        this->load_image_file(filename);
}

/* ---------------------------------------------------------------- */

void
ViewInspect::load_image_file (QString filename)
{
    std::string fname = filename.toStdString();
    std::string ext4 = util::string::right(fname, 4);
    std::string ext5 = util::string::right(fname, 5);

    /* Try to load it as byte image. */
    mve::ImageBase::Ptr img;
    try
    { img = mve::image::load_file(fname); }
    catch (...) {}

    if (img == nullptr && ext4 == ".pfm")
    {
        try { img = mve::image::load_pfm_file(fname); }
        catch (...) {}
    }

    if (img == nullptr && (ext4 == ".tif" || ext5 == ".tiff"))
    {
        try { img = mve::image::load_tiff_16_file(fname); }
        catch (...) {}
    }

    if (img == nullptr)
    {
        QMessageBox::warning(this, tr("Image Viewer"),
            tr("Cannot load image %1").arg(filename));
        return;
    }

    this->set_image(img);
    this->update_actions();
}

/* ---------------------------------------------------------------- */

void
ViewInspect::load_mve_file (QString filename)
{
    mve::View::Ptr view(mve::View::create());
    try
    { view->load_view(filename.toStdString()); }
    catch (std::exception& e)
    {
        QMessageBox::warning(this, tr("Image Viewer"),
            tr("Cannot load %1:\n%2").arg(filename, e.what()));
        return;
    }

    this->on_view_selected(view);
}

/* ---------------------------------------------------------------- */

void
ViewInspect::load_ply_file (QString filename)
{
    try
    {
        mve::FloatImage::Ptr img(mve::geom::load_ply_depthmap(filename.toStdString()));
        this->set_image(img);
    }
    catch (std::exception& e)
    {
        QMessageBox::warning(this, tr("Image Viewer"),
            tr("Cannot load %1:\n%2").arg(filename, e.what()));
        return;
    }
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_view_selected (mve::View::Ptr view)
{
    if (!this->is_tab_active) {
        this->next_view = view;
        return;
    }

    this->reset();
    this->view = view;
    this->next_view = nullptr;
    if (this->view == nullptr)
        return;

    this->load_recent_embedding();
    this->populate_embeddings();
    this->label_name->setText(view->get_name().c_str());
    this->populate_exif_viewer();
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_scene_selected (mve::Scene::Ptr /*scene*/)
{
    this->reset();
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_tab_activated (void)
{
    if (this->next_view != nullptr)
        this->on_view_selected(this->next_view);
}

/* ---------------------------------------------------------------- */

void
ViewInspect::set_embedding (std::string const& name)
{
    if (this->view == nullptr)
    {
        QMessageBox::warning(this, tr("Image Viewer"), tr("No view loaded!"));
        return;
    }

    /* Request the embedding. */
    mve::ImageBase::Ptr img(this->view->get_image(name));
    if (img == nullptr)
    {
        QMessageBox::warning(this, tr("Image Viewer"),
            tr("Embedding not available: %1").arg(QString(name.c_str())));
        return;
    }

    this->recent_embedding = name;
    this->set_image(img);
}

/* ---------------------------------------------------------------- */

void
ViewInspect::set_image (mve::ImageBase::ConstPtr img)
{
	tone_mapping->setEnabled(false);
	image = img;
	if (image == nullptr)
		return;

	/* Enable tone mapping? */
	if (image->get_type() == mve::IMAGE_TYPE_DOUBLE || image->get_type() == mve::IMAGE_TYPE_FLOAT)
	{
		tone_mapping->setEnabled(true);
		tone_mapping->set_image(image);
	}

	on_image_changed();


    /* Update dimension and memory labels. */
    std::stringstream ss;
    ss << img->width() << "x" << img->height() << "x"
        << img->channels() << " (" << img->get_type_string() << ")";
	label_dimension->setText(tr("%1").arg(ss.str().c_str()));
	label_memory->setText(tr("%1 KB").arg(img->get_byte_size() / 1024));

}

/* ---------------------------------------------------------------- */

void
ViewInspect::display_byte_image (mve::ByteImage::ConstPtr img)
{
    int const iw = img->width();
    int const ih = img->height();
    int const ic = img->channels();
    bool const is_gray = (ic == 1 || ic == 2);
    bool const has_alpha = (ic == 2 || ic == 4);

    QImage img_qimage(iw, ih, QImage::Format_ARGB32);
    {
        std::size_t inpos = 0;
        for (int y = 0; y < ih; ++y)
            for (int x = 0; x < iw; ++x)
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

    QPixmap pixmap = QPixmap::fromImage(img_qimage);
#if QT_VERSION >= 0x050000
    pixmap.setDevicePixelRatio(this->windowHandle()->devicePixelRatio());
#endif
    this->scroll_image->set_pixmap(pixmap);
    this->action_zoom_fit->setEnabled(true);
    this->update_actions();
}

/* ---------------------------------------------------------------- */

void
ViewInspect::load_recent_embedding (void)
{
    /* If no embedding is set, fall back to undistorted image. */
    if (this->recent_embedding.empty()
        || !this->view->has_image(this->recent_embedding))
        this->recent_embedding = "undistorted";

    /* Give up. */
    if (!this->view->has_image(this->recent_embedding))
        return;

    /* Load embedding. */
    this->set_embedding(this->recent_embedding);
}

/* ---------------------------------------------------------------- */

void
ViewInspect::populate_embeddings (void)
{
    this->embeddings->clear();
    if (this->view == nullptr)
        return;

	/* Get image proxies of current view. */
    std::vector<std::string> names;
    mve::View::ImageProxies const& proxies = this->view->get_images();
    for (std::size_t i = 0; i < proxies.size(); ++i)
	{
		mve::View::ImageProxy const& proxy = proxies[i];

		/* Only add images which can be displayed. */
		if (proxy.type != mve::IMAGE_TYPE_FLOAT &&
			proxy.type != mve::IMAGE_TYPE_DOUBLE &&
			proxy.type != mve::IMAGE_TYPE_UINT8)
		{
			continue;
		}

		names.push_back(proxy.name);
	}
    std::sort(names.begin(), names.end());

	/* Add supported images to displayed embeddings list. */
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        this->embeddings->addItem(QString(names[i].c_str()), (int)i);
        if (names[i] == this->recent_embedding)
           this->embeddings->setCurrentIndex(this->embeddings->count() - 1);
    }
    this->embeddings->adjustSize();
    this->embeddings->setEnabled(!proxies.empty());
}

/* ---------------------------------------------------------------- */

void
ViewInspect::populate_exif_viewer (void)
{
    this->exif_viewer->setHtml("<i>No EXIF data available</i>");

    if (this->view == nullptr)
        return;

    mve::ByteImage::Ptr exif = this->view->get_blob("exif");
    if (exif == nullptr)
        return;

    mve::image::ExifInfo info;
    try
    {
        info = mve::image::exif_extract(exif->get_byte_pointer(),
            exif->get_byte_size());
    }
    catch (std::exception& e)
    {
        std::cout << "Error parsing EXIF: " << e.what() << std::endl;
        return;
    }

    std::stringstream ss;
    mve::image::exif_debug_print(ss, info);
    this->exif_viewer->setText(ss.str().c_str());
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_zoom_in (void)
{
    this->scroll_image->zoom_in();
    this->action_zoom_fit->setChecked(false);
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_zoom_out (void)
{
    this->scroll_image->zoom_out();
    this->action_zoom_fit->setChecked(false);
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_normal_size (void)
{
    this->scroll_image->reset_scale();
    this->action_zoom_fit->setChecked(false);
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_fit_to_window (void)
{
    this->scroll_image->set_auto_scale(true);
    this->update_actions();
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_open (void)
{
    QFileDialog dialog(this->window(), tr("Open File"), last_image_dir);
    dialog.setFileMode(QFileDialog::ExistingFile);
    if (!dialog.exec())
        return;

    last_image_dir = dialog.directory().path();
    QString filename = dialog.selectedFiles()[0];

    if (filename.isEmpty())
        return;

    this->load_file(filename);
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_view_reload (void)
{
    if (this->view == nullptr)
        return;

    QMessageBox::StandardButton answer = QMessageBox::question(this,
        tr("Reload view?"), tr("Really reload view \"%1\" from file?"
        " Unsaved changes get lost, this cannot be undone.")
        .arg(this->view->get_directory().c_str()),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);

    if (answer != QMessageBox::Yes)
        return;

    this->view->reload_view();
    this->populate_embeddings();
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_details_toggled (void)
{
    bool show = this->action_show_details->isChecked();
    this->show_details(show);
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_embedding_selected (QString const& name)
{
    //std::cout << "Embedding selected: " << name.toStdString() << std::endl;
    this->set_embedding(name.toStdString());
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_image_clicked (int x, int y, QMouseEvent* event)
{
    if (event->buttons() & Qt::RightButton)
    {
        this->inspector->magnify(x, y);
        this->action_show_details->setChecked(true);
        this->show_details(true);
        this->image_details->setCurrentIndex(1);
    }
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_reload_embeddings (void)
{
    this->populate_embeddings();
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_image_changed (void)
{
	mve::ByteImage::ConstPtr byte_image = nullptr;
	if (this->tone_mapping->isEnabled())
	{
		byte_image = this->tone_mapping->render();
	}
	else
	{
		byte_image = std::dynamic_pointer_cast<mve::ByteImage const>(this->image);
		if (nullptr == byte_image)
			return;
	}

    this->inspector->set_image(byte_image, this->image);
    this->display_byte_image(byte_image);
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_ply_export (void)
{
    if (this->view == nullptr)
    {
        QMessageBox::information(this, tr("Export PLY"),
            tr("No view assigned!"));
        return;
    }

    /* Raise dialog that queries embedding names for:
     * depthmap, confidence map, normal map, color image.
     */
    PlyExportDialog d(this->view, this);
    int answer = d.exec();
    if (answer != QDialog::Accepted)
        return;

    QString filename = QFileDialog::getSaveFileName(this,
        "Export PLY file...");
    if (filename.size() == 0)
        return;

    /* Determine filename of PLY and XF file. */
    std::string plyname = filename.toStdString();
    if (util::string::right(plyname, 4) != ".ply")
        plyname += ".ply";
    std::string xfname = util::string::left
        (plyname, plyname.size() - 4) + ".xf";

    try
    {
        mve::geom::save_ply_view(this->view, plyname,
            d.depthmap, d.confidence, d.colorimage);
        mve::geom::save_xf_file(xfname, this->view->get_camera());
    }
    catch (std::exception& e)
    {
        QMessageBox::warning(this, tr("Export PLY"),
            tr("Error exporting PLY: %1").arg(QString(e.what())));
    }
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_image_export (void)
{
    if (!this->scroll_image->get_image()->pixmap())
    {
        QMessageBox::critical(this, "Cannot save image", "No such image");
        return;
    }

    QString filename = QFileDialog::getSaveFileName(this,
        "Export Image...");
    if (filename.size() == 0)
        return;

    try
    {
        this->scroll_image->save_image(filename.toStdString());
    }
    catch (std::exception& e)
    {
        QMessageBox::critical(this, "Cannot save image", e.what());
        return;
    }
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_copy_embedding (void)
{
    if (this->image == nullptr || this->view == nullptr)
    {
        QMessageBox::warning(this, tr("Image Viewer"),
            tr("No embedding selected!"));
        return;
    }

    bool text_ok = false;
    QString qtext = QInputDialog::getText(this, tr("Copy Embedding"),
        tr("Enter a target name for the new embedding."),
        QLineEdit::Normal, this->recent_embedding.c_str(), &text_ok);

    if (!text_ok || qtext.isEmpty())
        return;

    std::string text = qtext.toStdString();

    if (text == this->recent_embedding)
    {
        QMessageBox::warning(this, tr("Image Viewer"),
            tr("Target and current embedding are the same!"));
        return;
    }

    bool exists = this->view->has_image(text);
    if (exists)
    {
        QMessageBox::StandardButton answer = QMessageBox::question(this,
            tr("Overwrite Embedding?"),
            tr("Target embedding exists. Overwrite?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);

        if (answer != QMessageBox::Yes)
            return;
    }

    mve::ImageBase::Ptr image_copy = this->image->duplicate_base();
    this->view->set_image(image_copy, text);
    this->populate_embeddings();
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_del_embedding (void)
{
    if (this->image == nullptr || this->view == nullptr)
    {
        QMessageBox::warning(this, tr("Image Viewer"),
            tr("No embedding selected!"));
        return;
    }

    QMessageBox::StandardButton answer = QMessageBox::question(this,
        tr("Delete Embedding?"), tr("Really delete embedding \"%1\"?"
        " This cannot be undone.").arg(this->recent_embedding.c_str()),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);

    if (answer != QMessageBox::Yes)
        return;

    this->view->remove_image(this->recent_embedding);
    this->load_recent_embedding();
    this->populate_embeddings();
}

/* ---------------------------------------------------------------- */

void
ViewInspect::on_save_view (void)
{
    if (this->view == nullptr)
    {
        QMessageBox::warning(this, tr("Image Viewer"),
            tr("No view selected!"));
        return;
    }

    try
    {
        this->view->save_view();
    }
    catch (std::exception& e)
    {
        std::cout << "Error saving view: " << e.what() << std::endl;
        QMessageBox::critical(this, tr("Error saving view"),
            tr("Error saving view:\n%1").arg(e.what()));
    }
}

/* ---------------------------------------------------------------- */

void
ViewInspect::reset (void)
{
    this->view.reset();
    this->image.reset();
    this->tone_mapping->reset();
    this->inspector->reset();
    this->scroll_image->get_image()->setPixmap(QPixmap());
    this->embeddings->clear();
}

/* ---------------------------------------------------------------- */

QString
ViewInspect::get_title (void)
{
    return tr("View inspect");
}
