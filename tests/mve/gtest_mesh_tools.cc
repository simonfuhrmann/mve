// Test cases for the MVE mesh tools.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "mve/mesh.h"
#include "mve/mesh_tools.h"

TEST(MeshToolsTest, Components)
{
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    verts.insert(verts.end(), 6, math::Vec3f(0.0f));
    mesh->get_faces().push_back(0);
    mesh->get_faces().push_back(1);
    mesh->get_faces().push_back(2);

    mve::TriangleMesh::Ptr tmp;
    tmp = mesh->duplicate();
    mve::geom::mesh_components(tmp, 0);  // Deletes nothing.
    EXPECT_EQ(6, tmp->get_vertices().size());

    tmp = mesh->duplicate();
    mve::geom::mesh_components(tmp, 1);  // Deletes isolated verts.
    EXPECT_EQ(3, tmp->get_vertices().size());

    tmp = mesh->duplicate();
    mve::geom::mesh_components(tmp, 2);  // Deletes isolated verts.
    EXPECT_EQ(3, tmp->get_vertices().size());

    tmp = mesh->duplicate();
    mve::geom::mesh_components(tmp, 3);  // Deletes everything here.
    EXPECT_EQ(0, tmp->get_vertices().size());
}

TEST(MeshToolsTest, DeleteUnreferenced)
{
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mesh->get_vertices().push_back(math::Vec3f(0.0f));
    mesh->get_vertices().push_back(math::Vec3f(1.0f));
    mesh->get_vertices().push_back(math::Vec3f(2.0f));
    mesh->get_vertices().push_back(math::Vec3f(3.0f));
    mesh->get_vertices().push_back(math::Vec3f(4.0f));
    mesh->get_faces().push_back(1);
    mesh->get_faces().push_back(2);
    mesh->get_faces().push_back(3);

    EXPECT_EQ(2, mve::geom::mesh_delete_unreferenced(mesh));
    EXPECT_EQ(3, mesh->get_vertices().size());
    EXPECT_EQ(math::Vec3f(1.0f), mesh->get_vertices()[0]);
    EXPECT_EQ(math::Vec3f(2.0f), mesh->get_vertices()[1]);
    EXPECT_EQ(math::Vec3f(3.0f), mesh->get_vertices()[2]);
    EXPECT_EQ(3, mesh->get_faces().size());
    EXPECT_EQ(0, mesh->get_faces()[0]);
    EXPECT_EQ(1, mesh->get_faces()[1]);
    EXPECT_EQ(2, mesh->get_faces()[2]);
}
