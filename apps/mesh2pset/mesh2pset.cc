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
#include <limits>
#include <vector>

#include "util/system.h"
#include "util/tokenizer.h"
#include "util/timer.h"
#include "util/arguments.h"
#include "mve/mesh.h"
#include "mve/mesh_tools.h"
#include "mve/mesh_io.h"
#include "mve/mesh_io_ply.h"
#include "mve/mesh_info.h"
#include "mve/depthmap.h"

struct AppSettings
{
    std::string in_mesh;
    std::string out_pointset;
    std::string aabb;
    float sample_scale = 0.0f;
    float scale_factor = 1.0f;
    bool no_confidences = false;
    bool no_scale_values = false;
    bool no_normals = false;
};

void
split_mesh (mve::TriangleMesh::Ptr mesh, std::string const& fname)
{
    /* Original mesh. */
    mve::TriangleMesh::VertexList const& verts = mesh->get_vertices();
    mve::TriangleMesh::ConfidenceList const& conf
        = mesh->get_vertex_confidences();
    mve::TriangleMesh::ValueList const& values = mesh->get_vertex_values();
    mve::TriangleMesh::NormalList const& norm = mesh->get_vertex_normals();

    /* New mesh. */
    mve::TriangleMesh::Ptr amesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& averts = amesh->get_vertices();
    mve::TriangleMesh::ConfidenceList& aconf
        = amesh->get_vertex_confidences();
    mve::TriangleMesh::ValueList& avalue = amesh->get_vertex_values();
    mve::TriangleMesh::NormalList& anorm = amesh->get_vertex_normals();

    std::vector<bool> delete_list(verts.size(), false);
    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        if (i % 10 != 0) // Skip 90%
            continue;
        delete_list[i] = true;
        averts.push_back(verts[i]);
        aconf.push_back(conf[i]);
        avalue.push_back(values[i]);
        anorm.push_back(norm[i]);
    }
    mesh->delete_vertices(delete_list); // Delete 10%

    mve::geom::SavePLYOptions ply_options;
    ply_options.format_binary = true;
    ply_options.write_vertex_values = true;
    ply_options.write_vertex_normals = true;
    ply_options.write_vertex_confidences = true;
    mve::geom::save_ply_mesh(amesh, fname, ply_options);
}

