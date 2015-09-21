// Test cases for the mesh info class.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "mve/mesh.h"
#include "mve/mesh_info.h"

namespace
{
    mve::TriangleMesh::Ptr
    create_test_mesh (void)
    {
        mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
        mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
        mve::TriangleMesh::FaceList& faces = mesh->get_faces();
        verts.insert(verts.end(), 11, math::Vec3f(0.0f));
        faces.push_back(0); faces.push_back(1); faces.push_back(6);
        faces.push_back(1); faces.push_back(7); faces.push_back(6);
        faces.push_back(1); faces.push_back(2); faces.push_back(7);
        faces.push_back(2); faces.push_back(3); faces.push_back(7);
        faces.push_back(3); faces.push_back(4); faces.push_back(7);
        faces.push_back(4); faces.push_back(6); faces.push_back(7);
        faces.push_back(4); faces.push_back(5); faces.push_back(6);
        faces.push_back(5); faces.push_back(0); faces.push_back(6);
        faces.push_back(7); faces.push_back(9); faces.push_back(8);
        faces.push_back(3); faces.push_back(8); faces.push_back(9);
        return mesh;
    }
}

TEST(MeshInfoTest, ClassificationTest)
{
    mve::TriangleMesh::Ptr mesh = create_test_mesh();
    mve::MeshInfo mesh_info(mesh);

    EXPECT_EQ(mesh->get_vertices().size(), mesh_info.size());
    EXPECT_EQ(mve::MeshInfo::VERTEX_CLASS_BORDER, mesh_info[0].vclass);
    EXPECT_EQ(mve::MeshInfo::VERTEX_CLASS_BORDER, mesh_info[1].vclass);
    EXPECT_EQ(mve::MeshInfo::VERTEX_CLASS_BORDER, mesh_info[2].vclass);
    EXPECT_EQ(mve::MeshInfo::VERTEX_CLASS_COMPLEX, mesh_info[3].vclass);
    EXPECT_EQ(mve::MeshInfo::VERTEX_CLASS_BORDER, mesh_info[4].vclass);
    EXPECT_EQ(mve::MeshInfo::VERTEX_CLASS_BORDER, mesh_info[5].vclass);
    EXPECT_EQ(mve::MeshInfo::VERTEX_CLASS_SIMPLE, mesh_info[6].vclass);
    EXPECT_EQ(mve::MeshInfo::VERTEX_CLASS_COMPLEX, mesh_info[7].vclass);
    EXPECT_EQ(mve::MeshInfo::VERTEX_CLASS_BORDER, mesh_info[8].vclass);
    EXPECT_EQ(mve::MeshInfo::VERTEX_CLASS_BORDER, mesh_info[9].vclass);
    EXPECT_EQ(mve::MeshInfo::VERTEX_CLASS_UNREF, mesh_info[10].vclass);

    EXPECT_EQ(2, mesh_info[0].faces.size());
    EXPECT_EQ(0, mesh_info[0].faces[0]);
    EXPECT_EQ(7, mesh_info[0].faces[1]);
    EXPECT_EQ(3, mesh_info[0].verts.size());
    EXPECT_EQ(1, mesh_info[0].verts[0]);
    EXPECT_EQ(6, mesh_info[0].verts[1]);
    EXPECT_EQ(5, mesh_info[0].verts[2]);

    EXPECT_EQ(4, mesh_info[1].verts.size());
    EXPECT_EQ(3, mesh_info[1].faces.size());
    EXPECT_EQ(2, mesh_info[1].faces[0]);
    EXPECT_EQ(1, mesh_info[1].faces[1]);
    EXPECT_EQ(0, mesh_info[1].faces[2]);

    EXPECT_EQ(2, mesh_info[2].faces.size());
    EXPECT_EQ(3, mesh_info[2].verts.size());

    EXPECT_EQ(3, mesh_info[3].faces.size());
    EXPECT_EQ(5, mesh_info[3].verts.size());

    EXPECT_EQ(5, mesh_info[6].faces.size());
    EXPECT_EQ(7, mesh_info[6].faces[0]);
    EXPECT_EQ(0, mesh_info[6].faces[1]);
    EXPECT_EQ(1, mesh_info[6].faces[2]);
    EXPECT_EQ(5, mesh_info[6].faces[3]);
    EXPECT_EQ(6, mesh_info[6].faces[4]);

    EXPECT_EQ(5, mesh_info[6].verts.size());
    EXPECT_EQ(5, mesh_info[6].verts[0]);
    EXPECT_EQ(0, mesh_info[6].verts[1]);
    EXPECT_EQ(1, mesh_info[6].verts[2]);
    EXPECT_EQ(7, mesh_info[6].verts[3]);
    EXPECT_EQ(4, mesh_info[6].verts[4]);

    EXPECT_EQ(6, mesh_info[7].faces.size());
    EXPECT_EQ(7, mesh_info[7].verts.size());

    EXPECT_EQ(2, mesh_info[9].faces.size());
    EXPECT_EQ(3, mesh_info[9].verts.size());

    EXPECT_EQ(0, mesh_info[10].faces.size());
    EXPECT_EQ(0, mesh_info[10].verts.size());
}
