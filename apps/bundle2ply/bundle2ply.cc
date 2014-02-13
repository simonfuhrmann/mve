#include <iostream>

#include "mve/bundle.h"
#include "mve/bundle_io.h"
#include "mve/mesh.h"
#include "mve/mesh_io.h"

int
main (int argc, char** argv)
{
    if (argc != 3)
    {
        std::cout << "Syntax: " << argv[0] << " <bundle> <ply>" << std::endl;
        std::exit(1);
    }

    mve::Bundle::Ptr bundle;
    try
    {
        bundle = mve::load_mve_bundle(argv[1]);
    }
    catch (std::exception& e)
    {
        std::cout << "Error reading bundle: " << e.what() << std::endl;
        std::exit(1);
    }

    mve::TriangleMesh::Ptr mesh = bundle->get_features_as_mesh();
    try
    {
        mve::geom::save_mesh(mesh, argv[2]);
    }
    catch (std::exception& e)
    {
        std::cout << "Error writing PLY: " << e.what() << std::endl;
        std::exit(1);
    }

    return 0;
}
