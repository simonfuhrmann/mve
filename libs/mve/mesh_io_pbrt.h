/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MVE_PBRTFILE_HEADER
#define MVE_PBRTFILE_HEADER

#include "mve/mesh.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

/**
 * Saves a PBRT compatible mesh from a triangle mesh.
 */
void
save_pbrt_mesh (TriangleMesh::ConstPtr mesh, std::string const& filename);

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_PBRTFILE_HEADER */
