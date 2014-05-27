/*
 * App to extract an isosurface from the sampled implicit function.
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

#include "util/timer.h"
#include "util/arguments.h"
#include "mve/mesh.h"
#include "mve/mesh_tools.h"
#include "mve/mesh_io_ply.h"
#include "fssr/iso_octree.h"
#include "fssr/mesh_clean.h"
#include "iso/SimonIsoOctree.h"
#include "iso/MarchingCubes.h"

struct AppSettings
{
    std::string in_octree;
    std::string out_mesh;
    float conf_threshold;
    int component_size;
    bool clean_degenerated;
};

void
remove_low_conf_geometry (mve::TriangleMesh::Ptr mesh, float const thres)
{
    mve::TriangleMesh::ConfidenceList const& confs = mesh->get_vertex_confidences();
    std::vector<bool> delete_list(confs.size(), false);
    std::size_t num_deleted = 0;
    for (std::size_t i = 0; i < confs.size(); ++i)
    {
        if (confs[i] > thres)
            continue;
        num_deleted += 1;
        delete_list[i] = true;
    }
    mesh->delete_vertices_fix_faces(delete_list);
    std::cout << "Deleted " << num_deleted
        << " low-confidence vertices." << std::endl;
}

int
main (int argc, char** argv)
{
    /* Setup argument parser. */
    util::Arguments args;
    args.set_exit_on_error(true);
    args.set_nonopt_minnum(2);
    args.set_nonopt_maxnum(2);
    args.set_helptext_indent(25);
    args.set_usage(argv[0], "[ OPTS ] IN_OCTREE OUT_PLY_MESH");
    args.add_option('t', "threshold", true, "Threshold on the geometry confidence [1.0]");
    args.add_option('c', "component-size", true, "Minimum number of vertices per component [1000]");
    args.add_option('n', "no-clean", false, "Prevents cleanup of degenerated faces");
    args.set_description("Extracts the isosurface from the sampled implicit "
        "function from an input octree. The accumulated weights in the octree "
        "can be thresholded to extract reliable parts of the geometry only. "
        "Small isolated components may be removed using a threshold on the "
        "vertex amount per component. A cleanup procedure for Marching Cubes "
        "artifacts is executed, but can be disabled.");
    args.parse(argc, argv);

    /* Init default settings. */
    AppSettings conf;
    conf.in_octree = args.get_nth_nonopt(0);
    conf.out_mesh = args.get_nth_nonopt(1);
    conf.conf_threshold = 1.0f;
    conf.component_size = 1000;
    conf.clean_degenerated = true;

    /* Scan arguments. */
    while (util::ArgResult const* arg = args.next_result())
    {
        if (arg->opt == NULL)
            continue;

        switch (arg->opt->sopt)
        {
            case 't': conf.conf_threshold = arg->get_arg<float>(); break;
            case 'c': conf.component_size = arg->get_arg<int>(); break;
            case 'n': conf.clean_degenerated = false; break;
            default:
                std::cerr << "Invalid option: " << arg->opt->sopt << std::endl;
                return 1;
        }
    }

    /* Load octree. */
    std::cout << "Octree input file: " << conf.in_octree << std::endl;
    std::cout << "Loading octree from file..." << std::flush;
    util::WallTimer timer;
    fssr::IsoOctree octree;
    octree.read_from_file(conf.in_octree);
    std::cout << " took " << timer.get_elapsed() << "ms." << std::endl;
    std::cout << "Octree contains " << octree.get_voxels().size()
        << " voxels in " << octree.get_num_nodes() << " nodes." << std::endl;

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

    /* Remove low-confidence geometry. */
    std::cout << "Removing low-confidence geometry (threshold "
        << conf.conf_threshold << ")..." << std::endl;
    remove_low_conf_geometry(mesh, conf.conf_threshold);

    /* Check for color and delete if not existing. */
    mve::TriangleMesh::ColorList& colors = mesh->get_vertex_colors();
    if (!colors.empty() && colors[0].minimum() < 0.0f)
    {
        std::cout << "Removing dummy mesh coloring..." << std::endl;
        colors.clear();
    }

    /* Remove isolated components if requested. */
    if (conf.component_size > 0)
    {
        std::cout << "Removing isolated components with <"
            << conf.component_size << " vertices..." << std::endl;
        std::size_t num_verts = mesh->get_vertices().size();
        mve::geom::mesh_components(mesh, conf.component_size);
        std::size_t new_num_verts = mesh->get_vertices().size();
        std::cout << "Deleted " << (num_verts - new_num_verts)
            << " vertices in isolated regions." << std::endl;
    }

    /* Remove degenerated faces from the mesh. */
    if (conf.clean_degenerated)
    {
        std::cout << "Removing degenerated faces..." << std::flush;
        std::size_t num_collapsed = fssr::clean_mc_mesh(mesh);
        std::cout << " collapsed " << num_collapsed << " edges." << std::endl;
    }

    mve::geom::SavePLYOptions ply_opts;
    ply_opts.write_vertex_colors = true;
    ply_opts.write_vertex_confidences = true;
    ply_opts.write_vertex_values = true;
    std::cout << "Mesh output file: " << conf.out_mesh << std::endl;
    mve::geom::save_ply_mesh(mesh, conf.out_mesh, ply_opts);

    return 0;
}