int
main (int argc, char** argv)
{
    util::system::register_segfault_handler();
    util::system::print_build_timestamp("MVE FSSR Mesh to Pointset");

    /* Setup argument parser. */
    util::Arguments args;
    args.set_exit_on_error(true);
    args.set_nonopt_minnum(2);
    args.set_nonopt_maxnum(2);
    args.set_helptext_indent(25);
    args.add_option('s', "scale", true, "Set constant scale for all samples [off]");
    args.add_option('a', "adaptive", true, "Average distance to neighbors scale factor [1.0]");
    args.add_option('b', "bounding-box", true, "Six comma separated values used as AABB [off]");
    args.add_option('c', "no-confidences", false, "Do not compute vertex confidences");
    args.add_option('x', "no-scale-values", false, "Do not compute sample scale");
    args.add_option('n', "no-normals", false, "Do not compute sample normals");
    args.set_usage(argv[0], "[ OPTS ] IN_MESH OUT_PLY_PSET");
    args.set_description("This app creates a PLY point cloud from the input "
        "mesh by stripping the connectivity information. Scale values "
        "are computed for each vertex as the average distance to each "
        "neighbor (using the connectivity information). Confidence "
        "values are computed by down-weighting boundary vertices.");
    args.parse(argc, argv);

    /* Init default settings. */
    AppSettings conf;
    conf.in_mesh = args.get_nth_nonopt(0);
    conf.out_pointset = args.get_nth_nonopt(1);

    /* Scan arguments. */
    while (util::ArgResult const* arg = args.next_result())
    {
        if (arg->opt == nullptr)
            continue;

        switch (arg->opt->sopt)
        {
            case 's': conf.sample_scale = arg->get_arg<float>(); break;
            case 'a': conf.scale_factor = arg->get_arg<float>(); break;
            case 'b': conf.aabb = arg->arg; break;
            case 'c': conf.no_confidences = true; break;
            case 'x': conf.no_scale_values = true; break;
            case 'n': conf.no_normals = true; break;
            default:
                std::cerr << "Invalid option: " << arg->opt->sopt << std::endl;
                return EXIT_FAILURE;
        }
    }

    /* If requested, use given AABB. */
    math::Vec3f aabb_min(-std::numeric_limits<float>::max());
    math::Vec3f aabb_max(std::numeric_limits<float>::max());
    if (!conf.aabb.empty())
    {
        util::Tokenizer tok;
        tok.split(conf.aabb, ',');
        if (tok.size() != 6)
        {
            std::cerr << "Error: Invalid AABB: " << conf.aabb << std::endl;
            return EXIT_FAILURE;
        }

        for (int i = 0; i < 3; ++i)
        {
            aabb_min[i] = util::string::convert<float>(tok[i]);
            aabb_max[i] = util::string::convert<float>(tok[i + 3]);
        }
        std::cout << "Using AABB: (" << aabb_min << ") / ("
            << aabb_max << ")" << std::endl;
    }

    /* Read input mesh. */
    mve::TriangleMesh::Ptr mesh = mve::geom::load_mesh(conf.in_mesh);
    mve::TriangleMesh::VertexList const& verts = mesh->get_vertices();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();
    mve::TriangleMesh::ValueList& values = mesh->get_vertex_values();

    if (!conf.no_normals)
        mesh->ensure_normals(false, true);

    /* Set scale. */
    values.resize(verts.size(), conf.sample_scale);
    if (conf.sample_scale <= 0.0f)
    {
        std::cout << "Computing scale..." << std::endl;
        mve::MeshInfo mesh_info(mesh);
        std::size_t num_unreferenced = 0;
        for (std::size_t i = 0; i < mesh_info.size(); ++i)
        {
            mve::MeshInfo::VertexInfo const& vi = mesh_info[i];
            if (vi.verts.size() < 3)
            {
                num_unreferenced += 1;
                values[i] = 0.0f;
                continue;
            }

            float avg_distance = 0.0f;
            for (std::size_t j = 0; j < vi.verts.size(); ++j)
                avg_distance += (verts[i] - verts[vi.verts[j]]).norm();
            avg_distance /= static_cast<float>(vi.verts.size());
            values[i] = avg_distance * conf.scale_factor;
        }

        if (num_unreferenced > 0)
        {
            std::cout << "Warning: " << num_unreferenced
                << " unreferenced vertices." << std::endl;
        }
    }
    else
    {
        std::cout << "Setting constant scale "
            << conf.sample_scale << std::endl;
    }

    /* Compute confidences. */
    if (!conf.no_confidences)
    {
        std::cout << "Computing mesh confidences..." << std::endl;
        mve::geom::depthmap_mesh_confidences(mesh, 3);
    }

    /* Drop triangles. */
    faces.clear();

    /* Drop vertices outside AABB. */
    if (!conf.aabb.empty())
    {
        std::cout << "Deleting vertices outside AABB..." << std::endl;
        std::vector<bool> delete_list(verts.size(), false);
        std::size_t num_outside_aabb = 0;
        for (std::size_t i = 0; i < verts.size(); ++i)
            for (int j = 0; j < 3; ++j)
                if (verts[i][j] < aabb_min[j] || verts[i][j] > aabb_max[j])
                {
                    delete_list[i] = true;
                    num_outside_aabb += 1;
                }
        mesh->delete_vertices(delete_list);

        std::cout << "Info: Deleted " << num_outside_aabb
            << " vertices outside AABB." << std::endl;
    }

    /* Separate 10% of the samples and save to different mesh. */
    //split_mesh(mesh, "/tmp/splitted.ply");

    /* Save output mesh. */
    mve::geom::SavePLYOptions ply_options;
    ply_options.format_binary = true;
    ply_options.write_vertex_normals = !conf.no_normals;
    ply_options.write_vertex_confidences = !conf.no_confidences;
    ply_options.write_vertex_values = !conf.no_scale_values;
    mve::geom::save_ply_mesh(mesh, conf.out_pointset, ply_options);

    return EXIT_SUCCESS;
}
