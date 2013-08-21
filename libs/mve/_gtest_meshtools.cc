// Test cases for the MVE mesh tools.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "mve/trianglemesh.h"
#include "mve/meshtools.h"

TEST(MeshToolsTest, Components)
{
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    mve::TriangleMesh::Ptr outmesh;
    verts.insert(verts.end(), 6, math::Vec3f(0.0f));
    mesh->get_faces().push_back(0);
    mesh->get_faces().push_back(1);
    mesh->get_faces().push_back(2);

    outmesh = mve::geom::mesh_components(mesh, 0);  // Deletes nothing.
    EXPECT_EQ(6, outmesh->get_vertices().size());

    outmesh = mve::geom::mesh_components(mesh, 1);  // Deletes isolated verts.
    EXPECT_EQ(3, outmesh->get_vertices().size());

    outmesh = mve::geom::mesh_components(mesh, 2);  // Deletes isolated verts.
    EXPECT_EQ(3, outmesh->get_vertices().size());

    outmesh = mve::geom::mesh_components(mesh, 3);  // Deletes everything here.
    EXPECT_EQ(0, outmesh->get_vertices().size());
}
