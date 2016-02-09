/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 *
 * The surface reconstruction approach implemented here is described in:
 *
 *     Floating Scale Surface Reconstruction
 *     Simon Fuhrmann and Michael Goesele
 *     In: ACM ToG (Proceedings of ACM SIGGRAPH 2014).
 *     http://tinyurl.com/floating-scale-surface-recon
 */

#include <cstdlib>
#include <iostream>
#include <string>

#include "mve/mesh.h"
#include "mve/mesh_io_ply.h"
#include "util/timer.h"
#include "util/arguments.h"
#include "util/system.h"
#include "fssr/sample_io.h"
#include "fssr/iso_octree.h"
#include "fssr/iso_surface.h"
#include "fssr/hermite.h"
#include "fssr/defines.h"

struct AppOptions
{
    std::vector<std::string> in_files;
    std::string out_mesh;
    int refine_octree = 0;
    fssr::InterpolationType interp_type = fssr::INTERPOLATION_CUBIC;
};

void
fssrecon (AppOptions const& app_opts, fssr::SampleIO::Options const& pset_opts)
{
    /* Load input point set and insert samples in the octree. */
    fssr::IsoOctree octree;
    for (std::size_t i = 0; i < app_opts.in_files.size(); ++i)
    {
        std::cout << "Loading: " << app_opts.in_files[i] << "..." << std::endl;
        util::WallTimer timer;

        fssr::SampleIO loader(pset_opts);
        loader.open_file(app_opts.in_files[i]);
        fssr::Sample sample;
        while (loader.next_sample(&sample))
            octree.insert_sample(sample);

        std::cout << "Loading samples took "
            << timer.get_elapsed() << "ms." << std::endl;
    }

    /* Exit if no samples have been inserted. */
    if (octree.get_num_samples() == 0)
    {
        std::cerr << "Octree does not contain any samples, exiting."
            << std::endl;
        std::exit(EXIT_FAILURE);
    }

    /* Refine octree if requested. Each iteration adds one level. */
    if (app_opts.refine_octree > 0)
    {
        std::cout << "Refining octree..." << std::flush;
        util::WallTimer timer;
        for (int i = 0; i < app_opts.refine_octree; ++i)
            octree.refine_octree();
        std::cout << " took " << timer.get_elapsed() << "ms" << std::endl;
    }

    /* Compute voxels. */
    octree.limit_octree_level();
    octree.print_stats(std::cout);
    octree.compute_voxels();
    octree.clear_samples();

    /* Extract isosurface. */
    mve::TriangleMesh::Ptr mesh;
    {
        std::cout << "Extracting isosurface..." << std::endl;
        util::WallTimer timer;
        fssr::IsoSurface iso_surface(&octree, app_opts.interp_type);
        mesh = iso_surface.extract_mesh();
        std::cout << "  Done. Surface extraction took "
            << timer.get_elapsed() << "ms." << std::endl;
    }
    octree.clear();

    /* Check if anything has been extracted. */
    if (mesh->get_vertices().empty())
    {
        std::cerr << "Isosurface does not contain any vertices, exiting."
            << std::endl;
        std::exit(EXIT_FAILURE);
    }

    /* Surfaces between voxels with zero confidence are ghosts. */
    {
        std::cout << "Deleting zero confidence vertices..." << std::flush;
        util::WallTimer timer;
        std::size_t num_vertices = mesh->get_vertices().size();
        mve::TriangleMesh::DeleteList delete_verts(num_vertices, false);
        for (std::size_t i = 0; i < num_vertices; ++i)
            if (mesh->get_vertex_confidences()[i] == 0.0f)
                delete_verts[i] = true;
        mesh->delete_vertices_fix_faces(delete_verts);
        std::cout << " took " << timer.get_elapsed() << "ms." << std::endl;
    }

    /* Check for color and delete if not existing. */
    mve::TriangleMesh::ColorList& colors = mesh->get_vertex_colors();
    if (!colors.empty() && colors[0].minimum() < 0.0f)
    {
        std::cout << "Removing dummy mesh coloring..." << std::endl;
        colors.clear();
    }

    /* Write output mesh. */
    mve::geom::SavePLYOptions ply_opts;
    ply_opts.write_vertex_colors = true;
    ply_opts.write_vertex_confidences = true;
    ply_opts.write_vertex_values = true;
    std::cout << "Mesh output file: " << app_opts.out_mesh << std::endl;
    mve::geom::save_ply_mesh(mesh, app_opts.out_mesh, ply_opts);
}

