/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <iostream>
#include <QPushButton>

#include "util/timer.h"
#include "mve/mesh.h"
#include "mve/mesh_tools.h"
#include "mve/depthmap.h"

#include "guihelpers.h"
#include "scene_addins/addin_dm_triangulate.h"

AddinDMTriangulate::AddinDMTriangulate (void)
{
    this->dm_triangulate_but = new QPushButton("Triangulate");
    this->dm_depthmap_cb = new QComboBox();
    this->dm_colorimage_cb = new QComboBox();
    this->dm_depth_disc = new QDoubleSpinBox();
    this->dm_depth_disc->setValue(5.0);

    /* Create DM triangulate layout. */
    this->dm_form = new QFormLayout();
    this->dm_form->setVerticalSpacing(0);
    this->dm_form->addRow(tr("Depthmap"), this->dm_depthmap_cb);
    this->dm_form->addRow(tr("Image"), this->dm_colorimage_cb);
    this->dm_form->addRow(tr("DD factor"), this->dm_depth_disc);
    this->dm_form->addRow(this->dm_triangulate_but);

    this->connect(this->dm_triangulate_but, SIGNAL(clicked()),
        this, SLOT(on_triangulate_clicked()));
    this->connect(this->dm_depthmap_cb, SIGNAL(activated(QString)),
        this, SLOT(on_select_colorimage(QString)));
}

QWidget*
AddinDMTriangulate::get_sidebar_widget (void)
{
    return get_wrapper(this->dm_form);
}

void
AddinDMTriangulate::set_selected_view (SelectedView* view)
{
    this->view = view;
    this->connect(this->view, SIGNAL(view_selected(mve::View::Ptr)),
        this, SLOT(on_view_selected()));
}

void
AddinDMTriangulate::on_triangulate_clicked (void)
{
    if (this->view == nullptr)
        return;

    float dd_factor = this->dm_depth_disc->value();
    std::string embedding = this->dm_depthmap_cb->currentText().toStdString();
    std::string colorimage = this->dm_colorimage_cb->currentText().toStdString();
    if (embedding.empty() || colorimage.empty())
    {
        this->show_error_box("Error triangulating", "No embedding selected");
        return;
    }

    mve::View::Ptr view(this->view->get_view());
    if (view == nullptr)
    {
        this->show_error_box("Error triangulating", "No view available");
        return;
    }

    /* Fetch depthmap and color image. */
    mve::FloatImage::Ptr dm(view->get_float_image(embedding));
    if (dm == nullptr)
    {
        this->show_error_box("Error triangulating",
            "Depthmap not available: " + embedding);
        return;
    }
    mve::ByteImage::Ptr ci(view->get_byte_image(colorimage));
    mve::CameraInfo const& cam(view->get_camera());

    /* Triangulate mesh. */
    util::WallTimer timer;
    mve::TriangleMesh::Ptr mesh;
    try
    { mesh = mve::geom::depthmap_triangulate(dm, ci, cam, dd_factor); }
    catch (std::exception& e)
    {
        this->show_error_box("Error triangulating", e.what());
        return;
    }
    std::cout << "Triangulating took " << timer.get_elapsed() << "ms" << std::endl;

    emit mesh_generated(view->get_name() + "-" + embedding, mesh);
    this->repaint();
}

void
AddinDMTriangulate::on_select_colorimage (QString name)
{
    if (this->view == nullptr || this->view->get_view() == nullptr)
        return;

    std::string const depth_str("depth-");
    std::string const undist_str("undist-");
    std::string depthmap = name.toStdString();

    std::size_t pos = depthmap.find(depth_str);
    if (pos == std::string::npos)
        return;

    if (depthmap == "depth-L0")
        depthmap = "undistorted";
    else
    {
        depthmap.replace(depthmap.begin() + pos,
            depthmap.begin() + pos + depth_str.size(),
            undist_str.begin(), undist_str.end());
    }

    for (int i = 0; i < this->dm_colorimage_cb->count(); ++i)
    {
        if (this->dm_colorimage_cb->itemText(i).toStdString() == depthmap)
        {
            this->dm_colorimage_cb->setCurrentIndex(i);
            break;
        }
    }
}

void
AddinDMTriangulate::on_view_selected (void)
{
    this->view->fill_embeddings(*this->dm_depthmap_cb, mve::IMAGE_TYPE_FLOAT);
    this->view->fill_embeddings(*this->dm_colorimage_cb, mve::IMAGE_TYPE_UINT8);
}
