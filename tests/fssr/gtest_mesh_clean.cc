// Test cases for mesh decimator.
// Written by Simon Fuhrmann.

#include <gtest/gtest.h>

#include "mve/mesh.h"
#include "mve/mesh_tools.h"
#include "fssr/mesh_clean.h"

#if 0
TEST(MeshCleanTest, CleanTest1)
{
    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();

    verts.push_back(math::Vec3f(0,0,0));
    verts.push_back(math::Vec3f(1,0,0));
    verts.push_back(math::Vec3f(2,0,0));
    verts.push_back(math::Vec3f(2,4,0));
    verts.push_back(math::Vec3f(1,4,0));
    verts.push_back(math::Vec3f(0,4,0));
    verts.push_back(math::Vec3f(0.9,2,0));
    verts.push_back(math::Vec3f(1.1,2,0));

    faces.push_back(0); faces.push_back(1); faces.push_back(6);
    faces.push_back(1); faces.push_back(2); faces.push_back(7);
    faces.push_back(2); faces.push_back(3); faces.push_back(7);
    faces.push_back(3); faces.push_back(4); faces.push_back(7);
    faces.push_back(4); faces.push_back(5); faces.push_back(6);
    faces.push_back(5); faces.push_back(0); faces.push_back(6);
    faces.push_back(1); faces.push_back(7); faces.push_back(6);
    faces.push_back(4); faces.push_back(6); faces.push_back(7);

    mve::geom::save_mesh(mesh, "/tmp/testmesh.off");
    fssr::clean_slivers(mesh, 0.1f);
    mve::geom::save_mesh(mesh, "/tmp/testmesh_cleaned.off");

    // TODO: Write some EXPECT tests!
}
#endif

#if 0
TEST(MeshCleanTest, CleanTest2)
{
    mve::TriangleMesh::Ptr mesh = mve::geom::load_mesh("/tmp/camel_mc.off");
    std::size_t num_collapsed = 0;
    num_collapsed += fssr::clean_needles(mesh, 0.4f);
    mve::geom::save_mesh(mesh, "/tmp/camel_cleaned_1.ply");
    num_collapsed += fssr::clean_caps(mesh);
    mve::geom::save_mesh(mesh, "/tmp/camel_cleaned_2.ply");
    num_collapsed += fssr::clean_needles(mesh, 0.4f);
    mve::geom::save_mesh(mesh, "/tmp/camel_cleaned_3.ply");
    std::cout << "Collapsed " << num_collapsed << " edges." << std::endl;
}
#endif
