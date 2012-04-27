#include <iostream>

#include "util/fs.h"
#include "util/string.h"
#include "mve/trianglemesh.h"
#include "mve/meshtools.h"
#include "mve/offfile.h"

#include "meshlist.h"

QMeshContextMenu::QMeshContextMenu (QMeshList* parent)
{
    this->parent = parent;
    this->rep = 0;
    this->item = 0;
}

/* ---------------------------------------------------------------- */

void
QMeshContextMenu::build (void)
{
    QAction* action_reload_mesh = new QAction("Reload mesh", this);
    QAction* action_scale_and_center = new QAction("Scale and center", this);
    QAction* action_invert_faces = new QAction("Invert faces", this);
    QAction* action_strip_faces = new QAction("Strip faces", this);
    QAction* action_save_mesh = new QAction("Save mesh...", this);

    this->connect(action_reload_mesh, SIGNAL(triggered()),
        this, SLOT(on_reload_mesh()));
    this->connect(action_invert_faces, SIGNAL(triggered()),
        this, SLOT(on_invert_faces()));
    this->connect(action_strip_faces, SIGNAL(triggered()),
        this, SLOT(on_strip_faces()));
    this->connect(action_scale_and_center, SIGNAL(triggered()),
        this, SLOT(on_scale_and_center()));
    this->connect(action_save_mesh, SIGNAL(triggered()),
        this, SLOT(on_save_mesh()));

    std::string num_verts(util::string::get(rep->mesh->get_vertices().size()));
    std::string num_faces(util::string::get(rep->mesh->get_faces().size() / 3));
    util::string::punctate(num_verts, '\'');
    util::string::punctate(num_faces, '\'');

    this->addAction(tr("Vertices: %1").arg(num_verts.c_str()))->setEnabled(false);
    this->addAction(tr("Faces: %1").arg(num_faces.c_str()))->setEnabled(false);
    this->addSeparator();
    this->addAction(action_reload_mesh);
    this->addAction(action_scale_and_center);
    this->addAction(action_invert_faces);
    this->addAction(action_strip_faces);
    this->addAction(action_save_mesh);

    /* Enable / disable certain actions. */
    action_reload_mesh->setEnabled(!rep->filename.empty());
    if (rep->mesh->get_faces().empty())
    {
        action_invert_faces->setEnabled(false);
        action_strip_faces->setEnabled(false);
    }
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
QMeshContextMenu::on_strip_faces (void)
{
    this->rep->mesh->get_faces().clear();
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
/* ---------------------------------------------------------------- */

QMeshList::QMeshList (QWidget *parent)
    : QWidget(parent)
{
    //this->toolbar = new QToolBar();
    //this->toolbar->setIconSize(QSize(16, 16));
    this->qlist = new QListWidget();
    this->qlist->setContextMenuPolicy(Qt::CustomContextMenu);

    QPushButton* select_all_but = new QPushButton("All");
    QPushButton* select_none_but = new QPushButton("None");
    QPushButton* inv_selection_but = new QPushButton("Inv");
    QPushButton* select_next_but = new QPushButton("Next");
    select_all_but->setMinimumWidth(15);
    select_none_but->setMinimumWidth(15);
    inv_selection_but->setMinimumWidth(15);
    select_next_but->setMinimumWidth(15);

    QHBoxLayout* button_hbox = new QHBoxLayout();
    button_hbox->setSpacing(1);
    button_hbox->addWidget(select_all_but, 1);
    button_hbox->addWidget(select_none_but, 1);
    button_hbox->addWidget(inv_selection_but, 1);
    button_hbox->addWidget(select_next_but, 1);

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
    this->connect(inv_selection_but, SIGNAL(clicked()),
        this, SLOT(on_inv_selection()));
    this->connect(select_next_but, SIGNAL(clicked()),
        this, SLOT(on_next_select_next()));
}

/* ---------------------------------------------------------------- */

QMeshList::~QMeshList (void)
{
}

/* ---------------------------------------------------------------- */

void
QMeshList::load_mesh (std::string const& filename)
{
    std::cout << "Loading triangle mesh..." << std::endl;
    mve::TriangleMesh::Ptr mesh;
    try
    {
        mesh = mve::geom::load_mesh(filename);
    }
    catch (std::exception& e)
    {
        std::cout << "> Error loading mesh: " << e.what() << std::endl;
        mesh = mve::TriangleMesh::create();
    }
    //mve::geom::mesh_scale_and_center(mesh);
    mesh->ensure_normals();

    std::string name = util::fs::get_file_component(filename);
    this->add(name, mesh);
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
    return 0;
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
QMeshList::on_next_select_next (void)
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
