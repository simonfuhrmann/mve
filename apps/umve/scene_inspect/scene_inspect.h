/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef UMVE_SCENE_INSPECT_HEADER
#define UMVE_SCENE_INSPECT_HEADER

#include "ogl/opengl.h"

#include <string>

#include <QGLWidget>
#include <QTabWidget>
#include <QString>
#include <QWidget>
#include <QToolBar>

#include "mve/scene.h"
#include "mve/view.h"

#include "mainwindowtab.h"
#include "glwidget.h"
#include "scene_inspect/addin_manager.h"

class SceneInspect : public MainWindowTab
{
    Q_OBJECT

public:
    SceneInspect (QWidget* parent = nullptr);

    /* Loads a mesh from file and adds to mesh list. */
    void load_file (std::string const& filename);

    /* Removes references to the scene. */
    void reset (void);

    virtual QString get_title (void);

private:
    void create_actions (QToolBar* toolbar);

private slots:
    void on_open_mesh (void);
    void on_reload_shaders (void);
    void on_details_toggled (void);
    void on_save_screenshot (void);
    void on_scene_selected (mve::Scene::Ptr scene);
    void on_view_selected (mve::View::Ptr view);
    void on_tab_activated (void);

private:
    std::string last_mesh_dir;
    mve::View::Ptr next_view;

    QTabWidget* tab_widget;
    AddinManager* addin_manager;
    GLWidget* gl_widget;
    QAction* action_open_mesh;
    QAction* action_reload_shaders;
    QAction* action_show_details;
    QAction* action_save_screenshot;
};

#endif /* UMVE_SCENE_INSPECT_HEADER */
