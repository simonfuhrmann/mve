/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <cstring>
#include <cerrno>
#include <iostream>
#include <fstream>
#include <cstdlib>

#include "util/system.h"
#include "util/arguments.h"
#include "util/file_system.h"
#include "mve/view.h"
#include "sfm/bundler_common.h"
#include "sfm/feature_set.h"

/* The old prebundle format signature. */
#define VIEWPORTS_SIGNATURE_LEN 14
#define VIEWPORTS_SIGNATURE "MVE_VIEWPORTS\n"
#define MATCHING_SIGNATURE_LEN 13
#define MATCHING_SIGNATURE "MVE_MATCHING\n"

struct AppSettings
{
    std::string input_path;
    bool keep_original = false;
};

/* ------------------- Input for old Prebundle -------------------- */

void
load_old_prebundle (std::istream& in,
    sfm::bundler::ViewportList* viewports,
    sfm::bundler::PairwiseMatching* matching)
{
    /* Read and check file signature. */
    {
        char signature[VIEWPORTS_SIGNATURE_LEN + 1];
        in.read(signature, VIEWPORTS_SIGNATURE_LEN);
        signature[VIEWPORTS_SIGNATURE_LEN] = '\0';
        if (std::string(VIEWPORTS_SIGNATURE) != signature)
            throw std::invalid_argument("Invalid viewports signature");
    }

    /* Read number of viewports. */
    int32_t num_viewports;
    in.read(reinterpret_cast<char*>(&num_viewports), sizeof(int32_t));
    viewports->clear();
    viewports->resize(num_viewports);

    for (int i = 0; i < num_viewports; ++i)
    {
        sfm::bundler::Viewport& vp = viewports->at(i);
        sfm::FeatureSet& vpf = vp.features;
        int width = 0, height = 0;
        in.read(reinterpret_cast<char*>(&width), sizeof(int));
        in.read(reinterpret_cast<char*>(&height), sizeof(int));
        in.read(reinterpret_cast<char*>(&vp.focal_length), sizeof(float));
        in.read(reinterpret_cast<char*>(&vp.radial_distortion), sizeof(float));

        /* Read positions. */
        int32_t num_positions;
        in.read(reinterpret_cast<char*>(&num_positions), sizeof(int32_t));
        vpf.positions.resize(num_positions);
        for (int j = 0; j < num_positions; ++j)
            in.read(reinterpret_cast<char*>(&vpf.positions[j]), sizeof(math::Vec2f));

        /* Normalize image coordinates. */
        if (width > 0 && height > 0)
        {
            float const fwidth = static_cast<float>(width);
            float const fheight = static_cast<float>(height);
            float const fnorm = std::max(fwidth, fheight);
            for (std::size_t j = 0; j < vpf.positions.size(); ++j)
            {
                math::Vec2f& pos = vpf.positions[j];
                pos[0] = (pos[0] + 0.5f - fwidth / 2.0f) / fnorm;
                pos[1] = (pos[1] + 0.5f - fheight / 2.0f) / fnorm;
            }
        }

        /* Read colors. */
        int32_t num_colors;
        in.read(reinterpret_cast<char*>(&num_colors), sizeof(int32_t));
        vpf.colors.resize(num_colors);
        for (int j = 0; j < num_colors; ++j)
            in.read(reinterpret_cast<char*>(&vpf.colors[j]), sizeof(math::Vec3uc));

        /* Read track IDs. */
        int32_t num_track_ids;
        in.read(reinterpret_cast<char*>(&num_track_ids), sizeof(int32_t));
        vp.track_ids.resize(num_track_ids);
        for (int j = 0; j < num_track_ids; ++j)
            in.read(reinterpret_cast<char*>(&vp.track_ids[j]), sizeof(int));
    }

    /* Read and check file signature. */
    {
        char signature[MATCHING_SIGNATURE_LEN + 1];
        in.read(signature, MATCHING_SIGNATURE_LEN);
        signature[MATCHING_SIGNATURE_LEN] = '\0';
        if (std::string(MATCHING_SIGNATURE) != signature)
            throw std::invalid_argument("Invalid matching signature");
    }

    matching->clear();

    /* Read matching result. */
    int32_t num_pairs;
    in.read(reinterpret_cast<char*>(&num_pairs), sizeof(int32_t));
    for (int32_t i = 0; i < num_pairs; ++i)
    {
        int32_t id1, id2;
        in.read(reinterpret_cast<char*>(&id1), sizeof(int32_t));
        in.read(reinterpret_cast<char*>(&id2), sizeof(int32_t));
        int32_t num_matches;
        in.read(reinterpret_cast<char*>(&num_matches), sizeof(int32_t));

        sfm::bundler::TwoViewMatching tvr;
        tvr.view_1_id = static_cast<int>(id1);
        tvr.view_2_id = static_cast<int>(id2);
        tvr.matches.reserve(num_matches);
        for (int32_t j = 0; j < num_matches; ++j)
        {
            int32_t i1, i2;
            in.read(reinterpret_cast<char*>(&i1), sizeof(int32_t));
            in.read(reinterpret_cast<char*>(&i2), sizeof(int32_t));
            sfm::CorrespondenceIndex c;
            c.first = static_cast<int>(i1);
            c.second = static_cast<int>(i2);
            tvr.matches.push_back(c);
        }
        matching->push_back(tvr);
    }
}

void
load_old_prebundle_file (std::string const& filename,
    sfm::bundler::ViewportList* viewports,
    sfm::bundler::PairwiseMatching* matching)
{
    std::ifstream in(filename.c_str());
    if (!in.good())
        throw util::FileException(filename, std::strerror(errno));

    try
    {
        load_old_prebundle(in, viewports, matching);
    }
    catch (...)
    {
        in.close();
        throw;
    }

    if (in.eof())
    {
        in.close();
        throw util::Exception("Premature EOF");
    }
    in.close();
}

