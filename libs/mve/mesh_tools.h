/*
 * Tools to process meshes.
 * Written by Simon Fuhrmann.
 */

#ifndef MVE_MESH_TOOLS_HEADER
#define MVE_MESH_TOOLS_HEADER

#include "math/matrix.h"
#include "mve/defines.h"
#include "mve/mesh.h"

MVE_NAMESPACE_BEGIN
MVE_GEOM_NAMESPACE_BEGIN

/**
 * Auto-detects filetype from extension and refers to other readers.
 */
TriangleMesh::Ptr
load_mesh (std::string const& filename);

/**
 * Auto-detects filetime from extension and refers to other writers
 */
void
save_mesh (TriangleMesh::ConstPtr mesh, std::string const& filename);

/**
 * Simple importer for Kazhdan's .npts ASCII files.
 */
TriangleMesh::Ptr
load_npts_mesh (std::string const& filename, bool format_binary = false);

/**
 * Simple exporter for Kazhdan's .npts ASCII files.
 */
void
save_npts_mesh (TriangleMesh::ConstPtr mesh,
    std::string const& filename, bool format_binary = false);

/**
 * Transforms the vertices and normals of the mesh using the
 * specified rotation matrix. Transformation is IN-PLACE.
 */
void
mesh_transform (TriangleMesh::Ptr mesh, math::Matrix3f const& rot);

/**
 * Transforms the vertices of the mesh using the specified transformation
 * matrix and rotates the normals of the mesh using the roation matrix
 * of the transformation. Transformation is IN-PLACE.
 */
void
mesh_transform (TriangleMesh::Ptr mesh, math::Matrix4f const& trans);

/**
 * Merges the first given mesh with the second one, modifying the second one.
 * All vertex and face attributes are copied, faces itself are copied and
 * indices are offsetted accordingly. Note that this method will FAIL
 * if, for example, one mesh with a certain property is merged with a mesh
 * without that property (e.g. vertex color).
 */
void
mesh_merge (TriangleMesh::ConstPtr mesh1, TriangleMesh::Ptr mesh2);

/**
 * Discards isolated components with a vertex count below a threshold.
 * Passing 0 does nothing. Passing 1 or 2 deletes isolated vertices.
 * Passing 3 deletes isolated faces, and so on.
 */
void
mesh_components (TriangleMesh::Ptr mesh, std::size_t vertex_threshold);

/**
 * Scales the mesh such that it fits into a cube with length 1
 * and centers the mesh in the coordinate origin.
 */
void
mesh_scale_and_center (TriangleMesh::Ptr mesh,
    bool scale = true, bool center = true);

/**
 * Inverts the orientation of all faces in the mesh.
 */
void
mesh_invert_faces (TriangleMesh::Ptr mesh);

/**
 * Calculates the mesh axis-aligned bounding box (AABB).
 * This is done by iterating over all vertices.
 */
void
mesh_find_aabb (TriangleMesh::ConstPtr mesh,
    math::Vec3f& aabb_min, math::Vec3f& aabb_max);

/**
 * Cleans unreferenced vertices from the mesh.
 * Returns the number of deleted vertices.
 */
std::size_t
mesh_delete_unreferenced (TriangleMesh::Ptr mesh);

MVE_GEOM_NAMESPACE_END
MVE_NAMESPACE_END

#endif /* MVE_MESH_TOOLS_HEADER */
