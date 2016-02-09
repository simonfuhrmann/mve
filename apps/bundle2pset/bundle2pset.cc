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

#include "util/system.h"
#include "util/arguments.h"
#include "mve/bundle.h"
#include "mve/bundle_io.h"
#include "mve/mesh.h"
#include "mve/mesh_io_ply.h"

struct AppSettings
{
    std::string input_bundle;
    std::string output_ply;
    float sphere_radius = 0.0f;
};

float ico_verts[12][3] =
{
    { 0.0f, -0.5257311f, 0.8506508f },
    { 0.0f, 0.5257311f, 0.8506508f },
    { 0.0f, -0.5257311f, -0.8506508f },
    { 0.0f, 0.5257311f, -0.8506508f },
    { 0.8506508f, 0.0f, 0.5257311f },
    { 0.8506508f, 0.0f, -0.5257311f },
    { -0.8506508f, 0.0f, 0.5257311f },
    { -0.8506508f, 0.0f, -0.5257311f },
    { 0.5257311f, 0.8506508f, 0.0f },
    { 0.5257311f, -0.8506508f, 0.0f },
    { -0.5257311f, 0.8506508f, 0.0f },
    { -0.5257311f, -0.8506508f, 0.0f }
};

unsigned int ico_faces[20][3] =
{
    { 0, 4, 1 },
    { 0, 9, 4 },
    { 9, 5, 4 },
    { 4, 5, 8 },
    { 4, 8, 1 },
    { 8, 10, 1 },
    { 8, 3, 10 },
    { 5, 3, 8 },
    { 5, 2, 3 },
    { 2, 7, 3 },
    { 7, 10, 3 },
    { 7, 6, 10 },
    { 7, 11, 6 },
    { 11, 0, 6 },
    { 0, 1, 6 },
    { 6, 1, 10 },
    { 9, 0, 11 },
    { 9, 11, 2 },
    { 9, 2, 5 },
    { 7, 2, 11 }
};

mve::TriangleMesh::Ptr
generate_spheres (mve::TriangleMesh::Ptr mesh, AppSettings const& conf)
{
    mve::TriangleMesh::Ptr out = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& out_verts = out->get_vertices();
    mve::TriangleMesh::ColorList& out_colors = out->get_vertex_colors();
    mve::TriangleMesh::FaceList& out_faces = out->get_faces();

    for (std::size_t i = 0; i < mesh->get_vertices().size(); ++i)
    {
        unsigned int vertex_base = out_verts.size();
        for (int j = 0; j < 12; ++j)
        {
            out_verts.push_back(mesh->get_vertices()[i] + math::Vec3f(ico_verts[j]) * conf.sphere_radius);
            out_colors.push_back(mesh->get_vertex_colors()[i]);
        }
        for (int j = 0; j < 20; ++j)
        {
            out_faces.push_back(vertex_base + ico_faces[j][0]);
            out_faces.push_back(vertex_base + ico_faces[j][1]);
            out_faces.push_back(vertex_base + ico_faces[j][2]);
        }
    }

    return out;
}

int
main (int argc, char** argv)
{
    util::system::register_segfault_handler();
    util::system::print_build_timestamp("MVE Bundle to Pointset");

    /* Setup argument parser. */
    util::Arguments args;
    args.set_usage(argv[0], "[ OPTIONS ] INPUT_BUNDLE OUTPUT_PLY");
    args.set_exit_on_error(true);
    args.set_nonopt_maxnum(2);
    args.set_nonopt_minnum(2);
    args.set_helptext_indent(22);
    args.set_description("This application reads a bundle file and "
        "outputs a PLY file with a colored point cloud.");
    args.add_option('s', "spheres", true,
        "Generates a sphere for every point (radius ARG) [0.0]");
    args.parse(argc, argv);

    AppSettings conf;
    conf.input_bundle = args.get_nth_nonopt(0);
    conf.output_ply = args.get_nth_nonopt(1);

    /* Read arguments. */
    for (util::ArgResult const* i = args.next_option();
        i != nullptr; i = args.next_option())
    {
        if (i->opt->lopt == "spheres")
            conf.sphere_radius = i->get_arg<float>();
        else
        {
            args.generate_helptext(std::cout);
            std::cerr << "Unexpected option: " << i->opt->lopt << std::endl;
            return EXIT_FAILURE;
        }
    }

    mve::Bundle::Ptr bundle;
    try
    {
        bundle = mve::load_mve_bundle(conf.input_bundle);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error reading bundle: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    mve::TriangleMesh::Ptr mesh = bundle->get_features_as_mesh();
    if (conf.sphere_radius > 0.0f)
        mesh = generate_spheres(mesh, conf);

    try
    {
        mve::geom::save_ply_mesh(mesh, conf.output_ply);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error writing PLY: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
