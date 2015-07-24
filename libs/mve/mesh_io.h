/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MVE_MESH_IO_HEADER
#define MVE_MESH_IO_HEADER

#include <string>

#include "mve/defines.h"
#include "mve/mesh.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

/**
 * Auto-detects filetype from extension and delegates to readers.
 */
TriangleMesh::Ptr
load_mesh (std::string const& filename);

/**
 * Auto-detects filetype from extension and delegates to writers.
 */
void
save_mesh (TriangleMesh::ConstPtr mesh, std::string const& filename);

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_MESH_IO_HEADER */
