#include <iostream>
#include <string>
#include <cstdlib>

#include "util/arguments.h"
#include "mve/meshtools.h"

struct AppSettings
{
    std::string infile;
    std::string outfile;
    bool compute_normals;
};

int
main (int argc, char** argv)
{
    /* Setup argument parser. */
    util::Arguments args;
    args.set_exit_on_error(true);
    args.set_nonopt_minnum(2);
    args.set_nonopt_maxnum(2);
    args.set_helptext_indent(25);
    args.set_usage("Usage: meshconvert [ OPTS ] IN_MESH OUT_MESH");
    args.set_description(
        "Converts the mesh given by IN_MESH to the output file OUT_MESH. "
        "The format of the input and output mesh are detected by extension. "
        "Supported file formats are .off, .ply (Stanford), .npts (PoissonSR) "
        "and .pbrt.");
    args.add_option('n', "normals", false, "Compute vertex normals");
    args.parse(argc, argv);

    /* Init default settings. */
    AppSettings conf;
    conf.infile = args.get_nth_nonopt(0);
    conf.outfile = args.get_nth_nonopt(1);
    conf.compute_normals = false;

    for (util::ArgResult const* arg = args.next_result();
        arg != 0; arg = args.next_result())
    {
        if (arg->opt == 0)
            continue;

        switch (arg->opt->sopt)
        {
            case 'n': conf.compute_normals = true; break;
            default: throw std::runtime_error("Invalid option");
        }
    }

    mve::TriangleMesh::Ptr mesh;
    try
    {
        mesh = mve::geom::load_mesh(conf.infile);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error loading mesh: " << e.what() << std::endl;
        std::exit(1);
    }

    if (conf.compute_normals)
        mesh->ensure_normals();

    try
    {
        mve::geom::save_mesh(mesh, conf.outfile);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error saving mesh: " << e.what() << std::endl;
        std::exit(1);
    }

    return 0;
}
