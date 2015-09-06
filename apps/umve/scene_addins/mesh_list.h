/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef UMVE_MESHLIST_HEADER
#define UMVE_MESHLIST_HEADER

#include <vector>
#include <QListWidget>
#include <QMenu>

#include "math/matrix.h"
#include "mve/mesh.h"

#include "ogl/mesh_renderer.h"
#include "ogl/texture.h"

/* Simple mesh representation with name, renderer and texture. */
struct MeshRep
{
    std::string name;
    std::string filename;
    bool active;
    mve::TriangleMesh::Ptr mesh;
    ogl::MeshRenderer::Ptr renderer;
    ogl::Texture::Ptr texture;
};

/* ---------------------------------------------------------------- */

class QMeshList;

/**
 * Context menu for each loaded mesh.
 */
class QMeshContextMenu : public QMenu
{
    Q_OBJECT

private slots:
    void on_reload_mesh (void);
    void on_invert_faces (void);
    void on_delete_faces (void);
    void on_scale_and_center (void);
    void on_compute_aabb (void);
    void on_save_mesh (void);
    void on_rename_mesh (void);
    void on_delete_vertex_normals (void);
    void on_normalize_vertex_normals (void);
    void on_delete_vertex_colors (void);
    void on_delete_vertex_confidences (void);
    void on_colorize_confidences (void);
    void on_delete_vertex_values (void);
    void on_colorize_values (void);
    void on_colorize_with_attrib (std::vector<float> const& attrib);
    void on_colorize_mesh_red (void);
    void on_colorize_mesh_green (void);
    void on_colorize_mesh_blue (void);
    void on_colorize_mesh_custom (void);
    void on_colorize_mesh (float red, float green, float blue);

public:
    QListWidgetItem* item;
    MeshRep* rep;
    QMeshList* parent;

public:
    QMeshContextMenu (QMeshList* parent);
    void build (void);
};

/* ---------------------------------------------------------------- */


class QMeshList : public QWidget
{
    Q_OBJECT
    friend class QMeshContextMenu;

public:
    typedef std::vector<MeshRep> MeshList;

protected slots:
    void on_item_activated (QListWidgetItem* item);
    void on_item_changed (QListWidgetItem* item);
    void on_select_all (void);
    void on_select_none (void);
    void on_select_next (void);
    void on_inv_selection (void);
    void on_select_toggle (void);
    void on_list_context_menu (QPoint pos);

signals:
    void signal_redraw (void);

private:
    MeshList meshes;

    QListWidget* qlist;
    //QToolBar* toolbar;
    //QAction* action_open_mesh;

private:
    void update_list (void);
    MeshRep* mesh_by_name (std::string const& name);

public:
    QMeshList (QWidget* parent = nullptr);
    ~QMeshList (void);

    void add (std::string const& name, mve::TriangleMesh::Ptr mesh,
        std::string const& filename = "", ogl::Texture::Ptr texture = nullptr);
    void remove (std::string const& name);

    MeshList const& get_meshes (void) const;
    MeshList& get_meshes (void);

    /* QT stuff. */
    QSize sizeHint (void) const;
};

/* -------------------------- Implementation ---------------------- */

inline QSize
QMeshList::sizeHint (void) const
{
    return QSize(175, 0);
}

inline QMeshList::MeshList const&
QMeshList::get_meshes (void) const
{
    return this->meshes;
}

inline QMeshList::MeshList&
QMeshList::get_meshes (void)
{
    return this->meshes;
}

#endif /* UMVE_MESHLIST_HEADER */
