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
#include <vector>

#include "util/system.h"
#include "util/file_system.h"
#include "util/arguments.h"
#include "util/strings.h"
#include "mve/mesh.h"
#include "mve/mesh_io.h"
#include "mve/mesh_tools.h"

#include "stanford_alignment.h"
#include "meshlab_alignment.h"

struct AppSettings
{
    std::vector<std::string> input;
    std::string output;
};

void
read_and_merge_meshlab (std::string const& config, mve::TriangleMesh::Ptr mesh)
{
    MeshlabAlignment meshlab;
    meshlab.read_alignment(config);

    MeshlabAlignment::RangeImages const& ris = meshlab.get_range_images();
    for (std::size_t i = 0; i < ris.size(); ++i)
    {
        RangeImage const& ri = ris[i];
        std::cout << "Processing " << ri.filename << "..." << std::endl;
        mve::TriangleMesh::ConstPtr tmp = meshlab.get_mesh(ri);
        mve::geom::mesh_merge(tmp, mesh);
    }
}

void
read_and_merge_stanford (std::string const& config, mve::TriangleMesh::Ptr mesh)
{
    StanfordAlignment stanford;
    stanford.read_alignment(config);

    StanfordAlignment::RangeImages const& ris = stanford.get_range_images();
    for (std::size_t i = 0; i < ris.size(); ++i)
    {
        RangeImage const& ri = ris[i];
        std::cout << "Processing " << ri.filename << "..." <<  std::endl;
        mve::TriangleMesh::ConstPtr tmp = stanford.get_mesh(ri);
        mve::geom::mesh_merge(tmp, mesh);
    }
}

int
main (int argc, char** argv)
{
    util::system::register_segfault_handler();
    util::system::print_build_timestamp("MVE FSSR Mesh Alignment");

    /* Setup argument parser. */
    util::Arguments args;
    args.set_exit_on_error(true);
    args.set_nonopt_minnum(2);
    args.set_helptext_indent(25);
    args.set_usage(argv[0], "[ OPTS ] ALIGNMENT_FILE [...] OUT_MESH");
    args.set_description("Generates a combined mesh from Stanford or Meshlab "
        "alignment data. The combined mesh contains all triangulated range "
        "images in a global, consistent coordinate system. "
        "Stanford alignments are .conf files with global camera and a "
        "list of meshes with alignment information. "
        "Meshlab alignment are .aln files with a list of meshes and a "
        "camera to world transformation matrix.");
    args.parse(argc, argv);

    /* Init default settings. */
    AppSettings conf;

    /* Scan arguments. */
    while (util::ArgResult const* arg = args.next_result())
    {
        if (arg->opt == nullptr)
        {
            conf.input.push_back(arg->arg);
            continue;
        }

        switch (arg->opt->sopt)
        {
            default:
                std::cerr << "Invalid option: " << arg->opt->sopt << std::endl;
                return EXIT_FAILURE;
        }
    }

    /* Check arguments. */
    if (conf.input.size() < 2)
    {
        args.generate_helptext(std::cerr);
        return EXIT_FAILURE;
    }
    conf.output = conf.input.back();
    conf.input.pop_back();

    /* Output file must not exist, for safety reasons. */
    if (util::fs::file_exists(conf.output.c_str()))
    {
        std::cerr << "Error: Output exists, exiting." << std::endl;
        return EXIT_FAILURE;
    }

    /* Read all stanford config files and merge into one mesh. */
    mve::TriangleMesh::Ptr all_meshes = mve::TriangleMesh::create();
    for (std::size_t i = 0; i < conf.input.size(); ++i)
    {
        std::cout << "Processing alignment file "
            << conf.input[i] << "..." << std::endl;

        if (util::string::right(conf.input[i], 4) == ".aln")
        {
            try
            {
                read_and_merge_meshlab(conf.input[i], all_meshes);
            }
            catch (std::exception& e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }
        else if (util::string::right(conf.input[i], 5) == ".conf")
        {
            try
            {
                read_and_merge_stanford(conf.input[i], all_meshes);
            }
            catch (std::exception& e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }
        else
        {
            std::cerr << "Unknown alignment format: "
                << conf.input[i] << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "Writing mesh: " << conf.output << std::endl;
    mve::geom::save_mesh(all_meshes, conf.output);

    return EXIT_SUCCESS;
}
