/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <limits>
#include <iostream>

#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QColorDialog>

#include "util/file_system.h"
#include "util/strings.h"
#include "mve/mesh.h"
#include "mve/mesh_io.h"
#include "mve/mesh_tools.h"

#include "scene_addins/mesh_list.h"

QMeshContextMenu::QMeshContextMenu (QMeshList* parent)
{
    this->parent = parent;
    this->rep = nullptr;
    this->item = nullptr;
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
            scale->setEnabled(false);

        QAction* compute_aabb = vertices_menu->addAction(tr("Compute AABB"));
        this->connect(compute_aabb, SIGNAL(triggered()),
            this, SLOT(on_compute_aabb()));
        if (rep->mesh->get_vertices().size() <= 1)
            compute_aabb->setEnabled(false);
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
        QAction* normalize_vnormals = menu->addAction(tr("Normalize normals"));
        this->connect(delete_vnormals, SIGNAL(triggered()),
            this, SLOT(on_delete_vertex_normals()));
        this->connect(normalize_vnormals, SIGNAL(triggered()),
            this, SLOT(on_normalize_vertex_normals()));
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

    /* Colorize menu. */
    {
        QMenu* menu = this->addMenu(tr("Colorize"));
        QAction* colorize_red = menu->addAction(tr("Red"));
        QAction* colorize_green = menu->addAction(tr("Green"));
        QAction* colorize_blue = menu->addAction(tr("Blue"));
        QAction* colorize_custom = menu->addAction(tr("Custom..."));

        this->connect(colorize_red, SIGNAL(triggered()),
            this, SLOT(on_colorize_mesh_red()));
        this->connect(colorize_green, SIGNAL(triggered()),
            this, SLOT(on_colorize_mesh_green()));
        this->connect(colorize_blue, SIGNAL(triggered()),
            this, SLOT(on_colorize_mesh_blue()));
        this->connect(colorize_custom, SIGNAL(triggered()),
            this, SLOT(on_colorize_mesh_custom()));
    }

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
QMeshContextMenu::on_compute_aabb (void)
{
    math::Vec3f aabb_min(std::numeric_limits<float>::max());
    math::Vec3f aabb_max(-std::numeric_limits<float>::max());

    mve::TriangleMesh::VertexList const& verts = this->rep->mesh->get_vertices();
    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            aabb_min[j] = std::min(aabb_min[j], verts[i][j]);
            aabb_max[j] = std::max(aabb_max[j], verts[i][j]);
        }
    }

    std::stringstream aabb_ss;
    aabb_ss << "<b>Exact AABB</b><br/>" << std::endl;
    aabb_ss << "AABB min: " << aabb_min << "<br/>" << std::endl;
    aabb_ss << "AABB max: " << aabb_max << "<br/>" << std::endl;
    aabb_ss << "AABB string: "
        << aabb_min[0] << ","
        << aabb_min[1] << ","
        << aabb_min[2] << ","
        << aabb_max[0] << ","
        << aabb_max[1] << ","
        << aabb_max[2] << "<br/><br/>" << std::endl;

    for (int j = 0; j < 3; ++j)
    {
        aabb_min[j] -= (aabb_max[j] - aabb_min[j]) / 20.0f;
        aabb_max[j] += (aabb_max[j] - aabb_min[j]) / 20.0f;
    }

    aabb_ss << "<b>AABB with 10% border</b><br/>" << std::endl;
    aabb_ss << "AABB min: " << aabb_min << "<br/>" << std::endl;
    aabb_ss << "AABB max: " << aabb_max << "<br/>" << std::endl;
    aabb_ss << "AABB string: "
        << aabb_min[0] << ","
        << aabb_min[1] << ","
        << aabb_min[2] << ","
        << aabb_max[0] << ","
        << aabb_max[1] << ","
        << aabb_max[2] << "<br/>" << std::endl;

    QMessageBox::information(this->parent, "Mesh AABB",
        tr(aabb_ss.str().c_str()));
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
        this->rep->name = util::fs::basename(fname);
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
    if (old_mesh != nullptr)
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
QMeshContextMenu::on_normalize_vertex_normals (void)
{
    mve::TriangleMesh::NormalList& nl = this->rep->mesh->get_vertex_normals();
    for (std::size_t i = 0; i < nl.size(); ++i)
        nl[i].normalize();
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

#if 1
    /* Find min/max value of the attribute. */
    float fmin = std::numeric_limits<float>::max();
    float fmax = -std::numeric_limits<float>::max();
    for (std::size_t i = 0; i < attrib.size(); ++i)
    {
        fmin = std::min(fmin, attrib[i]);
        fmax = std::max(fmax, attrib[i]);
    }
    std::cout << "Mapping min: " << fmin << ", max: " << fmax << std::endl;
    //fmin = 0.0f;
    //fmax = 0.00002f;
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
        vcolor[i] = math::Vec4f(1.0f, 1.0f - value, 1.0f - value, 1.0f);
    }

    this->rep->renderer.reset();
    emit this->parent->signal_redraw();
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::on_colorize_mesh_red (void)
{
    this->on_colorize_mesh(1.0f, 0.0f, 0.0f);
}

void
QMeshContextMenu::on_colorize_mesh_green (void)
{
    this->on_colorize_mesh(0.0f, 1.0f, 0.0f);
}

void
QMeshContextMenu::on_colorize_mesh_blue (void)
{
    this->on_colorize_mesh(0.0f, 0.0f, 1.0f);
}

void
QMeshContextMenu::on_colorize_mesh_custom (void)
{
    QColor color = QColorDialog::getColor(Qt::white, this);
    if (!color.isValid())
        return;
    this->on_colorize_mesh(color.redF(), color.greenF(), color.blueF());
}

void
QMeshContextMenu::on_colorize_mesh (float red, float green, float blue)
{
    mve::TriangleMesh::ColorList& vcolor
        = this->rep->mesh->get_vertex_colors();
    vcolor.clear();
    vcolor.resize(this->rep->mesh->get_vertices().size(),
        math::Vec4f(red, green, blue, 1.0f));

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
    std::string const& filename, ogl::Texture::Ptr texture)
{
    /* Check if mesh by that name is already available. */
    for (std::size_t i = 0; i < this->meshes.size(); ++i)
    {
        MeshRep& rep(this->meshes[i]);
        if (rep.name == name)
        {
            rep.mesh = mesh;
            rep.texture = texture;
            rep.renderer.reset();
            return;
        }
    }

    MeshRep rep;
    rep.name = name;
    rep.filename = filename;
    rep.mesh = mesh;
    rep.texture = texture;
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
    return nullptr;
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
