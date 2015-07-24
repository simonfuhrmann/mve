/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MVE_OFF_FILE_HEADER
#define MVE_OFF_FILE_HEADER

#include <string>

#include "mve/defines.h"
#include "mve/mesh.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

/** Loads a triangle mesh from an OFF model file. */
TriangleMesh::Ptr
load_off_mesh (std::string const& filename);

/** Saves a triangle mesh to an OFF model file. */
void
save_off_mesh (TriangleMesh::ConstPtr mesh, std::string const& filename);

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_OFF_FILE_HEADER */
