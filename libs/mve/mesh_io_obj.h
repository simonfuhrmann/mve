/*
 * Simple OBJ file writer, only supports vertex and face data.
 * Written by Simon Fuhrmann.
 */

#ifndef MVE_MESH_IO_OBJ_HEADER
#define MVE_MESH_IO_OBJ_HEADER

#include <string>

#include "mve/mesh.h"
#include "mve/defines.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

/** Saves a triangle mesh to an OBJ model file. */
void
save_obj_mesh (TriangleMesh::ConstPtr mesh, std::string const& filename);

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_MESH_IO_OBJ_HEADER */
