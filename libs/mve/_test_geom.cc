#include <iostream>

#include "trianglemesh.h"
#include "meshtools.h"
#include "offfile.h"
#include "plyfile.h"

int
main (void)
{
#if 0
    /* Cylinder mesh generation. */
    mve::TriangleMesh::Ptr mesh(mve::TriangleMesh::create());
    mve::TriangleMesh::VertexList& verts(mesh->get_vertices());
    mve::TriangleMesh::FaceList& faces(mesh->get_faces());

    for (std::size_t h = 0; h < 200; ++h)
        for (std::size_t i = 0; i < 360; ++i)
        {
            math::Vec3f p(std::sin(MATH_DEG2RAD((float)i)),
                (float)h / 100.0f - 1.0f,
                -std::cos(MATH_DEG2RAD((float)i)));
            verts.push_back(p);

            if (h == 0 || i == 0)
                continue;

            faces.push_back(h * 360 + i - 1);
            faces.push_back((h - 1) * 360 + i);
            faces.push_back((h - 1) * 360 + i - 1);
            faces.push_back(h * 360 + i);
            faces.push_back((h - 1) * 360 + i);
            faces.push_back(h * 360 + i - 1);
        }

    mve::geom::save_off_mesh(mesh, "/tmp/testmesh.off");

#endif


#if 1
    /* Test PLY file reading. */
    std::cout << "Loading PLY model..." << std::endl;
    std::string fname("/gris/gris-f/home/sfuhrman/offmodels/animals/bunny.off");
    mve::TriangleMesh::Ptr mesh = mve::geom::load_mesh(fname);
    mesh->ensure_normals();

    /*
    mve::TriangleMesh::ColorList const& vcolors(mesh->get_vertex_colors());
    mve::TriangleMesh::ColorList const& fcolors(mesh->get_face_colors());
    if (mesh->has_vertex_colors())
    {

    }
    */

    /* Write out OFF model. */
    std::cout << "Writing PLY model..." << std::endl;
    bool format_binary = true;
    bool use_vcolors = true;
    bool use_vnormals = false;
    bool use_fcolors = false;
    bool use_fnormals = false;
    bool use_conf = false;
    mve::geom::save_ply_mesh(mesh, "/tmp/testmesh.ply", format_binary,
        use_vcolors, use_vnormals, use_fcolors, use_fnormals, use_conf);
#endif

#if 0
    /* Cleaning duplicated vertices test. */

    mve::TriangleMesh::Ptr mesh;
    mesh = mve::geom::load_off_mesh("/data/offmodels/testing/testmesh5.off");

    mve::MeshCleanup cleaner(mesh);
    std::size_t erased = cleaner.delete_unreferenced();

    std::cout << "Erased " << erased << " vertices from mesh" << std::endl;
    if (erased == 0)
        return 0;

    mve::geom::save_off_mesh(mesh, "/tmp/testmesh.off");
#endif

    return 0;
}
