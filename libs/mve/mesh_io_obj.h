/*
 * Copyright (C) 2015, Simon Fuhrmann, Nils Moehrle
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef MVE_MESH_IO_OBJ_HEADER
#define MVE_MESH_IO_OBJ_HEADER

#include <string>

#include "mve/mesh.h"
#include "mve/defines.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

struct ObjModelPart
{
    mve::TriangleMesh::Ptr mesh;
    std::string texture_filename;
};

/** Loads a triangle mesh from an OBJ model file. */
mve::TriangleMesh::Ptr
load_obj_mesh (std::string const& filename);

/** Loads all groups from an OBJ model file. */
void
load_obj_mesh (std::string const& filename,
    std::vector<ObjModelPart>* obj_model_parts);

/** Saves a triangle mesh to an OBJ model file. */
void
save_obj_mesh (TriangleMesh::ConstPtr mesh, std::string const& filename);

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_MESH_IO_OBJ_HEADER */
