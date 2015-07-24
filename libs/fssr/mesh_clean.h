/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef FSSR_MESH_CLEAN_HEADER
#define FSSR_MESH_CLEAN_HEADER

#include "fssr/defines.h"
#include "mve/mesh.h"

FSSR_NAMESPACE_BEGIN

/**
 * Cleans needles from the mesh by collapsing short edges of degenerated
 * triangles. The number of successful edge collapses is returned.
 */
std::size_t
clean_needles (mve::TriangleMesh::Ptr mesh, float needle_ratio_thres);

/**
 * Cleans caps from the mesh by removing vertices with only three
 * adjacent triangles. The number of successful edge collapses is returned.
 */
std::size_t
clean_caps (mve::TriangleMesh::Ptr mesh);

/**
 * Removes degenerated triangles from the mesh typical for Marching Cubes.
 * The routine first cleans needles, then caps, then remaining needles.
 */
std::size_t
clean_mc_mesh (mve::TriangleMesh::Ptr mesh, float needle_ratio_thres = 0.4f);

FSSR_NAMESPACE_END

#endif /* FSSR_MESH_CLEAN_HEADER */
