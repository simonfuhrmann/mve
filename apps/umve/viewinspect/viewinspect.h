/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef VIEW_INSPECT_HEADER
#define VIEW_INSPECT_HEADER

#include <string>
#include <QTextEdit>
#include <QToolBar>

#include "mve/view.h"

#include "mainwindowtab.h"
#include "scrollimage.h"
#include "imageinspector.h"
#include "imageoperations.h"
#include "tonemapping.h"

class ViewInspect : public MainWindowTab
{
    Q_OBJECT

private:
    ScrollImage* scroll_image;
    QComboBox* embeddings;
    QToolBar* toolbar;
    QTabWidget* image_details;
    QLabel* label_name;
    QLabel* label_dimension;
    QLabel* label_memory;
    ImageInspectorWidget* inspector;
    ImageOperationsWidget* operations;
    ToneMapping* tone_mapping;
    QTextEdit* exif_viewer;
    QAction* action_open;
    QAction* action_reload;
    QAction* action_save_view;
    QAction* action_export_ply;
    QAction* action_export_image;
    QAction* action_zoom_in;
    QAction* action_zoom_out;
    QAction* action_zoom_reset;
    QAction* action_zoom_fit;
    QAction* action_show_details;
    QAction* action_copy_embedding;
    QAction* action_del_embedding;
    QString last_image_dir;

    mve::View::Ptr view;
    mve::View::Ptr next_view;
    mve::ImageBase::ConstPtr image;
    std::string recent_embedding;

private slots:
    void on_open (void);
    void on_zoom_in (void);
    void on_zoom_out (void);
    void on_normal_size (void);
    void on_fit_to_window (void);
    void on_details_toggled (void);
    void on_embedding_selected (QString const& name);
    void on_ply_export (void);
    void on_image_export (void);
    void on_copy_embedding (void);
    void on_del_embedding (void);
    void on_save_view (void);
    void on_view_reload (void);
    void on_reload_embeddings (void);
    void on_image_clicked (int x, int y, QMouseEvent* event);
	void on_image_changed (void);

    void on_scene_selected (mve::Scene::Ptr scene);
    void on_view_selected (mve::View::Ptr view);
    void on_tab_activated (void);

private:
    void create_detail_frame (void);
    void create_actions (void);
    void create_menus (void);
    void update_actions (void);
    void load_recent_embedding (void);
    void populate_embeddings (void);
    void populate_exif_viewer (void);
    void set_embedding (std::string const& name);
    void display_byte_image (mve::ByteImage::ConstPtr img);

public:
    ViewInspect (QWidget* parent = nullptr);

    void set_image (mve::ImageBase::ConstPtr img);
    void show_details (bool show);
    void load_file (QString filename);
    void load_image_file (QString filename);
    void load_mve_file (QString filename);
    void load_ply_file (QString filename);
    void reset (void);
    virtual QString get_title (void);
};

#endif /* VIEW_INSPECT_HEADER */
