/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <QBoxLayout>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>

#include <iostream>

#include "util/file_system.h"

#include "guihelpers.h"
#include "scenemanager.h"
#include "scene_inspect/scene_inspect.h"

SceneInspect::SceneInspect (QWidget* parent)
    : MainWindowTab(parent)
{
    /* Create toolbar, add actions. */
    QToolBar* toolbar = new QToolBar("Mesh tools");
    this->create_actions(toolbar);

    /* Create details notebook. */
    this->tab_widget = new QTabWidget();
    this->tab_widget->setTabPosition(QTabWidget::East);

    /* Create GL context. */
    this->gl_widget = new GLWidget();
    this->addin_manager = new AddinManager(this->gl_widget, this->tab_widget);
    this->gl_widget->set_context(this->addin_manager);

    /* Connect signals. */
    this->connect(&SceneManager::get(), SIGNAL(scene_selected(mve::Scene::Ptr)),
        this, SLOT(on_scene_selected(mve::Scene::Ptr)));
    this->connect(&SceneManager::get(), SIGNAL(view_selected(mve::View::Ptr)),
        this, SLOT(on_view_selected(mve::View::Ptr)));
    this->connect(this, SIGNAL(tab_activated()), SLOT(on_tab_activated()));

    /* Pack everything together. */
    QVBoxLayout* vbox = new QVBoxLayout();
    vbox->addWidget(toolbar);
    vbox->addWidget(this->gl_widget);

    QHBoxLayout* main_layout = new QHBoxLayout(this);
    main_layout->addLayout(vbox, 1);
    main_layout->addWidget(this->tab_widget);

    //this->gl_widget->repaint_gl();
}

/* ---------------------------------------------------------------- */

void
SceneInspect::load_file (std::string const& filename)
{
    this->addin_manager->load_file(filename);
    this->last_mesh_dir = util::fs::dirname(filename);
}

/* ---------------------------------------------------------------- */

void
SceneInspect::reset (void)
{
    this->addin_manager->reset_scene();
}

/* ---------------------------------------------------------------- */

void
SceneInspect::on_tab_activated (void)
{
    if (this->next_view != nullptr)
        this->on_view_selected(this->next_view);
}

/* ---------------------------------------------------------------- */

void
SceneInspect::create_actions (QToolBar* toolbar)
{
    this->action_open_mesh = new QAction(QIcon(":/images/icon_open_file.svg"),
        tr("Open mesh"), this);
    this->connect(this->action_open_mesh, SIGNAL(triggered()),
        this, SLOT(on_open_mesh()));

    this->action_reload_shaders = new QAction(QIcon(":/images/icon_revert.svg"),
        tr("Reload shaders"), this);
    this->connect(this->action_reload_shaders, SIGNAL(triggered()),
        this, SLOT(on_reload_shaders()));

    this->action_show_details = new QAction
        (QIcon(":/images/icon_toolbox.svg"), tr("Show &Details"), this);
    this->action_show_details->setCheckable(true);
    this->action_show_details->setChecked(true);
    this->connect(this->action_show_details, SIGNAL(triggered()),
        this, SLOT(on_details_toggled()));

    this->action_save_screenshot = new QAction(QIcon(":/images/icon_screenshot.svg"),
        tr("Save Screenshot"), this);
    this->connect(this->action_save_screenshot, SIGNAL(triggered()),
        this, SLOT(on_save_screenshot()));

    toolbar->addAction(this->action_open_mesh);
    toolbar->addAction(this->action_reload_shaders);
    toolbar->addSeparator();
    toolbar->addAction(this->action_save_screenshot);
    toolbar->addWidget(get_expander());
    toolbar->addAction(this->action_show_details);
}

/* ---------------------------------------------------------------- */

void
SceneInspect::on_open_mesh (void)
{
    QFileDialog dialog(this->window(), tr("Open Mesh"), this->last_mesh_dir.c_str());
    dialog.setFileMode(QFileDialog::ExistingFiles);
    if (!dialog.exec())
        return;

    QStringList filenames = dialog.selectedFiles();
    for (QStringList::iterator it = filenames.begin();
         it != filenames.end(); ++it)
    {
        this->load_file(it->toStdString());
    }
}

/* ---------------------------------------------------------------- */

void
SceneInspect::on_details_toggled (void)
{
    bool show = this->action_show_details->isChecked();
    this->tab_widget->setVisible(show);
}

/* ---------------------------------------------------------------- */

void
SceneInspect::on_reload_shaders (void)
{
    this->addin_manager->load_shaders();
}

/* ---------------------------------------------------------------- */

void
SceneInspect::on_scene_selected (mve::Scene::Ptr scene)
{
    this->addin_manager->set_scene(scene);
    if (scene)
        this->last_mesh_dir = scene->get_path();
}

/* ---------------------------------------------------------------- */

void
SceneInspect::on_view_selected (mve::View::Ptr view)
{
    if (!this->is_tab_active)
    {
        this->next_view = view;
        return;
    }

    this->addin_manager->set_view(view);
    this->next_view = nullptr;
}

/* ---------------------------------------------------------------- */

void
SceneInspect::on_save_screenshot (void)
{
    QString filename = QFileDialog::getSaveFileName(this,
        "Export Image...");
    if (filename.size() == 0)
        return;

    QImage img = this->gl_widget->grabFrameBuffer(false);
    bool success = img.save(filename);
    if (!success)
        QMessageBox::critical(this, "Cannot save image",
            "Error saving image");
}

/* ---------------------------------------------------------------- */

QString
SceneInspect::get_title (void)
{
    return tr("Scene inspect");
}
