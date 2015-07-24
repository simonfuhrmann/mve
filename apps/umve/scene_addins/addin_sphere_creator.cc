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
#include "scene_addins/addin_sphere_creator.h"

AddinSphereCreator::AddinSphereCreator (void)
{
    this->layout = new QFormLayout();
    this->layout->setSpacing(1);

    for (int i = 0; i < 4; ++i)
    {
        this->spins[i] = new QDoubleSpinBox();
        this->spins[i]->setMinimum(-999.0);
        this->spins[i]->setMaximum(999.0);
        this->spins[i]->setDecimals(4);
        this->spins[i]->setSingleStep(0.1);
    }
    this->layout->addRow("Pos X:", this->spins[0]);
    this->layout->addRow("Pos Y:", this->spins[1]);
    this->layout->addRow("Pos Z:", this->spins[2]);
    this->layout->addRow("Radius:", this->spins[3]);

    QPushButton* create_but = new QPushButton("Create");
    this->layout->addRow(create_but);
    this->connect(create_but, SIGNAL(clicked()),
        this, SLOT(on_create_clicked()));
}

QWidget*
AddinSphereCreator::get_sidebar_widget (void)
{
    return get_wrapper(this->layout);
}

void
AddinSphereCreator::on_create_clicked (void)
{
    math::Vec3f const pos(this->spins[0]->value(),
        this->spins[1]->value(), this->spins[2]->value());
    float const radius = this->spins[3]->value();


    /* Build icosahedron. */
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();

    verts.reserve(12);
    verts.push_back(math::Vec3f(0.0f, -0.5257311f, 0.8506508f));
    verts.push_back(math::Vec3f(0.0f, 0.5257311f, 0.8506508f));
    verts.push_back(math::Vec3f(0.0f, -0.5257311f, -0.8506508f));
    verts.push_back(math::Vec3f(0.0f, 0.5257311f, -0.8506508f));
    verts.push_back(math::Vec3f(0.8506508f, 0.0f, 0.5257311f));
    verts.push_back(math::Vec3f(0.8506508f, 0.0f, -0.5257311f));
    verts.push_back(math::Vec3f(-0.8506508f, 0.0f, 0.5257311f));
    verts.push_back(math::Vec3f(-0.8506508f, 0.0f, -0.5257311f));
    verts.push_back(math::Vec3f(0.5257311f, 0.8506508f, 0.0f));
    verts.push_back(math::Vec3f(0.5257311f, -0.8506508f, 0.0f));
    verts.push_back(math::Vec3f(-0.5257311f, 0.8506508f, 0.0f));
    verts.push_back(math::Vec3f(-0.5257311f, -0.8506508f, 0.0f));

    faces.reserve(20 * 3);
    faces.push_back(0); faces.push_back(4); faces.push_back(1);
    faces.push_back(0); faces.push_back(9); faces.push_back(4);
    faces.push_back(9); faces.push_back(5); faces.push_back(4);
    faces.push_back(4); faces.push_back(5); faces.push_back(8);
    faces.push_back(4); faces.push_back(8); faces.push_back(1);
    faces.push_back(8); faces.push_back(10); faces.push_back(1);
    faces.push_back(8); faces.push_back(3); faces.push_back(10);
    faces.push_back(5); faces.push_back(3); faces.push_back(8);
    faces.push_back(5); faces.push_back(2); faces.push_back(3);
    faces.push_back(2); faces.push_back(7); faces.push_back(3);
    faces.push_back(7); faces.push_back(10); faces.push_back(3);
    faces.push_back(7); faces.push_back(6); faces.push_back(10);
    faces.push_back(7); faces.push_back(11); faces.push_back(6);
    faces.push_back(11); faces.push_back(0); faces.push_back(6);
    faces.push_back(0); faces.push_back(1); faces.push_back(6);
    faces.push_back(6); faces.push_back(1); faces.push_back(10);
    faces.push_back(9); faces.push_back(0); faces.push_back(11);
    faces.push_back(9); faces.push_back(11); faces.push_back(2);
    faces.push_back(9); faces.push_back(2); faces.push_back(5);
    faces.push_back(7); faces.push_back(2); faces.push_back(11);

    /* Icosahedron subdivision. */
    int const num_subdivisions = 2;
    for (int i = 0; i < num_subdivisions; ++i)
    {
        /* Walk over faces and generate vertices. */
        typedef std::pair<unsigned int, unsigned int> Edge;
        typedef std::pair<Edge, unsigned int> MapValueType;
        std::map<Edge, unsigned int> edge_map;
        for (std::size_t j = 0; j < faces.size(); j += 3)
        {
            unsigned int const v0 = faces[j + 0];
            unsigned int const v1 = faces[j + 1];
            unsigned int const v2 = faces[j + 2];
            edge_map.insert(MapValueType(Edge(v0, v1), verts.size()));
            edge_map.insert(MapValueType(Edge(v1, v0), verts.size()));
            verts.push_back((verts[v0] + verts[v1]) / 2.0f);
            edge_map.insert(MapValueType(Edge(v1, v2), verts.size()));
            edge_map.insert(MapValueType(Edge(v2, v1), verts.size()));
            verts.push_back((verts[v1] + verts[v2]) / 2.0f);
            edge_map.insert(MapValueType(Edge(v2, v0), verts.size()));
            edge_map.insert(MapValueType(Edge(v0, v2), verts.size()));
            verts.push_back((verts[v2] + verts[v0]) / 2.0f);
        }

        /* Walk over faces and create new connectivity. */
        std::size_t num_faces = faces.size();
        for (std::size_t j = 0; j < num_faces; j += 3)
        {
            unsigned int const v0 = faces[j + 0];
            unsigned int const v1 = faces[j + 1];
            unsigned int const v2 = faces[j + 2];
            unsigned int ev0 = edge_map[Edge(v0, v1)];
            unsigned int ev1 = edge_map[Edge(v1, v2)];
            unsigned int ev2 = edge_map[Edge(v2, v0)];
            faces.push_back(ev0); faces.push_back(v1); faces.push_back(ev1);
            faces.push_back(ev1); faces.push_back(v2); faces.push_back(ev2);
            faces.push_back(ev2); faces.push_back(v0); faces.push_back(ev0);
            faces[j + 0] = ev0;
            faces[j + 1] = ev1;
            faces[j + 2] = ev2;
        }
    }

    /* Normalize and transform vertices. */
    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        verts[i].normalize();
        verts[i] = verts[i] * radius + pos;
    }

    mesh->recalc_normals();
    emit this->mesh_generated("sphere", mesh);
    this->repaint();
}
