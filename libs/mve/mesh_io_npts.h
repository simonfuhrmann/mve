/*
 * Reader and writer for Kazhdan's ASCII and binary .npts files.
 * Written by Simon Fuhrmann.
 */

#ifndef MVE_MESH_IO_NPTS_HEADER
#define MVE_MESH_IO_NPTS_HEADER

#include <string>

#include "mve/defines.h"
#include "mve/mesh.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

/**
 * Simple importer for Kazhdan's .npts ASCII and binary files.
 */
TriangleMesh::Ptr
load_npts_mesh (std::string const& filename, bool format_binary = false);

/**
 * Simple exporter for Kazhdan's .npts ASCII and binary files.
 */
void
save_npts_mesh (TriangleMesh::ConstPtr mesh,
    std::string const& filename, bool format_binary = false);

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_MESH_IO_NPTS_HEADER */
