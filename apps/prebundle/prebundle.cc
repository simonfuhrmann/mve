/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the GPL 3-Clause license. See the LICENSE.txt file for details.
 *
 * Generate DOT: ./prebundle --graph-mode=prebundle.dot prebundle.sfm
 * Render DOT: circo -Tpng:cairo:cairo prebundle.dot > prebundle-graph.png
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include "util/system.h"
#include "util/arguments.h"
#include "util/exception.h"
#include "sfm/bundler_common.h"

struct AppSettings
{
    std::string prebundle_file;
    std::string graph_file;
};

std::string
color_for_score (float score)
{
    int const red = static_cast<int>((1.0f - score) * 255.0f + 0.5f);
    int const green = static_cast<int>(score * 255.0f + 0.5f);
    std::stringstream ss;
    ss << "#" << std::setfill('0') << std::setw(2) << std::hex << red << green << "00";
    return ss.str();
}

void
graph_mode (AppSettings const& conf,
    sfm::bundler::ViewportList const& /*viewports*/,
    sfm::bundler::PairwiseMatching const& pairwise_matching)
{
    std::ofstream out(conf.graph_file.c_str(), std::ios::binary);
    if (!out.good())
        throw util::FileException(conf.graph_file, std::strerror(errno));

    out << "strict graph {\n";

    for (std::size_t i = 0; i < pairwise_matching.size(); ++i)
    {
        sfm::bundler::TwoViewMatching const& pair = pairwise_matching[i];
        float score = std::min(1.0f, std::max(0.0f,
            static_cast<float>(pair.matches.size()) / 100));
        out << "  " << pair.view_1_id << " -- " << pair.view_2_id;
        out << " [color=\"" << color_for_score(score) << "\"]";
        out << " [label=\" " << pair.matches.size() << "\"]";
        out << "\n";
    }

    out << "}\n";
    out.close();
}

int
main (int argc, char** argv)
{
    util::system::register_segfault_handler();
    util::system::print_build_timestamp("MVE SfM Prebundle");

    /* Setup argument parser. */
    util::Arguments args;
    args.set_usage(argv[0], "[ MODES ] PREBUNDLE_FILE");
    args.set_exit_on_error(true);
    args.set_nonopt_maxnum(1);
    args.set_nonopt_minnum(1);
    args.set_helptext_indent(23);
    args.set_description("Statistics generator for prebundle files. "
        "The graph mode outputs the matching graph in Graphviz DOT format.");
    args.add_option('g', "graph-mode", true,
        "Graph mode: Output matching graph file for DOT");
    args.parse(argc, argv);

    /* Setup defaults. */
    AppSettings conf;
    conf.prebundle_file = args.get_nth_nonopt(0);

    /* Read arguments. */
    for (util::ArgResult const* i = args.next_option();
        i != nullptr; i = args.next_option())
    {
        if (i->opt->lopt == "graph-mode")
            conf.graph_file = i->arg;
        else
            throw std::invalid_argument("Unexpected option");
    }

    /* Load prebundle file. */
    sfm::bundler::ViewportList viewports;
    sfm::bundler::PairwiseMatching pairwise_matching;
    try
    {
        sfm::bundler::load_prebundle_from_file(conf.prebundle_file,
            &viewports, &pairwise_matching);
    }
    catch (std::exception& e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        return 0;
    }

    if (!conf.graph_file.empty())
        graph_mode(conf, viewports, pairwise_matching);

    return EXIT_SUCCESS;
}
