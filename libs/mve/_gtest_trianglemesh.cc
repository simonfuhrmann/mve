// Test cases for the triangle mesh class and related features.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "mve/trianglemesh.h"

TEST(TriangleMeshTest, DeleteInvalidFacesTest1)
{
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();

    mesh->delete_invalid_triangles();
    EXPECT_EQ(0, faces.size());

    faces.push_back(0); faces.push_back(1); faces.push_back(2);
    mesh->delete_invalid_triangles();
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(1, faces[1]);
    EXPECT_EQ(2, faces[2]);

    faces.clear();
    faces.push_back(0); faces.push_back(0); faces.push_back(2);
    mesh->delete_invalid_triangles();
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(0, faces[1]);
    EXPECT_EQ(2, faces[2]);

    faces.clear();
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    mesh->delete_invalid_triangles();
    EXPECT_EQ(0, faces.size());

    faces.clear();
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    mesh->delete_invalid_triangles();
    EXPECT_EQ(0, faces.size());

    faces.clear();
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    mesh->delete_invalid_triangles();
    EXPECT_EQ(0, faces.size());

}

TEST(TriangleMeshTest, DeleteInvalidFacesTest2)
{
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();

    faces.clear();
    faces.push_back(0); faces.push_back(1); faces.push_back(2);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    mesh->delete_invalid_triangles();
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(1, faces[1]);
    EXPECT_EQ(2, faces[2]);

    faces.clear();
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(1); faces.push_back(2);
    mesh->delete_invalid_triangles();
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(1, faces[1]);
    EXPECT_EQ(2, faces[2]);

    faces.clear();
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(1); faces.push_back(2);
    mesh->delete_invalid_triangles();
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(1, faces[1]);
    EXPECT_EQ(2, faces[2]);

    faces.clear();
    faces.push_back(0); faces.push_back(1); faces.push_back(2);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    mesh->delete_invalid_triangles();
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(1, faces[1]);
    EXPECT_EQ(2, faces[2]);

    faces.clear();
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    faces.push_back(0); faces.push_back(1); faces.push_back(2);
    faces.push_back(0); faces.push_back(0); faces.push_back(0);
    mesh->delete_invalid_triangles();
    EXPECT_EQ(3, faces.size());
    EXPECT_EQ(0, faces[0]);
    EXPECT_EQ(1, faces[1]);
    EXPECT_EQ(2, faces[2]);
}