int
main (int argc, char** argv)
{
    util::system::register_segfault_handler();
    util::system::print_build_timestamp("Floating Scale Surface Reconstruction");

    /* Setup argument parser. */
    util::Arguments args;
    args.set_exit_on_error(true);
    args.set_nonopt_minnum(2);
    args.set_helptext_indent(25);
    args.set_usage(argv[0], "[ OPTS ] IN_PLY [ IN_PLY ... ] OUT_PLY");
    args.add_option('s', "scale-factor", true, "Multiply sample scale with factor [1.0]");
    args.add_option('r', "refine-octree", true, "Refines octree with N levels [0]");
    args.add_option('\0', "min-scale", true, "Minimum scale, smaller samples are clamped");
    args.add_option('\0', "max-scale", true, "Maximum scale, larger samples are ignored");
#if FSSR_USE_DERIVATIVES
    args.add_option('\0', "interpolation", true, "Interpolation: linear, scaling, lsderiv, [cubic]");
#endif // FSSR_USE_DERIVATIVES
    args.set_description("Samples the implicit function defined by the input "
        "samples and produces a surface mesh. The input samples must have "
        "normals and the \"values\" PLY attribute (the scale of the samples). "
        "Both confidence values and vertex colors are optional. The final "
        "surface should be cleaned (sliver triangles, isolated components, "
        "low-confidence vertices) afterwards.");
    args.parse(argc, argv);

    /* Init default settings. */
    AppOptions app_opts;
    fssr::SampleIO::Options pset_opts;

    /* Scan arguments. */
    while (util::ArgResult const* arg = args.next_result())
    {
        if (arg->opt == nullptr)
        {
            app_opts.in_files.push_back(arg->arg);
            continue;
        }

        if (arg->opt->lopt == "scale-factor")
            pset_opts.scale_factor = arg->get_arg<float>();
        else if (arg->opt->lopt == "refine-octree")
            app_opts.refine_octree = arg->get_arg<int>();
        else if (arg->opt->lopt == "min-scale")
            pset_opts.min_scale = arg->get_arg<float>();
        else if (arg->opt->lopt == "max-scale")
            pset_opts.max_scale = arg->get_arg<float>();
        else if (arg->opt->lopt == "interpolation")
        {
            if (arg->arg == "linear")
                app_opts.interp_type = fssr::INTERPOLATION_LINEAR;
            else if (arg->arg == "scaling")
                app_opts.interp_type = fssr::INTERPOLATION_SCALING;
            else if (arg->arg == "lsderiv")
                app_opts.interp_type = fssr::INTERPOLATION_LSDERIV;
            else if (arg->arg == "cubic")
                app_opts.interp_type = fssr::INTERPOLATION_CUBIC;
            else
            {
                args.generate_helptext(std::cerr);
                std::cerr << std::endl << "Error: Invalid interpolation: "
                    << arg->arg << std::endl;
                return 1;
            }
        }
        else
        {
            std::cerr << "Invalid option: " << arg->opt->sopt << std::endl;
            return EXIT_FAILURE;
        }
    }

    if (app_opts.in_files.size() < 2)
    {
        args.generate_helptext(std::cerr);
        return EXIT_FAILURE;
    }
    app_opts.out_mesh = app_opts.in_files.back();
    app_opts.in_files.pop_back();

    if (app_opts.refine_octree < 0 || app_opts.refine_octree > 3)
    {
        std::cerr << "Unreasonable refine level of "
            << app_opts.refine_octree << ", exiting." << std::endl;
        return EXIT_FAILURE;
    }

    try
    {
        fssrecon(app_opts, pset_opts);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "All done. Remember to clean the output mesh." << std::endl;

    return EXIT_SUCCESS;
}
