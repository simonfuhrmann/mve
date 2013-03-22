#include <cmath>
#include <iostream>

#include "util/string.h"
#include "util/timer.h"
#include "util/system.h"
#include "util/endian.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/algo.h"
#include "mve/defines.h"
#include "mve/image.h"
#include "mve/imagetools.h"
#include "mve/imagefile.h"
#include "mve/view.h"
#include "mve/scene.h"
#include "mve/plyfile.h"
#include "mve/bilateral.h"
#include "mve/trianglemesh.h"
#include "mve/vertexinfo.h"
#include "mve/offfile.h"
#include "mve/depthmap.h"
#include "mve/meshtools.h"
#include "mve/volume.h"
#include "mve/marchingcubes.h"

/* ---------------------------------------------------------------- */

int main (int argc, char** argv)
{
    if (argc == 0)
        argv = 0;


#if 1
    // Timing test for image operations.
    util::WallTimer timer;
    for (int i = 0; i < 100; ++i) {
        mve::FloatImage img(1000, 1000, 5);
        img.delete_channel(3);
    }
    std::cout << "Took: " << timer.get_elapsed() << std::endl;
#endif


#if 0

    /* Triangulate depth map. */

    //mve::View::Ptr view = mve::View::create("/data/dev/phd/mvs_datasets/mve_q3scene/views/view_0000.mve");
    mve::View::Ptr view = mve::View::create("/gris/scratch/home/sfuhrman/q3scene_mve/views/view_0000.mve");
    mve::FloatImage::Ptr dm = view->get_float_image("depthmap");
    mve::ByteImage::Ptr ci = view->get_byte_image("undistorted");
    if (!dm.get())
    {
        std::cout << "No such embedding!" << std::endl;
        return 1;
    }

    mve::TriangleMesh::Ptr mesh = mve::geom::depthmap_triangulate(dm, ci,
        view->get_camera().flen, 3.0f);
    std::cout << "Saving mesh to file named " << std::endl;
    mve::geom::save_ply_mesh(mesh, "/tmp/mesh_mvs_bi.ply");

#endif


#if 0
    /* Matrix-vector foreach functor test. */
    mve::TriangleMesh::Ptr mesh(mve::TriangleMesh::create());
    mve::TriangleMesh::VertexList& verts(mesh->get_vertices());
    verts.push_back(math::Vec3f(0.0f, 0.0f, 0.0f));
    verts.push_back(math::Vec3f(1.0f, 0.0f, 0.0f));
    verts.push_back(math::Vec3f(0.0f, 1.0f, 0.0f));
    verts.push_back(math::Vec3f(0.0f, 0.0f, 1.0f));

    mve::geom::save_ply_mesh(mesh, "/tmp/mesh_simple1.ply");

    math::Matrix3f rot(0.0f);
    rot(0,1) = 1.0f;
    rot(1,2) = 1.0f;
    rot(2,0) = 1.0f;

    mve::geom::mesh_transform(mesh, rot);

    mve::geom::save_ply_mesh(mesh, "/tmp/mesh_simple2.ply");
#endif

#if 0
    mve::TriangleMesh::Ptr mesh(mve::TriangleMesh::create());
    mve::TriangleMesh::VertexList& verts(mesh->get_vertices());
    mve::TriangleMesh::ColorList& colors(mesh->get_vertex_colors());

    for (int v = 0; v < 3; ++v)
    {
    std::string viewname = "view_" + util::string::get_filled(v, 4) + ".mve";

    /* Depthmap to colored PLY point cloud. */
    mve::View::Ptr view = mve::View::create("/gris/scratch/mve_datasets/q3scene-101118/views/" + viewname);
    mve::FloatImage::Ptr dm = view->get_float_image("depth-L2");
    mve::ByteImage::Ptr bi = view->get_byte_image("undist-L2");
    mve::CameraInfo const& cam = view->get_camera();

    math::Matrix4f ctw_mat;
    cam.fill_cam_to_world(*ctw_mat);

    std::size_t w = dm->width();
    std::size_t h = dm->height();
    float flen = cam.flen;

    for (std::size_t y = 0; y < h; ++y)
        for (std::size_t x = 0; x < w; ++x)
        {
            float depth = dm->at(x, y, 0);
            if (depth <= 0.0f)
                continue;

            math::Vec4f color(1.0f);
            for (int i = 0; i < 3; ++i)
                color[i] = (1.0f / 255.0f) * (float)bi->at(x, y, i);

            math::Vec3f pos = mve::geom::pixel_3dpos(x, y, depth, w, h, flen);

            verts.push_back(ctw_mat.mult(pos, 1.0f));
            colors.push_back(color);
        }

    } // for v

    mve::geom::save_ply_mesh(mesh, "/tmp/pointset.ply");

#endif


#if 0

    mve::View::Ptr view = mve::View::create("/gris/scratch/home/pmuecke/q3scene2mve/views/view_0001.mve");
    mve::FloatImage::Ptr img = view->get_float_image("depthmap");

    std::cout << "Cleaning depth map..." << std::endl;
    mve::FloatImage::Ptr cleaned = mve::image::depthmap_cleanup(img, 50);
    view->set_image("dm-cleaned", cleaned);
    view->save_mve_file();

#endif

#if 0

    /* Depthmap confidence clean. */
    mve::View::Ptr view = mve::View::create("/data/dev/phd/mvs_datasets/mve_brownhouse/views/view_0000.mve");
    mve::FloatImage::Ptr dm = view->get_float_image("dm2");
    mve::FloatImage::ConstPtr cm = view->get_float_image("confidence");

    mve::image::depthmap_confidence_clean(dm, cm);
    view->mark_as_dirty("dm2");
    view->save_mve_file();


#endif

#if 0

    //mve::View::Ptr view = mve::View::create("/gris/scratch/home/pmuecke/q3scene2mve/views/view_0000.mve");
    mve::View::Ptr view = mve::View::create("/data/dev/phd/mvs_datasets/mve_brownhouse/views/view_0000.mve");

    mve::FloatImage::Ptr img = view->get_float_image("dm2");

    std::cout << "Filling depth map..." << std::endl;
    mve::FloatImage::Ptr filled = mve::image::depthmap_fill(img);

    view->set_image("dm-dilated", filled);
    view->save_mve_file();

    /*
    mve::geom::save_ply_file("/tmp/testing/filled_in.ply",
        view->get_camera(),
        img,
        view->get_float_image("confidence"),
        view->get_byte_image("undistorted"));

    mve::geom::save_ply_file("/tmp/testing/filled_out.ply",
        view->get_camera(),
        filled,
        mve::FloatImage::Ptr(),
        view->get_byte_image("undistorted"));
    */


#endif


#if 0

    mve::TriangleMesh::Ptr mesh(mve::geom::load_off_mesh("/data/offmodels/objects/julius-10k.off"));
    //mve::TriangleMesh::Ptr mesh(mve::geom::load_off_mesh("/gris/scratch/home/jackerma/mvsps/frog.off"));

    util::ClockTimer timer;
    mve::VertexInfoList::Ptr vinfo(mve::VertexInfoList::create(mesh));
    std::cout << "Took " << timer.get_elapsed() << "ms." << std::endl;
    vinfo->print_debug();
    vinfo.reset();

    /*
    mve::MeshVertexInfo const& vi(vinfo->at(42614));

    std::cout << "Vertices: ";
    for (std::size_t v = 0; v < vi.verts.size(); ++v)
        std::cout << vi.verts[v] << " ";
    std::cout << std::endl;

    std::cout << "Faces: ";
    for (std::size_t v = 0; v < vi.faces.size(); ++v)
        std::cout << vi.faces[v] << " ";
    std::cout << std::endl;
    */

#endif


#if 0

    mve::View::Ptr view = mve::View::create("/gris/scratch/home/pmuecke/q3scene2mve/views/view_0000.mve");
    //mve::View::Ptr view = mve::View::create("/data/dev/phd/mvs_datasets/mve_brownhous/views/view_0000.mve");

    mve::FloatImage::Ptr img = view->get_float_image("depthmap");

    std::cout << "Running bilateral filter..." << std::endl;
    mve::FloatImage::Ptr out = mve::image::bilateral_depthmap(img,
        view->get_camera().flen, 4.0f, 10.0f);

    mve::geom::save_ply_file("/tmp/testing/bilateral_in.ply",
        view->get_camera(),
        img,
        view->get_float_image("confidence"),
        view->get_byte_image("undistorted"));

    mve::geom::save_ply_file("/tmp/testing/bilateral_out.ply",
        view->get_camera(),
        out,
        view->get_float_image("confidence"),
        view->get_byte_image("undistorted"));

#endif


#if 0
    util::ClockTimer timer;

    mve::ByteImage::Ptr orig = mve::image::load_file
        ("../../data/testimages/lenna_gray_small.png");
    mve::FloatImage::Ptr img(mve::image::byte_to_float_image(orig));

    int a = 0;
    int b = 0;
    for (float gc_sigma = 1.0f; gc_sigma < 4.1f; gc_sigma *= 2, a++) // 4 times
        for (float pc_sigma = 0.05f; pc_sigma < 0.9f; pc_sigma *= 2, b++) // 5 times
        {
            mve::FloatImage::Ptr ret;
            ret = mve::image::bilateral_gray_image(img, gc_sigma, pc_sigma);

            mve::image::save_file(mve::image::float_to_byte_image(ret),
                "/tmp/test_out2_" + util::string::get(gc_sigma) + "_"
                + util::string::get(pc_sigma) + ".png");
        }

    std::cout << "Took " << timer.get_elapsed() << " ms." << std::endl;

#endif


#if 0
    if (argc != 2)
    {
        std::cout << "call with a PNG or JPG as argument" << std::endl;
        return 1;
    }

    mve::ByteImage img;
    try
    {
        img.load_file(argv[1]);
    }
    catch (util::Exception& e)
    {
        std::cout << "Error loading image: " << e << std::endl;
        return 1;
    }

    std::cout << "Image loaded from file. "
        << "Width: " << img.width() << ", height: " << img.height()
        << std::endl;
#endif

    return 0;
}

