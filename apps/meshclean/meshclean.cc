/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <cstdlib>
#include <iostream>
#include <string>

#include "util/system.h"
#include "util/timer.h"
#include "util/arguments.h"
#include "mve/mesh.h"
#include "mve/mesh_io.h"
#include "mve/mesh_io_ply.h"
#include "mve/mesh_tools.h"
#include "fssr/mesh_clean.h"

struct AppSettings
{
    std::string in_mesh;
    std::string out_mesh;
    bool clean_degenerated;
    bool delete_scale;
    bool delete_conf;
    bool delete_colors;
    float conf_threshold;
    float conf_percentile;
    int component_size;
};


template <class InputIt>
typename std::iterator_traits<InputIt>::value_type
percentile(InputIt first, InputIt last, float p) {
    if (p <= 0.0f || 1.0f <= p)
        throw std::domain_error("p has to be in the open intervall (0.0,1.0)");

    typedef typename std::iterator_traits<InputIt>::value_type T;
    typedef typename std::iterator_traits<InputIt>::difference_type D;

    D num_elements = std::distance(first, last);
    if (num_elements < 1)
        throw std::invalid_argument("Invalid range");

    bool invert = p > 0.5f;
    if (invert) p = 1.0f - p;

    InputIt it = first + std::ceil(num_elements * p);

    std::vector<T> heap(first, it);
    if (invert) std::for_each(heap.begin(), heap.end(), [] (T &v) {v = -v;});
    std::make_heap(heap.begin(), heap.end());

    for (; it != last; ++it)
    {
        T v = invert ? -*it : *it;
        if (heap.front() < v) continue;
        heap.push_back(v);
        std::push_heap(heap.begin(), heap.end());
        std::pop_heap(heap.begin(), heap.end());
        heap.pop_back();
    }

    return invert ? -heap.front() : heap.front();
}

void
remove_low_conf_vertices (mve::TriangleMesh::Ptr mesh, float const thres)
{
    mve::TriangleMesh::ConfidenceList const& confs = mesh->get_vertex_confidences();
    std::vector<bool> delete_list(confs.size(), false);
    for (std::size_t i = 0; i < confs.size(); ++i)
    {
        if (confs[i] > thres)
            continue;
        delete_list[i] = true;
    }
    mesh->delete_vertices_fix_faces(delete_list);
}

