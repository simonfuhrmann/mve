#include "view.h"
#include "trianglemesh.h"
#include "meshtools.h"
#include "depthmap.h"

//#define MVE_VIEW "/data/dev/phd/mvs_datasets/mve_q3scene/views/view_0000.mve"
//#define MVE_VIEW "/gris/scratch/mve_datasets/q3scene-101118/views/view_0000.mve"
#define MVE_VIEW "/gris/scratch/mve_datasets/temple-ring-101201/views/view_0000.mve"

int main (void)
{
    mve::View::Ptr view = mve::View::create();
    view->load_mve_file(MVE_VIEW);
    mve::FloatImage::Ptr dm = view->get_float_image("depth-L0");
    mve::ByteImage::Ptr ci = view->get_byte_image("undist-L0");

    mve::TriangleMesh::Ptr mesh = mve::geom::depthmap_triangulate(dm, ci, view->get_camera().flen, 5.0f);

    mve::geom::save_mesh(mesh, "/tmp/depthmap-p0.off");
    mve::geom::depthmap_mesh_peeling(mesh, 1);
    mve::geom::save_mesh(mesh, "/tmp/depthmap-p1.off");
    mve::geom::depthmap_mesh_peeling(mesh, 1);
    mve::geom::save_mesh(mesh, "/tmp/depthmap-p2.off");
    mve::geom::depthmap_mesh_peeling(mesh, 1);
    mve::geom::save_mesh(mesh, "/tmp/depthmap-p3.off");

    mesh = mve::geom::depthmap_triangulate(dm, ci, view->get_camera().flen, 5.0f);
    mve::geom::depthmap_mesh_peeling(mesh, 3);
    mve::geom::save_mesh(mesh, "/tmp/depthmap-p3b.off");

    return 0;
}