/* ---------------------------------------------------------------- */

void
convert_prebundle (AppSettings const& conf, std::string const& fname)
{
    /* Check if prebundle is already in new format. */
    std::ifstream in(fname.c_str());
    if (!in.good())
        throw util::FileException(fname, std::strerror(errno));
    char signature[VIEWPORTS_SIGNATURE_LEN + 1];
    in.read(signature, VIEWPORTS_SIGNATURE_LEN);
    signature[VIEWPORTS_SIGNATURE_LEN] = '\0';
    if (std::string(VIEWPORTS_SIGNATURE) != signature)
    {
        std::cout << "Skipping " << util::fs::basename(fname)
            << ": Not in old prebundle format. " << std::endl;
        return;
    }

    std::cout << "Converting prebundle: "
        << util::fs::basename(fname) << std::endl;

    /* Rename old file. */
    std::string fname_orig = fname + ".orig";
    if (!util::fs::rename(fname.c_str(), fname_orig.c_str()))
        throw util::FileException(fname, std::strerror(errno));

    /* Load old format, save new format. */
    sfm::bundler::ViewportList viewports;
    sfm::bundler::PairwiseMatching matching;
    load_old_prebundle_file(fname_orig, &viewports, &matching);
    sfm::bundler::save_prebundle_to_file(viewports, matching, fname);

    /* Delete old file if requested. */
    if (!conf.keep_original && !util::fs::unlink(fname_orig.c_str()))
    {
        std::cerr << "Warning: Error deleting "
            << util::fs::basename(fname) << ": "
            << std::strerror(errno) << std::endl;
    }
}

void
convert_view (AppSettings const& conf, std::string const& fname)
{
    if (util::fs::dir_exists(fname.c_str()))
    {
        std::cout << "View " << util::fs::basename(fname)
            << " is a directory, skipping." << std::endl;
        return;
    }

    std::cout << "Converting " << util::fs::basename(fname)
        << "..." << std::endl;

    std::string fname_orig = fname + ".orig";
    if (!util::fs::rename(fname.c_str(), fname_orig.c_str()))
        throw util::FileException(fname, std::strerror(errno));

    mve::View view;
    view.load_view_from_mve_file(fname_orig);
    view.save_view_as(fname);

    if (!conf.keep_original && !util::fs::unlink(fname_orig.c_str()))
    {
        std::cerr << "Warning: Error deleting "
            << util::fs::basename(fname) << ": "
            << std::strerror(errno) << std::endl;
    }
}

void
convert_file (AppSettings const& conf, std::string const& fname)
{
    std::string ext4 = util::string::lowercase(util::string::right(fname, 4));
    if (ext4 == ".mve")
        convert_view(conf, fname);
    else if (ext4 == ".sfm")
        convert_prebundle(conf, fname);
    else
        throw std::runtime_error("Unknown file extension: " + fname);
}

void
convert_scene (AppSettings const& conf)
{
    util::fs::Directory dir;

    /* Convert all views with .mve extension in the views directory. */
    dir.scan(util::fs::join_path(conf.input_path, "views"));
    std::sort(dir.begin(), dir.end());
    for (std::size_t i = 0; i < dir.size(); ++i)
    {
        std::string ext4 = util::string::lowercase
            (util::string::right(dir[i].name, 4));
        if (ext4 != ".mve")
            continue;
        convert_view(conf, dir[i].get_absolute_name());
    }

    /* Convert all files with .sfm extension in the base directory. */
    dir.scan(conf.input_path);
    std::sort(dir.begin(), dir.end());
    for (std::size_t i = 0; i < dir.size(); ++i)
    {
        std::string ext4 = util::string::lowercase
            (util::string::right(dir[i].name, 4));
        if (ext4 != ".sfm")
            continue;
        convert_prebundle(conf, dir[i].get_absolute_name());
    }
}

int
main (int argc, char** argv)
{
    util::system::register_segfault_handler();
    util::system::print_build_timestamp("MVE Scene Upgrade");

    /* Setup argument parser. */
    util::Arguments args;
    args.set_usage(argv[0], "[ OPTIONS ] INPUT");
    args.set_exit_on_error(true);
    args.set_nonopt_maxnum(1);
    args.set_nonopt_minnum(1);
    args.set_helptext_indent(22);
    args.set_description("This utility upgrades an MVE view, a prebundle "
        "file, or an MVE scene to the new format. See the Github wiki for "
        "more details about the new formats. INPUT can either be a single "
        ".mve view, a single .sfm prebundle file, or a scene directory. In "
        "the latter case, all views and the prebundle.sfm are upgraded.");
    args.add_option('k', "keep-original", false, "Keep original files");
    args.parse(argc, argv);

    /* Setup defaults. */
    AppSettings conf;
    conf.input_path = util::fs::sanitize_path(args.get_nth_nonopt(0));

    /* Assign options. */
    for (util::ArgResult const* i = args.next_option();
        i != nullptr; i = args.next_option())
    {
        if (i->opt->lopt == "keep-original")
            conf.keep_original = true;
        else
            throw std::invalid_argument("Unexpected option");
    }

    /* Check command line arguments. */
    if (conf.input_path.empty())
    {
        args.generate_helptext(std::cerr);
        return 1;
    }

    try
    {
        if (util::fs::dir_exists(conf.input_path.c_str()))
            convert_scene(conf);
        else if (util::fs::file_exists(conf.input_path.c_str()))
            convert_file(conf, conf.input_path);
        else
            throw util::Exception("File or directory does not exist: ",
                conf.input_path);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
