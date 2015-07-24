/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef UMVE_FSHELPERS_HEADER
#define UMVE_FSHELPERS_HEADER

#include <string>
#include <vector>

/**
 * Returns a vector of (OS-specific) search paths:
 * - Binary directory
 * - User home directory
 * - A global/system-wide directory
 * - QT resource path Calling
 *
 * get_search_paths(..., "X") returns a vector containing:
 * On Linux:
 *   <binary_dir>/X
 *   <home_dir>/.local/share/umve/X
 *   /usr/local/share/umve/X
 *   /usr/share/umve/X
 *
 * On Windows:
 *   <binary_dir>/X
 *   <home_dir>/Application Data/umve/X
 */
void get_search_paths (std::vector<std::string>* paths_out,
    std::string const& suffix);

#endif /* UMVE_FSHELPERS_HEADER */
