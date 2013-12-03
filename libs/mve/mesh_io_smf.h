/*
 * IO functions for the SMF "Simple Model Format".
 * Written by Simon Fuhrmann.
 *
 * TODO: SMF is a subset of OBJ. Is it possible to unify OBJ and SMF?
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
