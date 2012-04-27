#include <iostream>

#include "math/matrix.h"
#include "math/matrixtools.h"
#include "math/vector.h"

#include "trianglemesh.h"
#include "meshtools.h"
#include "camera.h"
#include "scene.h"

#define SCENE_DIR "/gris/scratch/mve_datasets/hanau-101027/"

int main (void)
{
    std::cout << "Loading scene..." << std::endl;
    mve::Scene scene;
    scene.load_scene(SCENE_DIR);

    mve::View::Ptr view = scene.get_view_by_id(4);

    mve::CameraInfo const& cam(view->get_camera());

    math::Matrix4f wtc;
    cam.fill_world_to_cam(*wtc);
    math::Matrix3f rot(cam.rot);

    std::cout << "Det: " << math::matrix_determinant(rot) << std::endl;

    mve::BundleFile::ConstPtr bundle(scene.get_bundle());
    mve::BundleFile::FeaturePoints const& points(bundle->get_points());

    mve::TriangleMesh::Ptr mesh(mve::TriangleMesh::create());
    mve::TriangleMesh::VertexList& verts(mesh->get_vertices());
    mve::TriangleMesh::ColorList& colors(mesh->get_vertex_colors());

    for (std::size_t i = 0; i < points.size(); ++i)
    {
        math::Vec3f point(points[i].pos);
        math::Vec3uc color(points[i].color);

        verts.push_back(-wtc.mult(point, 1.0f));

        math::Vec4f fcolor(1.0f);
        for (int j = 0; j < 3; ++j)
            fcolor[j] = (float)color[j] / 255.0f;
        colors.push_back(fcolor);
    }

    mve::geom::save_mesh(mesh, "/tmp/testmesh.ply");

    return 0;
}
