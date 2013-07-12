#ifndef UMVE_MESHLIST_HEADER
#define UMVE_MESHLIST_HEADER

#include <vector>
#include <QListWidget>
#include <QMenu>

#include "math/matrix.h"
#include "mve/trianglemesh.h"

#include "ogl/meshrenderer.h"

/* Simple mesh representation with name and modelview matrix. */
struct MeshRep
{
    std::string name;
    std::string filename;
    bool active;
    mve::TriangleMesh::Ptr mesh;
    ogl::MeshRenderer::Ptr renderer;
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
    void on_save_mesh (void);
    void on_rename_mesh (void);
    void on_delete_vertex_colors (void);
    void on_delete_vertex_confidences (void);
    void on_colorize_confidences (void);
    void on_delete_vertex_values (void);
    void on_colorize_values (void);
    void on_colorize_with_attrib (std::vector<float> const& attrib);

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
    QMeshList (QWidget* parent = 0);
    ~QMeshList (void);

    void load_mesh (std::string const& filename);

    void add (std::string const& name, mve::TriangleMesh::Ptr mesh,
        std::string const& filename = "");
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
