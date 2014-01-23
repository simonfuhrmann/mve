#include <limits>
#include <iostream>

#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "util/file_system.h"
#include "util/string.h"
#include "mve/mesh.h"
#include "mve/mesh_io.h"
#include "mve/mesh_tools.h"

#include "scene_addins/mesh_list.h"

QMeshContextMenu::QMeshContextMenu (QMeshList* parent)
{
    this->parent = parent;
    this->rep = NULL;
    this->item = NULL;
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::build (void)
{
    QAction* action_reload_mesh = new QAction("Reload mesh", this);
    QAction* action_save_mesh = new QAction("Save mesh...", this);
    QAction* action_rename_mesh = new QAction("Rename mesh...", this);

    this->connect(action_reload_mesh, SIGNAL(triggered()),
        this, SLOT(on_reload_mesh()));
    this->connect(action_save_mesh, SIGNAL(triggered()),
        this, SLOT(on_save_mesh()));
    this->connect(action_rename_mesh, SIGNAL(triggered()),
        this, SLOT(on_rename_mesh()));

    std::string num_verts(util::string::get(rep->mesh->get_vertices().size()));
    std::string num_faces(util::string::get(rep->mesh->get_faces().size() / 3));
    util::string::punctate(&num_verts, '\'');
    util::string::punctate(&num_faces, '\'');

    QMenu* vertices_menu =
        this->addMenu(tr("Vertices: %1").arg(num_verts.c_str()));
    {
        QAction* scale = vertices_menu->addAction(tr("Scale and center"));
        this->connect(scale, SIGNAL(triggered()),
            this, SLOT(on_scale_and_center()));
        if (rep->mesh->get_vertices().empty())
        {
            scale->setEnabled(false);
        }
    }

    QMenu* faces_menu = this->addMenu(tr("Faces: %1").arg(num_faces.c_str()));
    {
        QAction* invert_faces = faces_menu->addAction(tr("Invert faces"));
        QAction* delete_faces = faces_menu->addAction(tr("Delete faces"));
        this->connect(invert_faces, SIGNAL(triggered()),
            this, SLOT(on_invert_faces()));
        this->connect(delete_faces, SIGNAL(triggered()),
            this, SLOT(on_delete_faces()));
        if (rep->mesh->get_faces().empty())
        {
            invert_faces->setEnabled(false);
            delete_faces->setEnabled(false);
        }
    }

    if (rep->mesh->has_vertex_normals())
    {
        QMenu* menu = this->addMenu(tr("Vertex Normals"));
        QAction* delete_vnormals = menu->addAction(tr("Delete normals"));
        this->connect(delete_vnormals, SIGNAL(triggered()),
            this, SLOT(on_delete_vertex_normals()));
    }

    if (rep->mesh->has_vertex_colors())
    {
        QMenu* menu = this->addMenu(tr("Vertex Colors"));
        QAction* delete_vcolor = menu->addAction(tr("Delete colors"));
        this->connect(delete_vcolor, SIGNAL(triggered()),
            this, SLOT(on_delete_vertex_colors()));
    }

    if (rep->mesh->has_vertex_confidences())
    {
        QMenu* menu = this->addMenu(tr("Vertex Confidences"));
        QAction* convert_color = menu->addAction(tr("Map to color"));
        QAction* delete_vconfs = menu->addAction(tr("Delete confidences"));
        this->connect(convert_color, SIGNAL(triggered()),
            this, SLOT(on_colorize_confidences()));
        this->connect(delete_vconfs, SIGNAL(triggered()),
            this, SLOT(on_delete_vertex_confidences()));
    }

    if (rep->mesh->has_vertex_values())
    {
        QMenu* menu = this->addMenu(tr("Vertex Values"));
        QAction* convert_color = menu->addAction(tr("Map to color"));
        QAction* delete_vvalues = menu->addAction(tr("Delete values"));
        this->connect(convert_color, SIGNAL(triggered()),
            this, SLOT(on_colorize_values()));
        this->connect(delete_vvalues, SIGNAL(triggered()),
            this, SLOT(on_delete_vertex_values()));
    }

    if (rep->mesh->has_face_colors())
        this->addAction(tr("Face Colors: Yes"))->setEnabled(false);

    this->addSeparator();
    this->addAction(action_reload_mesh);
    this->addAction(action_rename_mesh);
    this->addAction(action_save_mesh);

    /* Enable / disable certain actions. */
    action_reload_mesh->setEnabled(!rep->filename.empty());
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::on_reload_mesh (void)
{
    try
    {
        this->rep->mesh = mve::geom::load_mesh(this->rep->filename);
        if (!this->rep->mesh->get_faces().empty())
            this->rep->mesh->ensure_normals();
    }
    catch (std::exception& e)
    {
        QMessageBox::critical(this->parent, tr("Error reloading mesh"),
            tr("There was an error while reloading the mesh.\n"
            "%1").arg(e.what()));
        return;
    }

    this->rep->renderer.reset();
    emit this->parent->signal_redraw();
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::on_invert_faces (void)
{
    mve::geom::mesh_invert_faces(this->rep->mesh);
    this->rep->renderer.reset();
    emit this->parent->signal_redraw();
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::on_delete_faces (void)
{
    this->rep->mesh->get_faces().clear();
    this->rep->mesh->get_face_normals().clear();
    this->rep->renderer.reset();
    emit this->parent->signal_redraw();
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::on_scale_and_center (void)
{
    mve::geom::mesh_scale_and_center(this->rep->mesh);
    this->rep->renderer.reset();
    emit this->parent->signal_redraw();
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::on_save_mesh (void)
{
    QString filename = QFileDialog::getSaveFileName(this->parent,
        tr("Save mesh to file"), QDir::currentPath());
    if (filename.isEmpty())
        return;

    std::string fname(filename.toStdString());
    try
    {
        mve::geom::save_mesh(this->rep->mesh, fname);
        this->rep->filename = fname;
        this->rep->name = util::fs::get_file_component(fname);
        this->item->setText(this->rep->name.c_str());
    }
    catch (std::exception& e)
    {
        QMessageBox::critical(this->parent, "Error saving mesh", e.what());
    }
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::on_rename_mesh (void)
{
    bool pressed_ok = false;
    QString q_new_name = QInputDialog::getText(this->parent,
        tr("Rename mesh..."), tr("New mesh name:"), QLineEdit::Normal,
        this->rep->name.c_str(), &pressed_ok);

    if (q_new_name.isEmpty() || !pressed_ok)
        return;

    /* Check if there is a mesh by that new name. That is not allowed! */
    std::string new_name = q_new_name.toStdString();
    MeshRep* old_mesh = this->parent->mesh_by_name(new_name);
    if (old_mesh != NULL)
    {
        QMessageBox::critical(this->parent, "Error renaming mesh",
            "A mesh by that name does already exist!");
        return;
    }

    this->rep->name = new_name;
    this->rep->filename.clear();
    this->item->setText(new_name.c_str());
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::on_delete_vertex_normals (void)
{
    this->rep->mesh->get_vertex_normals().clear();
    this->rep->renderer.reset();
    emit this->parent->signal_redraw();
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::on_delete_vertex_colors (void)
{
    this->rep->mesh->get_vertex_colors().clear();
    this->rep->renderer.reset();
    emit this->parent->signal_redraw();
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::on_delete_vertex_confidences (void)
{
    this->rep->mesh->get_vertex_confidences().clear();
    this->rep->renderer.reset();
    emit this->parent->signal_redraw();
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::on_delete_vertex_values (void)
{
    this->rep->mesh->get_vertex_values().clear();
    this->rep->renderer.reset();
    emit this->parent->signal_redraw();
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::on_colorize_values (void)
{
    this->on_colorize_with_attrib(this->rep->mesh->get_vertex_values());
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::on_colorize_confidences (void)
{
    this->on_colorize_with_attrib(this->rep->mesh->get_vertex_confidences());
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::on_colorize_with_attrib (std::vector<float> const& attrib)
{
    mve::TriangleMesh::ColorList& vcolor
        = this->rep->mesh->get_vertex_colors();
    if (attrib.size() != this->rep->mesh->get_vertices().size())
        return;

#if 0
    /* Find min/max value of the attribute. */
    float fmin = std::numeric_limits<float>::max();
    float fmax = -std::numeric_limits<float>::max();
    for (std::size_t i = 0; i < attrib.size(); ++i)
    {
        fmin = std::min(fmin, attrib[i]);
        fmax = std::max(fmax, attrib[i]);
    }
#else
    /* Robust percentile min/max assignment. */
    std::vector<float> tmp = attrib;
    std::sort(tmp.begin(), tmp.end());
    float const fmin = tmp[0];// tmp[1 * tmp.size() / 40];
    float const fmax = tmp[19 * tmp.size() / 20];
#endif

    /* Assign the min/max value as gray-scale color value. */
    vcolor.clear();
    vcolor.resize(attrib.size());
    for (std::size_t i = 0; i < attrib.size(); ++i)
    {
        float value = (attrib[i] - fmin) / (fmax - fmin);
        value = std::max(0.0f, std::min(1.0f, value));
        //value = std::pow(value, 2.0f);
        //vcolor[i] = math::Vec4f(1.0f, 1.0f - value, 1.0f - value, 1.0f);
        vcolor[i] = math::Vec4f(1.0f, value, value, 1.0f);
    }

    this->rep->renderer.reset();
    emit this->parent->signal_redraw();
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

QMeshList::QMeshList (QWidget *parent)
    : QWidget(parent)
{
    //this->toolbar = new QToolBar();
    //this->toolbar->setIconSize(QSize(16, 16));
    this->qlist = new QListWidget();
    this->qlist->setContextMenuPolicy(Qt::CustomContextMenu);

    QPushButton* select_all_but = new QPushButton("All");
    QPushButton* select_none_but = new QPushButton("Nne");
    QPushButton* inv_selection_but = new QPushButton("Inv");
    QPushButton* select_next_but = new QPushButton("Nxt");
    QPushButton* select_toggle_but = new QPushButton("Tgl");
    select_all_but->setMinimumWidth(8);
    select_none_but->setMinimumWidth(8);
    inv_selection_but->setMinimumWidth(8);
    select_next_but->setMinimumWidth(8);
    select_toggle_but->setMinimumWidth(8);
    select_all_but->setToolTip
        (tr("Check all meshes"));
    select_none_but->setToolTip
        (tr("Uncheck all meshes"));
    inv_selection_but->setToolTip
        (tr("Invert check state of all meshes"));
    select_next_but->setToolTip
        (tr("Move check state to the next mesh in order"));
    select_toggle_but->setToolTip
        (tr("Toggle between a checked and the selected mesh"));

    QHBoxLayout* button_hbox = new QHBoxLayout();
    button_hbox->setSpacing(1);
    button_hbox->addWidget(select_all_but, 1);
    button_hbox->addWidget(select_none_but, 1);
    button_hbox->addWidget(select_next_but, 1);
    button_hbox->addWidget(inv_selection_but, 1);
    button_hbox->addWidget(select_toggle_but, 1);

    QVBoxLayout* vbox = new QVBoxLayout();
    vbox->setSpacing(1);
    vbox->setContentsMargins(0,0,0,0);
    //vbox->addWidget(this->toolbar);
    vbox->addWidget(this->qlist);
    vbox->addLayout(button_hbox);
    this->setLayout(vbox);

    this->connect(this->qlist, SIGNAL(itemActivated(QListWidgetItem*)),
        this, SLOT(on_item_activated(QListWidgetItem*)));
    this->connect(this->qlist, SIGNAL(itemChanged(QListWidgetItem*)),
        this, SLOT(on_item_changed(QListWidgetItem*)));
    this->connect(this->qlist, SIGNAL(customContextMenuRequested(QPoint)),
        this, SLOT(on_list_context_menu(QPoint)));

    this->connect(select_all_but, SIGNAL(clicked()),
        this, SLOT(on_select_all()));
    this->connect(select_none_but, SIGNAL(clicked()),
        this, SLOT(on_select_none()));
    this->connect(select_next_but, SIGNAL(clicked()),
        this, SLOT(on_select_next()));
    this->connect(inv_selection_but, SIGNAL(clicked()),
        this, SLOT(on_inv_selection()));
    this->connect(select_toggle_but, SIGNAL(clicked()),
        this, SLOT(on_select_toggle()));
}

/* ---------------------------------------------------------------- */

QMeshList::~QMeshList (void)
{
}

/* ---------------------------------------------------------------- */

void
QMeshList::add (std::string const& name, mve::TriangleMesh::Ptr mesh,
    std::string const& filename)
{
    /* Check if mesh by that name is already available. */
    for (std::size_t i = 0; i < this->meshes.size(); ++i)
    {
        MeshRep& rep(this->meshes[i]);
        if (rep.name == name)
        {
            rep.mesh = mesh;
            rep.renderer.reset();
            return;
        }
    }

    MeshRep rep;
    rep.name = name;
    rep.filename = filename;
    rep.mesh = mesh;
    rep.active = true;
    this->meshes.push_back(rep);
    this->update_list();
}

/* ---------------------------------------------------------------- */

void
QMeshList::remove (std::string const& name)
{
    MeshList::iterator i = this->meshes.begin();
    while (i != this->meshes.end())
    {
        if (i->name == name)
            i = this->meshes.erase(i);
        else
            ++i;
    }
    this->update_list();
    emit this->signal_redraw();
}

/* ---------------------------------------------------------------- */

void
QMeshList::update_list (void)
{
    this->qlist->clear();
    for (std::size_t i = 0; i < this->meshes.size(); ++i)
    {
        MeshRep const& rep(this->meshes[i]);
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(rep.name.c_str());
        item->setCheckState(rep.active ? Qt::Checked : Qt::Unchecked);
        this->qlist->addItem(item);
    }
}

/* ---------------------------------------------------------------- */

MeshRep*
QMeshList::mesh_by_name (std::string const& name)
{
    for (std::size_t i = 0; i < this->meshes.size(); ++i)
        if (this->meshes[i].name == name)
            return &this->meshes[i];
    return NULL;
}

/* ---------------------------------------------------------------- */

void
QMeshList::on_item_activated (QListWidgetItem* item)
{
    std::string name = item->text().toStdString();
    this->remove(name);
}

/* ---------------------------------------------------------------- */

void
QMeshList::on_item_changed (QListWidgetItem* item)
{
    if (this->meshes.size() != (std::size_t)this->qlist->count())
    {
        std::cout << "Error: Meshlist size mismatch!" << std::endl;
        return;
    }

    std::string name = item->text().toStdString();
    for (std::size_t i = 0; i < this->meshes.size(); ++i)
    {
        MeshRep& rep(this->meshes[i]);
        QListWidgetItem* item = this->qlist->item(i);
        if (item->text().toStdString() == rep.name)
            rep.active = (item->checkState() == Qt::Checked);
    }
    emit this->signal_redraw();
}

/* ---------------------------------------------------------------- */

void
QMeshList::on_select_all (void)
{
    for (int i = 0; i < this->qlist->count(); ++i)
    {
        QListWidgetItem* item = this->qlist->item(i);
        item->setCheckState(Qt::Checked);
    }
    emit this->signal_redraw();
}

/* ---------------------------------------------------------------- */

void
QMeshList::on_select_none (void)
{
    for (int i = 0; i < this->qlist->count(); ++i)
    {
        QListWidgetItem* item = this->qlist->item(i);
        item->setCheckState(Qt::Unchecked);
    }
    emit this->signal_redraw();
}

/* ---------------------------------------------------------------- */

void
QMeshList::on_inv_selection (void)
{
    for (int i = 0; i < this->qlist->count(); ++i)
    {
        QListWidgetItem* item = this->qlist->item(i);
        Qt::CheckState cs = item->checkState();
        item->setCheckState(cs == Qt::Checked ? Qt::Unchecked : Qt::Checked);
    }
    emit this->signal_redraw();
}

/* ---------------------------------------------------------------- */

void
QMeshList::on_select_next (void)
{
    int cnt = this->qlist->count();
    std::vector<int> check;
    for (int i = 0; i < cnt; ++i)
    {
        QListWidgetItem* item = this->qlist->item(i);
        Qt::CheckState cs = item->checkState();
        if (cs == Qt::Checked)
            check.push_back((i + 1) % cnt);
        item->setCheckState(Qt::Unchecked);
    }

    for (std::size_t i = 0; i < check.size(); ++i)
        this->qlist->item(check[i])->setCheckState(Qt::Checked);

    emit this->signal_redraw();
}

/* ---------------------------------------------------------------- */

void
QMeshList::on_select_toggle (void)
{
    int current = this->qlist->currentRow();
    int checked = -1;

    /* Find the mesh that is checked (in case of multiple, find first). */
    int cnt = this->qlist->count();
    for (int i = 0; i < cnt; ++i)
    {
        QListWidgetItem* item = this->qlist->item(i);
        if (item->checkState() == Qt::Checked)
            checked = i;
        item->setCheckState(Qt::Unchecked);
    }

    /* Exchange state of checked and selected mesh. */
    if (checked != -1)
        this->qlist->setCurrentRow(checked);
    if (current != -1)
        this->qlist->item(current)->setCheckState(Qt::Checked);
}

/* ---------------------------------------------------------------- */

void
QMeshList::on_list_context_menu (QPoint pos)
{
    QListWidgetItem* item = this->qlist->itemAt(pos.x(), pos.y());
    if (!item)
        return;

    std::string name = item->text().toStdString();
    QPoint global_pos = this->mapToGlobal(pos);
    QMeshContextMenu* menu = new QMeshContextMenu(this);
    menu->rep = this->mesh_by_name(name);
    menu->item = item;
    menu->build();
    menu->exec(global_pos);
}
