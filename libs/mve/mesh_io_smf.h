/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MVE_MESH_IO_SMF_HEADER
#define MVE_MESH_IO_SMF_HEADER

#include <string>

#include "mve/mesh.h"
#include "mve/defines.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

/**
 * Loads a triangle mesh from a SMF file format.
 */
TriangleMesh::Ptr
load_smf_mesh (std::string const& filename);

/**
 * Saves a triangle mesh to a file in SMF file format.
 */
void
save_smf_mesh (mve::TriangleMesh::ConstPtr mesh, std::string const& filename);

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_MESH_IO_SMF_HEADER */