int
main (int argc, char** argv)
{
    util::system::register_segfault_handler();
    util::system::print_build_timestamp("MVE FSSR Mesh Cleaning");

    /* Setup argument parser. */
    util::Arguments args;
    args.set_exit_on_error(true);
    args.set_nonopt_minnum(2);
    args.set_nonopt_maxnum(2);
    args.set_helptext_indent(25);
    args.set_usage(argv[0], "[ OPTS ] IN_MESH OUT_MESH");
    args.add_option('t', "threshold", true, "Threshold on the geometry confidence [1.0]");
    args.add_option('p', "percentile", true, "Use the nth percentile (0 - 100) as confidence threshold [disabled]");
    args.add_option('c', "component-size", true, "Minimum number of vertices per component [1000]");
    args.add_option('n', "no-clean", false, "Prevents cleanup of degenerated faces");
    args.add_option('\0', "delete-scale", false, "Delete scale attribute from mesh");
    args.add_option('\0', "delete-conf", false, "Delete confidence attribute from mesh");
    args.add_option('\0', "delete-color", false, "Delete color attribute from mesh");
    args.set_description("The application cleans degenerated faces resulting "
        "from MC-like algorithms. Vertices below a confidence threshold and "
        "vertices in small isolated components are deleted as well.");
    args.parse(argc, argv);

    /* Init default settings. */
    AppSettings conf;
    conf.in_mesh = args.get_nth_nonopt(0);
    conf.out_mesh = args.get_nth_nonopt(1);
    conf.conf_threshold = 1.0f;
    conf.conf_percentile = -1.0f;
    conf.component_size = 1000;
    conf.clean_degenerated = true;
    conf.delete_scale = false;
    conf.delete_conf = false;
    conf.delete_colors = false;

    /* Scan arguments. */
    while (util::ArgResult const* arg = args.next_option())
    {
        if (arg->opt->lopt == "threshold")
            conf.conf_threshold = arg->get_arg<float>();
        if (arg->opt->lopt == "percentile")
            conf.conf_percentile = arg->get_arg<float>();
        else if (arg->opt->lopt == "component-size")
            conf.component_size = arg->get_arg<int>();
        else if (arg->opt->lopt == "no-clean")
            conf.clean_degenerated = false;
        else if (arg->opt->lopt == "delete-scale")
            conf.delete_scale = true;
        else if (arg->opt->lopt == "delete-conf")
            conf.delete_conf = true;
        else if (arg->opt->lopt == "delete-color")
            conf.delete_colors = true;
        else
        {
            std::cerr << "Invalid option: " << arg->opt->sopt << std::endl;
            return EXIT_FAILURE;
        }
    }

    /* Load input mesh. */
    mve::TriangleMesh::Ptr mesh;
    try
    {
        std::cout << "Loading mesh: " << conf.in_mesh << std::endl;
        mesh = mve::geom::load_mesh(conf.in_mesh);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error loading mesh: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    /* Sanity checks. */
    if (mesh->get_vertices().empty())
    {
        std::cerr << "Error: Mesh is empty!" << std::endl;
        return EXIT_FAILURE;
    }

    if (!mesh->has_vertex_confidences() && conf.conf_threshold > 0.0f)
    {
        std::cerr << "Error: Confidence cleanup requested, but mesh "
            "has no confidence values." << std::endl;
        return EXIT_FAILURE;
    }

    if (mesh->get_faces().empty()
        && (conf.clean_degenerated || conf.component_size > 0))
    {
        std::cerr << "Error: Components/faces cleanup "
            "requested, but mesh has no faces." << std::endl;
        return EXIT_FAILURE;
    }

    /* Remove low-confidence geometry. */
    if (conf.conf_percentile > 0.0f)
    {
        mve::TriangleMesh::ConfidenceList const & confidences =
            mesh->get_vertex_confidences();
        conf.conf_threshold = percentile(confidences.begin(), confidences.end(),
            conf.conf_percentile / 100.0f);
    }

    if (conf.conf_threshold > 0.0f)
    {
        std::cout << "Removing low-confidence geometry (threshold "
            << conf.conf_threshold << ")..." << std::endl;
        std::size_t num_verts = mesh->get_vertices().size();
        remove_low_conf_vertices(mesh, conf.conf_threshold);
        std::size_t new_num_verts = mesh->get_vertices().size();
        std::cout << "  Deleted " << (num_verts - new_num_verts)
            << " low-confidence vertices." << std::endl;
    }

    /* Remove isolated components if requested. */
    if (conf.component_size > 0)
    {
        std::cout << "Removing isolated components below "
            << conf.component_size << " vertices..." << std::endl;
        std::size_t num_verts = mesh->get_vertices().size();
        mve::geom::mesh_components(mesh, conf.component_size);
        std::size_t new_num_verts = mesh->get_vertices().size();
        std::cout << "  Deleted " << (num_verts - new_num_verts)
            << " vertices in isolated regions." << std::endl;
    }

    /* Remove degenerated faces from the mesh. */
    if (conf.clean_degenerated)
    {
        std::cout << "Removing degenerated faces..." << std::endl;
        std::size_t num_collapsed = fssr::clean_mc_mesh(mesh);
        std::cout << "  Collapsed " << num_collapsed << " edges." << std::endl;
    }

    /* Write output mesh. */
    std::cout << "Writing mesh: " << conf.out_mesh << std::endl;
    if (util::string::right(conf.out_mesh, 4) == ".ply")
    {
        mve::geom::SavePLYOptions ply_opts;
        ply_opts.write_vertex_colors = !conf.delete_colors;
        ply_opts.write_vertex_confidences = !conf.delete_conf;
        ply_opts.write_vertex_values = !conf.delete_scale;
        mve::geom::save_ply_mesh(mesh, conf.out_mesh, ply_opts);
    }
    else
    {
        mve::geom::save_mesh(mesh, conf.out_mesh);
    }

    return EXIT_SUCCESS;
}
