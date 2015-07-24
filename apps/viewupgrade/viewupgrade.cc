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

#include "util/arguments.h"
#include "util/file_system.h"
#include "mve/view.h"

struct AppSettings
{
    std::string input_path;
    bool delete_original;
};

void
convert_view (AppSettings const& conf, std::string const& fname)
{
    std::cout << "Converting view " << util::fs::basename(fname)
        << "..." << std::endl;

    std::string fname_orig = fname + ".orig";
    if (!util::fs::rename(fname.c_str(), fname_orig.c_str()))
        throw util::FileException(fname, std::strerror(errno));

    mve::View view;
    view.load_view_from_mve_file(fname_orig);
    view.save_view_as(fname);

    if (conf.delete_original && !util::fs::unlink(fname_orig.c_str()))
    {
        std::cerr << "Warning: Error deleting "
            << util::fs::basename(fname) << ": "
            << std::strerror(errno) << std::endl;
    }
}

void
convert_scene (AppSettings const& conf)
{
    util::fs::Directory dir;
    dir.scan(conf.input_path + "/views/");
    std::sort(dir.begin(), dir.end());

    for (std::size_t i = 0; i < dir.size(); ++i)
    {
        if (dir[i].is_dir || util::string::right(dir[i].name, 4) != ".mve")
            continue;
        convert_view(conf, dir[i].get_absolute_name());
    }
}

int
main (int argc, char** argv)
{
    /* Setup argument parser. */
    util::Arguments args;
    args.set_usage(argv[0], "[ OPTIONS ] INPUT");
    args.set_exit_on_error(true);
    args.set_nonopt_maxnum(1);
    args.set_nonopt_minnum(1);
    args.set_helptext_indent(22);
    args.set_description("This utility upgrades an MVE view or scene to "
        "the new MVE view format. In the deprecated format, a view is one "
        "file, while in the new format, a view is a directory. INPUT can "
        "either be a single .mve view, or a scene in which case all views "
        "are upgraded.");
    args.add_option('d', "delete-original", false, "Delete original files");
    args.parse(argc, argv);

    /* Setup defaults. */
    AppSettings conf;
    conf.input_path = util::fs::sanitize_path(args.get_nth_nonopt(0));
    conf.delete_original = false;

    /* Assign options. */
    for (util::ArgResult const* i = args.next_option();
        i != NULL; i = args.next_option())
    {
        if (i->opt->lopt == "delete-original")
            conf.delete_original = true;
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
            convert_view(conf, conf.input_path);
        else
            throw util::Exception("File or directory does not exist: ",
                conf.input_path);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
