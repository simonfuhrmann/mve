#include <iostream>

#include "mve/view.h"
#include "mve/scene.h"
#include "mve/mesh.h"
#include "mve/depthmap.h"
#include "mve/mesh_io_ply.h"
#include "mve/mesh_tools.h"
#include "math/matrix.h"
#include "math/matrix_tools.h"

int
main (int argc, char** argv)
{
#if 1
    std::cout << "--- Scene tests ---" << std::endl;

    if (argc != 2)
    {
        std::cout << "Pass scene dir as argument!" << std::endl;
        std::exit(1);
    }

    std::cout << "Loading scene..." << std::endl;
    mve::Scene scene;
    scene.load_scene(argv[1]);
    /*
    std::cout << "Memory: " << scene.get_total_mem_usage() << std::endl;
    scene.get_bundle();
    std::cout << "Memory: " << scene.get_total_mem_usage() << std::endl;
    scene.cache_cleanup();
    std::cout << "Memory: " << scene.get_total_mem_usage() << std::endl;
    */

    mve::View::Ptr view1 = scene.get_view_by_id(1);
    mve::View::Ptr view2 = scene.get_view_by_id(4);

    std::cout << "Loading depthmaps..." << std::endl;
    mve::FloatImage::Ptr dm1 = view1->get_float_image("depthmap");
    mve::FloatImage::Ptr dm2 = view2->get_float_image("depthmap");

    float flen1 = view1->get_camera().flen;
    float flen2 = view2->get_camera().flen;

    std::cout << "Bilateral filtering DM1..." << std::endl;
    dm1 = mve::image::depthmap_bilateral_filter(dm1, flen1, 4.0f, 5.0f);

    std::cout << "Bilateral filtering DM2..." << std::endl;
    //dm2 = mve::image::depthmap_bilateral_filter(dm2, flen2, 4.0f, 5.0f);

    std::cout << "Triangulating depthmaps..." << std::endl;
    mve::TriangleMesh::Ptr m1 = mve::geom::depthmap_triangulate(dm1,
        mve::ByteImage::Ptr(0), flen1);
    mve::TriangleMesh::Ptr m2 = mve::geom::depthmap_triangulate(dm2,
        mve::ByteImage::Ptr(0), flen2);

    mve::geom::save_ply_mesh(m1, "/tmp/single_dm.ply");
    return 0;

    std::cout << "Transforming meshes to world coordinates..." << std::endl;
    math::Matrix4f m1_wtc;
    view1->get_camera().fill_world_to_cam(*m1_wtc);
    math::Matrix4f m2_wtc;
    view2->get_camera().fill_world_to_cam(*m2_wtc);

    math::Matrix4f m1_ctw = math::matrix_invert_trans(m1_wtc);
    math::Matrix4f m2_ctw = math::matrix_invert_trans(m2_wtc);

    mve::geom::mesh_transform(m1, m1_ctw);
    mve::geom::mesh_transform(m2, m2_ctw);

    std::cout << "Combining meshes..." << std::endl;
    mve::TriangleMesh::VertexList const& m1v = m1->get_vertices();
    mve::TriangleMesh::VertexList const& m2v = m2->get_vertices();
    mve::TriangleMesh::FaceList const& m1f = m1->get_faces();
    mve::TriangleMesh::FaceList const& m2f = m2->get_faces();

    mve::TriangleMesh::Ptr ret = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = ret->get_vertices();
    mve::TriangleMesh::FaceList& faces = ret->get_faces();

    verts.reserve(m1v.size() + m2v.size());
    faces.reserve(m1f.size() + m2f.size());

    verts.insert(verts.end(), m1v.begin(), m1v.end());
    verts.insert(verts.end(), m2v.begin(), m2v.end());
    faces.insert(faces.end(), m1f.begin(), m1f.end());
    for (std::size_t i = 0; i < m2f.size(); ++i)
        faces.push_back(m2f[i] + m1v.size());

    std::cout << "Saving mesh..." << std::endl;
    mve::geom::save_ply_mesh(ret, "/tmp/combined_dm.ply");

    
#if 0
    mve::ByteImage::Ptr img;
    img = mve::image::load_file("../../data/testimages/test_rgb_32x32.png");
    view->set_image("testimage", img);
    view->set_image("testimage2", img);
#endif

    //view->remove_embedding("testimage");
    //view->remove_embedding("testimage2");
    
    //scene.save_views();
#endif

    return 0;
}
