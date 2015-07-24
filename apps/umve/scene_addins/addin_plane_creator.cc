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
#include "scene_addins/addin_plane_creator.h"

AddinPlaneCreator::AddinPlaneCreator (void)
{
    this->layout = new QFormLayout();
    this->layout->addRow(new QLabel("Normal"), new QLabel("Offset"));
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
AddinPlaneCreator::get_sidebar_widget (void)
{
    return get_wrapper(this->layout);
}

void
AddinPlaneCreator::on_create_clicked (void)
{
    math::Vec3f normal(this->spins[0]->value(),
        this->spins[1]->value(), this->spins[2]->value());
    math::Vec3f offset(this->spins[3]->value(),
        this->spins[4]->value(), this->spins[5]->value());

    /* Build LCS. */
    normal.normalize();
    math::Vec3f axis1, axis2;
    if (math::Vec3f(1.0f, 0.0f, 0.0f).is_similar(normal, 0.0001f))
        axis1 = normal.cross(math::Vec3f(0.0f, 1.0f, 0.0f)).normalized();
    else
        axis1 = normal.cross(math::Vec3f(1.0f, 0.0f, 0.0f)).normalized();
    axis2 = normal.cross(axis1).normalized();

    /* Build mesh. */
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();
    verts.reserve(4);
    faces.reserve(2 * 3);

    float const scale = 10.0f;
    verts.push_back(offset - axis1 * scale - axis2 * scale);
    verts.push_back(offset + axis1 * scale - axis2 * scale);
    verts.push_back(offset - axis1 * scale + axis2 * scale);
    verts.push_back(offset + axis1 * scale + axis2 * scale);

    faces.push_back(0); faces.push_back(1); faces.push_back(2);
    faces.push_back(1); faces.push_back(3); faces.push_back(2);

    mesh->recalc_normals();
    emit this->mesh_generated("plane", mesh);
    this->repaint();
}
