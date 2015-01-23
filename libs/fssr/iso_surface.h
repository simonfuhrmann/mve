/*
 * Isosurface extraction algorithm for octrees.
 * Written by Simon Fuhrmann.
 */

#ifndef FSSR_ISO_SURFACE_HEADER
#define FSSR_ISO_SURFACE_HEADER

#include <vector>
#include <map>
#include <stdint.h>  // TODO: Use <cstdint> once C++11 is standard.

#include "math/algo.h"
#include "fssr/voxel.h"
#include "fssr/octree.h"
#include "fssr/iso_octree.h"
#include "fssr/defines.h"

FSSR_NAMESPACE_BEGIN

/**
 * Isosurface extraction algorithm based on Michael Kazhdan's paper:
 *
 *     Unconstrained Isosurface Extraction on Arbitrary Octrees
 *     Michael Kazhdan, Allison Klein, Ketan Dalal, Hugues Hoppe
 *     Symposium on Geometry Processing, 2007
 *
 * The octree hierarchy and the voxel vector are required as input.
 */
class IsoSurface
{
public:
    struct IsoVertex
    {
        math::Vec3f pos;
        VoxelData data;
    };
    typedef std::pair<uint64_t, uint64_t> EdgeIndex;
    typedef std::map<EdgeIndex, IsoVertex> EdgeVertexMap;

public:
    IsoSurface (Octree* octree, IsoOctree::VoxelVector const& voxels);
    mve::TriangleMesh::Ptr extract_mesh (void);

private:
    void sanity_checks (void);
    void compute_mc_index (Octree::Iterator const& iter);
    void compute_isovertices (Octree::Iterator const& iter,
        EdgeVertexMap* isovertices);
    bool is_isovertex_on_edge (int mc_index, int edge_id);
    void get_finest_isoedge (Octree::Iterator const& iter,
        int edge_id, EdgeIndex* index);
    void get_isovertex (EdgeIndex const& edge_index, IsoVertex* iso_vertex);

    VoxelData const* get_voxel_data (VoxelIndex const& index);

private:
    Octree* octree;
    IsoOctree::VoxelVector const* voxels;
};

/* --------------------------------------------------------------------- */

inline
IsoSurface::IsoSurface (Octree* octree, IsoOctree::VoxelVector const& voxels)
    : octree(octree)
    , voxels(&voxels)
{
}

inline VoxelData const*
IsoSurface::get_voxel_data (VoxelIndex const& index)
{
    return math::algo::binary_search(*this->voxels, index);
}

FSSR_NAMESPACE_END

#endif // FSSR_ISO_SURFACE_HEADER
