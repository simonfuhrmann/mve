/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef UMVE_MAIN_WINDOW_HEADER
#define UMVE_MAIN_WINDOW_HEADER

#include "ogl/opengl.h"

#include <string>
#include <QMainWindow>

#include "mve/view.h"

#include "viewinspect/viewinspect.h"
#include "scene_inspect/scene_inspect.h"
#include "jobqueue.h"
#include "sceneoverview.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    QDockWidget* dock_scene;
    QDockWidget* dock_jobs;
    //QDockWidget* dock_log;
    QTabWidget* tabs;

    QStatusBar* statusbar;
    QLabel* memory_label;
    QTimer* update_timer;

    SceneOverview* scene_overview;
    JobQueue* jobqueue;
    ViewInspect* tab_viewinspect;
    SceneInspect* tab_sceneinspect;

    QAction* action_new_scene;
    QAction* action_open_scene;
    QAction* action_reload_scene;
    QAction* action_save_scene;
    QAction* action_close_scene;
    QAction* action_import_images;
    QAction* action_recon_export;
    QAction* action_batch_delete;
    QAction* action_generate_thumbs;
    QAction* action_cache_cleanup;
    QAction* action_refresh_scene;
    QAction* action_exit;
    QAction* action_about;
    QAction* action_about_qt;

    QMenu* menu_scene;
    QMenu* menu_help;

private:
    void create_actions (void);
    void create_menus (void);
    bool perform_close_scene (void);
    void enable_scene_actions (bool value);
    void load_plugins (void);

private slots:
    void on_new_scene (void);
    void on_reload_scene (void);
    void on_save_scene (void);
    void on_close_scene (void);
    void on_refresh_scene (void);
    void on_about (void);

    void on_import_images (void);
    void on_update_memory (void);
    void on_cache_cleanup (void);
    void on_recon_export (void);
    void on_batch_delete (void);
    void on_generate_thumbs (void);
    void on_switch_tabs (int tab_id);

    void closeEvent (QCloseEvent* event);

public slots:
    void raise_open_scene_dialog (void);

public:
    MainWindow (void);
    ~MainWindow (void);

    void load_scene (std::string const& path);
    void load_file (std::string const& filename);

    void open_scene_inspect (void);
};

/* ---------------------------------------------------------------- */

inline
MainWindow::~MainWindow (void)
{
}

#endif /* UMVE_MAIN_WINDOW_HEADER */
