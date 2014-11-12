#include <iostream>

#include "util/arguments.h"
#include "mve/bundle.h"
#include "mve/bundle_io.h"
#include "mve/mesh.h"
#include "mve/mesh_io_ply.h"

struct AppSettings
{
    std::string input_bundle;
    std::string output_ply;
};

int
main (int argc, char** argv)
{
    /* Setup argument parser. */
    util::Arguments args;
    args.set_usage(argv[0], "[ OPTIONS ] INPUT_BUNDLE OUTPUT_PLY");
    args.set_exit_on_error(true);
    args.set_nonopt_maxnum(2);
    args.set_nonopt_minnum(2);
    args.set_helptext_indent(22);
    args.set_description("This application reads a bundle file and "
        "outputs a PLY file with a colored point cloud.");
    //args.add_option('s', "spheres", true, "Export spheres with radius ARG instead of points [0.0]");
    args.parse(argc, argv);

    AppSettings conf;
    conf.input_bundle = args.get_nth_nonopt(0);
    conf.output_ply = args.get_nth_nonopt(1);

    mve::Bundle::Ptr bundle;
    try
    {
        bundle = mve::load_mve_bundle(conf.input_bundle);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error reading bundle: " << e.what() << std::endl;
        return 1;
    }

    mve::TriangleMesh::Ptr mesh = bundle->get_features_as_mesh();
    try
    {
        mve::geom::save_ply_mesh(mesh, conf.output_ply);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error writing PLY: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
