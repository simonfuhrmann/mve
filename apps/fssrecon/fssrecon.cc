/*
 * Floating Scale Surface Reconstruction driver app.
 * Written by Simon Fuhrmann.
 *
 * The surface reconstruction approach implemented here is described in:
 *
 *     Floating Scale Surface Reconstruction
 *     Simon Fuhrmann and Michael Goesele
 *     In: ACM ToG (Proceedings of ACM SIGGRAPH 2014).
 *     http://tinyurl.com/floating-scale-surface-recon
 */

#include <iostream>
#include <string>

#include "mve/mesh.h"
#include "mve/mesh_io_ply.h"
#include "util/timer.h"
#include "util/arguments.h"
#include "fssr/pointset.h"
#include "fssr/iso_octree.h"
#include "iso/SimonIsoOctree.h"
#include "iso/MarchingCubes.h"

struct AppSettings
{
    std::vector<std::string> in_files;
    std::string out_mesh;
    int skip_samples;
    float scale_factor;
    int refine_octree;
};

int
main (int argc, char** argv)
{
    /* Setup argument parser. */
    util::Arguments args;
    args.set_exit_on_error(true);
    args.set_nonopt_minnum(2);
    args.set_helptext_indent(25);
    args.set_usage(argv[0], "[ OPTS ] IN_PLY [ IN_PLY ... ] OUT_PLY");
    args.add_option('s', "scale-factor", true, "Multiply sample scale with factor [1.0]");
    args.add_option('r', "refine-octree", true, "Refines octree with N levels [0]");
    args.add_option('k', "skip-samples", true, "Skip input samples [0]");
    args.set_description("Samples the implicit function defined by the input "
        "samples and produces a surface mesh. The input samples must have "
        "normals and the \"values\" PLY attribute (the scale of the samples). "
        "Both confidence values and vertex colors are optional. The final "
        "surface should be cleaned (sliver triangles, isolated components, "
        "low-confidence vertices) afterwards.");
    args.parse(argc, argv);

    /* Init default settings. */
    AppSettings conf;
    conf.skip_samples = 0;
    conf.scale_factor = 1.0f;
    conf.refine_octree = 0;

    /* Scan arguments. */
    while (util::ArgResult const* arg = args.next_result())
    {
        if (arg->opt == NULL)
        {
            conf.in_files.push_back(arg->arg);
            continue;
        }

        switch (arg->opt->sopt)
        {
            case 's': conf.scale_factor = arg->get_arg<float>(); break;
            case 'k': conf.skip_samples = arg->get_arg<int>(); break;
            case 'r': conf.refine_octree = arg->get_arg<int>(); break;
            default:
                std::cerr << "Invalid option: " << arg->opt->sopt << std::endl;
                return 1;
        }
    }

    if (conf.in_files.size() < 2)
    {
        args.generate_helptext(std::cerr);
        return 1;
    }
    conf.out_mesh = conf.in_files.back();
    conf.in_files.pop_back();

    if (conf.refine_octree < 0 || conf.refine_octree > 3)
    {
        std::cerr << "Unreasonable refine level of " << conf.refine_octree
            << ", exiting." << std::endl;
        return 1;
    }

    /* Load input point set and insert samples in the octree. */
    util::WallTimer timer;
    fssr::IsoOctree octree;
    for (std::size_t i = 0; i < conf.in_files.size(); ++i)
    {
        std::cout << "Loading: " << conf.in_files[i] << "..." << std::endl;
        fssr::PointSet pset;
        pset.set_scale_factor(conf.scale_factor);
        pset.set_skip_samples(conf.skip_samples);
        try
        {
            pset.read_from_file(conf.in_files[i]);
        }
        catch (std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }

        std::cout << "Inserting samples into the octree..." << std::flush;
        timer.reset();
        octree.insert_samples(pset);
        std::cout << " took " << timer.get_elapsed() << "ms" << std::endl;
    }

    /* Refine octree if requested. Each iteration adds one level voxels. */
    if (conf.refine_octree > 0)
    {
        timer.reset();
        std::cout << "Refining octree..." << std::flush;
        for (int i = 0; i < conf.refine_octree; ++i)
            octree.refine_octree();
        std::cout << " took " << timer.get_elapsed() << "ms" << std::endl;
    }

    /* Make octree regular such that inner nodes have exactly 8 children. */
    {
        timer.reset();
        std::cout << "Making octree regular..." << std::flush;
        octree.make_regular_octree();
        std::cout << " took " << timer.get_elapsed() << "ms" << std::endl;
    }

    /* Compute voxels. */
    octree.print_stats(std::cout);
    octree.compute_voxels();

    /* Transfer octree. */
    std::cout << "Transfering octree and voxel data..." << std::flush;
    timer.reset();
    SimonIsoOctree iso_tree;
    iso_tree.set_octree(octree);
    octree.clear();
    std::cout << " took " << timer.get_elapsed() << "ms." << std::endl;

    /* Extract mesh from octree. */
    MarchingCubes::SetCaseTable();
    MarchingCubes::SetFullCaseTable();
    mve::TriangleMesh::Ptr mesh = iso_tree.extract_mesh();
    iso_tree.clear();

    /* Check if anything has been extracted. */
    if (mesh->get_vertices().empty())
    {
        std::cerr << "Isosurface does not contain any vertices." << std::endl;
        return 1;
    }

    /* Surfaces between voxels with zero confidence are ghosts. */
    std::cout << "Deleting zero confidence vertices..." << std::flush;
    {
        timer.reset();
        std::size_t num_vertices = mesh->get_vertices().size();
        mve::TriangleMesh::DeleteList delete_verts(num_vertices, false);
        for (std::size_t i = 0; i < num_vertices; ++i)
        if (mesh->get_vertex_confidences()[i] == 0.0f)
            delete_verts[i] = true;
        mesh->delete_vertices_fix_faces(delete_verts);
    }
    std::cout << " took " << timer.get_elapsed() << "ms." << std::endl;

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
    std::cout << "Mesh output file: " << conf.out_mesh << std::endl;
    mve::geom::save_ply_mesh(mesh, conf.out_mesh, ply_opts);

    std::cout << std::endl;
    std::cout << "All done. Remember to clean the output mesh." << std::endl;

    return 0;
}
