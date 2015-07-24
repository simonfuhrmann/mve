/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <QPushButton>

#include "mve/mesh.h"

#include "guihelpers.h"
#include "scene_addins/addin_aabb_creator.h"

AddinAABBCreator::AddinAABBCreator (void)
{
    this->layout = new QFormLayout();
    this->layout->addRow(new QLabel("Min"), new QLabel("Max"));
    this->layout->setSpacing(1);

    for (int i = 0; i < 6; ++i)
    {
        this->spins[i] = new QDoubleSpinBox();
        this->spins[i]->setMinimum(-999.0);
        this->spins[i]->setMaximum(999.0);
        this->spins[i]->setDecimals(4);
        this->spins[i]->setSingleStep(0.1);
    }
    for (int i = 0; i < 3; ++i)
        this->layout->addRow(this->spins[i], this->spins[i + 3]);

    QPushButton* create_but = new QPushButton("Create");
    this->layout->addRow(create_but);

    this->connect(create_but, SIGNAL(clicked()),
        this, SLOT(on_create_clicked()));
}

QWidget*
AddinAABBCreator::get_sidebar_widget (void)
{
    return get_wrapper(this->layout);
}

void
AddinAABBCreator::on_create_clicked (void)
{
    math::Vec3f aabb_min, aabb_max;
    aabb_min[0] = std::min(this->spins[0]->value(), this->spins[3]->value());
    aabb_max[0] = std::max(this->spins[0]->value(), this->spins[3]->value());
    aabb_min[1] = std::min(this->spins[1]->value(), this->spins[4]->value());
    aabb_max[1] = std::max(this->spins[1]->value(), this->spins[4]->value());
    aabb_min[2] = std::min(this->spins[2]->value(), this->spins[5]->value());
    aabb_max[2] = std::max(this->spins[2]->value(), this->spins[5]->value());

    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();
    verts.resize(6 * 4);
    faces.reserve(6 * 2 * 3);

    /* Bottom vertices. */
    verts[0]  = math::Vec3f(aabb_min[0], aabb_min[1], aabb_min[2]);
    verts[1]  = math::Vec3f(aabb_max[0], aabb_min[1], aabb_min[2]);
    verts[2]  = math::Vec3f(aabb_min[0], aabb_min[1], aabb_max[2]);
    verts[3]  = math::Vec3f(aabb_max[0], aabb_min[1], aabb_max[2]);
    /* Top vertices. */
    verts[4]  = math::Vec3f(aabb_min[0], aabb_max[1], aabb_min[2]);
    verts[5]  = math::Vec3f(aabb_max[0], aabb_max[1], aabb_min[2]);
    verts[6]  = math::Vec3f(aabb_min[0], aabb_max[1], aabb_max[2]);
    verts[7]  = math::Vec3f(aabb_max[0], aabb_max[1], aabb_max[2]);
    /* Back vertices. */
    verts[8]  = math::Vec3f(aabb_min[0], aabb_min[1], aabb_min[2]);
    verts[9]  = math::Vec3f(aabb_max[0], aabb_min[1], aabb_min[2]);
    verts[10] = math::Vec3f(aabb_min[0], aabb_max[1], aabb_min[2]);
    verts[11] = math::Vec3f(aabb_max[0], aabb_max[1], aabb_min[2]);
    /* Front vertices. */
    verts[12] = math::Vec3f(aabb_min[0], aabb_min[1], aabb_max[2]);
    verts[13] = math::Vec3f(aabb_max[0], aabb_min[1], aabb_max[2]);
    verts[14] = math::Vec3f(aabb_min[0], aabb_max[1], aabb_max[2]);
    verts[15] = math::Vec3f(aabb_max[0], aabb_max[1], aabb_max[2]);
    /* Left vertices. */
    verts[16] = math::Vec3f(aabb_min[0], aabb_min[1], aabb_min[2]);
    verts[17] = math::Vec3f(aabb_min[0], aabb_max[1], aabb_min[2]);
    verts[18] = math::Vec3f(aabb_min[0], aabb_min[1], aabb_max[2]);
    verts[19] = math::Vec3f(aabb_min[0], aabb_max[1], aabb_max[2]);
    /* Right vertices. */
    verts[20] = math::Vec3f(aabb_max[0], aabb_min[1], aabb_min[2]);
    verts[21] = math::Vec3f(aabb_max[0], aabb_max[1], aabb_min[2]);
    verts[22] = math::Vec3f(aabb_max[0], aabb_min[1], aabb_max[2]);
    verts[23] = math::Vec3f(aabb_max[0], aabb_max[1], aabb_max[2]);

    /* Bottom faces. */
    faces.push_back(0); faces.push_back(1); faces.push_back(2);
    faces.push_back(1); faces.push_back(3); faces.push_back(2);
    /* Top faces. */
    faces.push_back(4); faces.push_back(7); faces.push_back(5);
    faces.push_back(4); faces.push_back(6); faces.push_back(7);
    /* Back faces. */
    faces.push_back(8); faces.push_back(10); faces.push_back(9);
    faces.push_back(9); faces.push_back(10); faces.push_back(11);
    /* Front faces. */
    faces.push_back(12); faces.push_back(13); faces.push_back(14);
    faces.push_back(13); faces.push_back(15); faces.push_back(14);
    /* Left faces. */
    faces.push_back(16); faces.push_back(18); faces.push_back(17);
    faces.push_back(17); faces.push_back(18); faces.push_back(19);
    /* Right faces. */
    faces.push_back(20); faces.push_back(21); faces.push_back(22);
    faces.push_back(21); faces.push_back(23); faces.push_back(22);

    mesh->recalc_normals();
    emit this->mesh_generated("aabb", mesh);
    this->repaint();
}
