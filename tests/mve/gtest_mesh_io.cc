// Test cases for the MVE mesh file reader/writer.
// Written by Nils Moehrle.

#include <gtest/gtest.h>

#include "util/file_system.h"
#include "mve/mesh_io_obj.h"
#include "mve/mesh_io_ply.h"
#include "mve/mesh_io_off.h"

struct TempFile : public std::string
{
    TempFile (std::string const& postfix)
        : std::string(std::tmpnam(nullptr))
    {
        this->append(postfix);
    }

    ~TempFile (void)
    {
        util::fs::unlink(this->c_str());
    }
};


namespace
{
    mve::TriangleMesh::Ptr
    create_test_mesh (bool with_normals = false)
    {
        mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
        mve::TriangleMesh::VertexList& vertices = mesh->get_vertices();
        mve::TriangleMesh::NormalList& normals = mesh->get_vertex_normals();
        mve::TriangleMesh::FaceList& faces = mesh->get_faces();

        vertices.push_back(math::Vec3f(0.0f, 0.0f, 1.0f));
        vertices.push_back(math::Vec3f(0.0f, 1.0f, 0.0f));
        vertices.push_back(math::Vec3f(1.0f, 0.0f, 0.0f));

        if (with_normals)
        {
            normals.push_back(math::Vec3f(1.0f, 0.0f, 0.0f));
            normals.push_back(math::Vec3f(0.0f, 1.0f, 0.0f));
            normals.push_back(math::Vec3f(0.0f, 0.0f, 1.0f));
        }

        faces.push_back(0);
        faces.push_back(1);
        faces.push_back(2);

        return mesh;
    }

    template<typename T>
    bool
    eq (std::vector<T> const& vec1, std::vector<T> const& vec2)
    {
        return vec1.size() == vec2.size()
            && std::equal(vec1.begin(), vec1.end(), vec2.begin());
    }

    bool
    compare_mesh (mve::TriangleMesh::ConstPtr mesh1,
        mve::TriangleMesh::ConstPtr mesh2)
    {
        return eq(mesh1->get_vertices(), mesh2->get_vertices())
            && eq(mesh1->get_vertex_colors(), mesh2->get_vertex_colors())
            && eq(mesh1->get_vertex_confidences(), mesh2->get_vertex_confidences())
            && eq(mesh1->get_vertex_values(), mesh2->get_vertex_values())
            && eq(mesh1->get_vertex_normals(), mesh2->get_vertex_normals())
            && eq(mesh1->get_vertex_texcoords(), mesh2->get_vertex_texcoords())
            && eq(mesh1->get_faces(), mesh2->get_faces())
            && eq(mesh1->get_face_colors(), mesh2->get_face_colors());
    }
}

TEST(MeshFileTest, OBJSaveLoad)
{
    TempFile filename("objtest1");
    mve::TriangleMesh::Ptr mesh1 = create_test_mesh(), mesh2;
    try
    {
        std::vector<mve::geom::ObjModelPart> obj_model_parts;
        mve::geom::save_obj_mesh(mesh1, filename);
        mve::geom::load_obj_mesh(filename, &obj_model_parts);
        EXPECT_EQ(obj_model_parts.size(), 1);
        EXPECT_EQ(obj_model_parts[0].texture_filename, "");
        mesh2 = obj_model_parts[0].mesh;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return;
    }

    EXPECT_TRUE(compare_mesh(mesh1, mesh2));
}

TEST(MeshFileTest, PLYSaveLoad)
{
    TempFile filename("plytest1");
    mve::TriangleMesh::Ptr mesh1 = create_test_mesh(true), mesh2;
    mve::geom::SavePLYOptions options;
    options.write_vertex_normals = true;

    try
    {
        mve::geom::save_ply_mesh(mesh1, filename, options);
        mesh2 = mve::geom::load_ply_mesh(filename);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return;
    }

    EXPECT_TRUE(compare_mesh(mesh1, mesh2));
}

TEST(MeshFileTest, OFFSaveLoad)
{
    TempFile filename("offtest1");
    mve::TriangleMesh::Ptr mesh1 = create_test_mesh(), mesh2;

    try
    {
        mve::geom::save_off_mesh(mesh1, filename);
        mesh2 = mve::geom::load_off_mesh(filename);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return;
    }

    EXPECT_TRUE(compare_mesh(mesh1, mesh2));
}
