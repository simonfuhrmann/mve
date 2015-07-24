/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <string>
#include <vector>

#if defined(_WIN32)
#   include <shlobj.h>
#endif

#include "util/file_system.h"
#include "fshelpers.h"

void
get_search_paths (std::vector<std::string>* paths_out,
    std::string const& suffix)
{
    std::string binary_dir = util::fs::dirname(util::fs::get_binary_path());
    std::string app_data_dir = util::fs::get_app_data_dir();
    std::string long_suffix = util::fs::join_path("umve", suffix);

    /* Appending the program name is not necessary for the binary dir. */
    paths_out->push_back(util::fs::join_path(binary_dir, suffix));

    /* System paths need the long suffix. */
    paths_out->push_back(util::fs::join_path(app_data_dir, long_suffix));
#ifndef _WIN32
    paths_out->push_back(util::fs::join_path("/usr/local/share", long_suffix));
    paths_out->push_back(util::fs::join_path("/usr/share", long_suffix));
#endif
}
