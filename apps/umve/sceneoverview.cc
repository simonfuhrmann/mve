/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <QLineEdit>

#include "util/string.h"

#include "guihelpers.h"
#include "scenemanager.h"
#include "sceneoverview.h"

SceneOverview::SceneOverview (QWidget* parent)
    : QWidget(parent)
{
    this->toolbar = new QToolBar();
    this->toolbar->setIconSize(QSize(16, 16));
    this->viewlist = new QListWidget();
    this->filter = new QLineEdit();
    this->filter->setToolTip(tr("Enter embedding filter"));
    this->viewlist->setEnabled(false);

    QPushButton* clear_filter = new QPushButton();
    clear_filter->setIcon(QIcon(":/images/icon_clean.svg"));
    clear_filter->setFlat(true);
    clear_filter->setIconSize(QSize(18, 18));
    clear_filter->setToolTip(tr("Clear filter"));

    QHBoxLayout* filter_box = new QHBoxLayout();
    filter_box->setContentsMargins(QMargins(0,0,0,0));
    filter_box->setSpacing(0);
    filter_box->addWidget(this->filter, 1);
    filter_box->addWidget(clear_filter, 0);

    QVBoxLayout* vbox = new QVBoxLayout(this);
    vbox->setSpacing(0);
    vbox->setContentsMargins(0,0,0,0);
    vbox->addWidget(this->toolbar);
    vbox->addWidget(this->viewlist);
    //vbox->addWidget(this->filter);
    vbox->addLayout(filter_box);

    this->connect(this->viewlist, SIGNAL(currentRowChanged(int)),
        this, SLOT(on_row_changed(int)));
    this->connect(this->filter, SIGNAL(textChanged(QString)),
        this, SLOT(on_filter_changed()));
    this->connect(clear_filter, SIGNAL(clicked()),
        this, SLOT(on_clear_filter()));
    this->connect(&SceneManager::get(), SIGNAL(scene_selected(mve::Scene::Ptr)),
        this, SLOT(on_scene_changed(mve::Scene::Ptr)));
}

/* ---------------------------------------------------------------- */

void
SceneOverview::on_scene_changed (mve::Scene::Ptr scene)
{
    this->viewlist->clear();
    this->viewlist->setEnabled(false);

    if (scene == nullptr)
        return;

    std::string filter_str = this->filter->text().toStdString();
    mve::Scene::ViewList& sl(scene->get_views());

    /* If the scene has not views, indicate this with a label. */
    if (sl.empty())
    {
        QListWidgetItem* item = new QListWidgetItem("Scene has no views!");
        item->setData(Qt::UserRole, -1);
        this->viewlist->addItem(item);
    }
    else
    {
        this->viewlist->setEnabled(true);
    }

    /* Add all views to the list widget. */
    for (std::size_t i = 0; i < sl.size(); ++i)
    {
        mve::View::Ptr view(sl[i]);
        if (view == nullptr)
            continue;

        /* Add view if at least one embedding matches filter. */
        bool matches = filter_str.empty();
        mve::View::ImageProxies const& images = view->get_images();
        for (std::size_t j = 0; !matches && j < images.size(); ++j)
        {
            if (images[j].name.find(filter_str) != std::string::npos)
                matches = true;
        }

        mve::View::BlobProxies const& blobs = view->get_blobs();
        for (std::size_t j = 0; !matches && j < blobs.size(); ++j)
        {
            if (blobs[j].name.find(filter_str) != std::string::npos)
                matches = true;
        }

        /* Add view if one embeddings matches. */
        if (matches)
            this->add_view_to_layout(i, view);
    }
}

/* ---------------------------------------------------------------- */

void
SceneOverview::add_view_to_layout (std::size_t id, mve::View::Ptr view)
{
    if (view == nullptr)
        return;

    /* Find view name and number of embeddings. */
    std::string const& view_name = view->get_name();
    std::string view_id = util::string::get(view->get_id());
    int view_num_images = view->get_images().size();
    int view_num_data = view->get_blobs().size();
    bool cam_valid = view->get_camera().flen != 0.0f;
    QString name = QString("ID %2: %1")
        .arg(QString(view_name.c_str()), QString(view_id.c_str()));
    QString images = QString(QString("%1 img, %2 data")
        .arg(view_num_images).arg(view_num_data));

    /* Create the item image. */
    mve::ByteImage::ConstPtr thumb(view->get_byte_image("thumbnail"));
    QPixmap button_pixmap;
    if (thumb == nullptr)
    {
        button_pixmap.load(":/images/icon_broken.svg");
    }
    else
    {
        button_pixmap = get_pixmap_from_image(thumb);
        thumb.reset();
    }

    /* Create the item. */
    QIcon icon(button_pixmap);
    QListWidgetItem* item = new QListWidgetItem(icon,
        tr("%1\n%2").arg(name, images));
    if (!cam_valid)
        item->setBackgroundColor(QColor(255, 221, 221)); // #ffdddd
    item->setData(Qt::UserRole, (int)id);
    this->viewlist->addItem(item);
}

/* ---------------------------------------------------------------- */

void
SceneOverview::on_row_changed (int id)
{
    if (id < 0)
        return;

    QListWidgetItem* item = this->viewlist->item(id);
    std::size_t view_id = (std::size_t)item->data(Qt::UserRole).toInt();
    //std::cout << "ID " << id << " was selected!" << std::endl;

    mve::Scene::Ptr scene(SceneManager::get().get_scene());
    mve::View::Ptr view(scene->get_view_by_id(view_id));
    SceneManager::get().select_view(view);
}

/* ---------------------------------------------------------------- */

void
SceneOverview::on_filter_changed (void)
{
    this->on_scene_changed(SceneManager::get().get_scene());
}

/* ---------------------------------------------------------------- */

void
SceneOverview::on_clear_filter (void)
{
    this->filter->setText("");
    this->on_filter_changed();
}
