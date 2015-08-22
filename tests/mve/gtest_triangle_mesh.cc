// Test cases for the triangle mesh class and related features.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "mve/mesh.h"

TEST(TriangleMeshTest, MeshClearTest)
{
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();
    verts.push_back(math::Vec3f());
    verts.push_back(math::Vec3f());
    verts.push_back(math::Vec3f());
    faces.push_back(0);
    faces.push_back(1);
    faces.push_back(2);
    EXPECT_EQ(3, verts.size());
    EXPECT_EQ(3, faces.size());
    mesh->clear();
    EXPECT_EQ(0, verts.size());
    EXPECT_EQ(0, faces.size());
}

TEST(TriangleMeshTest, DeleteVerticesTest)
{
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();
    verts.push_back(math::Vec3f(0.0f));
    verts.push_back(math::Vec3f(1.0f));
    verts.push_back(math::Vec3f(2.0f));
    verts.push_back(math::Vec3f(3.0f));
    verts.push_back(math::Vec3f(4.0f));
    verts.push_back(math::Vec3f(5.0f));
    faces.push_back(0);
    faces.push_back(1);
    faces.push_back(2);

    mve::TriangleMesh::DeleteList delete_list;
    delete_list.push_back(true);
    delete_list.push_back(false);
    delete_list.push_back(false);
    delete_list.push_back(true);
    delete_list.push_back(true);
    delete_list.push_back(false);

    mesh->delete_vertices(delete_list);
    EXPECT_EQ(3, verts.size());
    EXPECT_EQ(math::Vec3f(1.0f), verts[0]);
    EXPECT_EQ(math::Vec3f(2.0f), verts[1]);
    EXPECT_EQ(math::Vec3f(5.0f), verts[2]);
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(1, faces[1]);
    EXPECT_EQ(2, faces[2]);
}

TEST(TriangleMeshTest, DeleteVerticesFixFaces)
{
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();
    verts.push_back(math::Vec3f(0.0f));
    verts.push_back(math::Vec3f(1.0f));
    verts.push_back(math::Vec3f(2.0f));
    verts.push_back(math::Vec3f(3.0f));
    verts.push_back(math::Vec3f(4.0f));
    verts.push_back(math::Vec3f(5.0f));
    faces.push_back(0);
    faces.push_back(1);
    faces.push_back(2);
    faces.push_back(1);
    faces.push_back(2);
    faces.push_back(5);

    mve::TriangleMesh::DeleteList delete_list;
    delete_list.push_back(true);
    delete_list.push_back(false);
    delete_list.push_back(false);
    delete_list.push_back(true);
    delete_list.push_back(true);
    delete_list.push_back(false);

    mesh->delete_vertices_fix_faces(delete_list);
    EXPECT_EQ(3, verts.size());
    EXPECT_EQ(math::Vec3f(1.0f), verts[0]);
    EXPECT_EQ(math::Vec3f(2.0f), verts[1]);
    EXPECT_EQ(math::Vec3f(5.0f), verts[2]);
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(1, faces[1]);
    EXPECT_EQ(2, faces[2]);
}

TEST(TriangleMeshTest, DeleteInvalidFacesTest1)
{
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();

    mesh->delete_invalid_faces();
    EXPECT_EQ(0, faces.size());

    faces.push_back(0); faces.push_back(1); faces.push_back(2);
    mesh->delete_invalid_faces();
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(1, faces[1]);
    EXPECT_EQ(2, faces[2]);

    faces.clear();
    faces.push_back(0); faces.push_back(0); faces.push_back(2);
    mesh->delete_invalid_faces();
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(0, faces[1]);
    EXPECT_EQ(2, faces[2]);

    faces.clear();
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    mesh->delete_invalid_faces();
    EXPECT_EQ(0, faces.size());

    faces.clear();
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    mesh->delete_invalid_faces();
    EXPECT_EQ(0, faces.size());

    faces.clear();
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    mesh->delete_invalid_faces();
    EXPECT_EQ(0, faces.size());

}

TEST(TriangleMeshTest, DeleteInvalidFacesTest2)
{
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();

    faces.clear();
    faces.push_back(0); faces.push_back(1); faces.push_back(2);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    mesh->delete_invalid_faces();
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(1, faces[1]);
    EXPECT_EQ(2, faces[2]);

    faces.clear();
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(1); faces.push_back(2);
    mesh->delete_invalid_faces();
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(1, faces[1]);
    EXPECT_EQ(2, faces[2]);

    faces.clear();
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(1); faces.push_back(2);
    mesh->delete_invalid_faces();
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(1, faces[1]);
    EXPECT_EQ(2, faces[2]);

    faces.clear();
    faces.push_back(0); faces.push_back(1); faces.push_back(2);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    mesh->delete_invalid_faces();
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(1, faces[1]);
    EXPECT_EQ(2, faces[2]);

    faces.clear();
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(1); faces.push_back(2);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    mesh->delete_invalid_faces();
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(1, faces[1]);
    EXPECT_EQ(2, faces[2]);
}
