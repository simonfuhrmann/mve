/*
 * Isosurface extraction for octrees. The algorithm is based on the paper:
 *
 *   Unconstrained Isosurface Extraction on Arbitrary Octrees
 *   Michael Kazhdan, Allison Klein, Ketan Dalal, Hugues Hoppe
 *   Symposium on Geometry Processing, 2007
 *
 * Written by Simon Fuhrmann.
 */

#ifndef FSSR_ISO_SURFACE_HEADER
#define FSSR_ISO_SURFACE_HEADER

#include <vector>
#include <map>
#include <cstdint>

#include "math/algo.h"
#include "fssr/voxel.h"
#include "fssr/octree.h"
#include "fssr/iso_octree.h"
#include "fssr/defines.h"

FSSR_NAMESPACE_BEGIN

/**
 * The surfacing algorithm requires the octree hierarchy and the vector
 * of voxels. The octree is modified in that every octree node gets the
 * Marching Cubes index assigned. Otherwise the octree is unchanged.
 */
class IsoSurface
{
public:
    IsoSurface (Octree* octree, IsoOctree::VoxelVector const& voxels);
    mve::TriangleMesh::Ptr extract_mesh (void);

private:
    /** The isovertex contains interpolated position and voxel data. */
    struct IsoVertex
    {
        math::Vec3f pos;
        VoxelData data;
    };

    /** The edge index identifies an octree edge using two voxel indices. */
    typedef std::pair<uint64_t, uint64_t> EdgeIndex;

    /** Additional information for an edge. */
    struct EdgeInfo
    {
        Octree::Iterator iter;
        int edge_id;
    };

    /** An IsoEdge is a connection between two cube edges. */
    struct IsoEdge
    {
        EdgeIndex first;
        EdgeIndex second;
        EdgeInfo first_info;
        EdgeInfo second_info;
    };

    /** Vector of IsoVertex elements. */
    typedef std::vector<IsoVertex> IsoVertexVector;
    /** Maps and edge to an isovertex ID. */
    typedef std::map<EdgeIndex, std::size_t> EdgeVertexMap;
    /** List of polygons, each indexing vertices. */
    typedef std::vector<std::vector<int> > PolygonList;
    /** List of iso edges connecting vertices on cube edges. */
    typedef std::vector<IsoEdge> IsoEdgeList;

private:
    void sanity_checks (void);
    void compute_mc_index (Octree::Iterator const& iter);
    void compute_isovertices (Octree::Iterator const& iter,
        EdgeVertexMap* edgemap, IsoVertexVector* isovertices);
    bool is_isovertex_on_edge (int mc_index, int edge_id);
    void get_finest_cube_edge (Octree::Iterator const& iter,
        int edge_id, EdgeIndex* edge_index, EdgeInfo* edge_info);
    void get_finest_isoedges (Octree::Iterator const& iter,
        int face_id, IsoEdgeList* isoedges, bool descend_only);
    void get_isovertex (EdgeIndex const& edge_index, IsoVertex* iso_vertex);
    void compute_isopolygons(Octree::Iterator const& iter,
        EdgeVertexMap const& edgemap,
        std::vector<std::vector<int> >* polygons);
    void compute_triangulation(IsoVertexVector const& isovertices,
        PolygonList const& polygons,
        mve::TriangleMesh::Ptr mesh);
    VoxelData const* get_voxel_data (VoxelIndex const& index);
    std::size_t lookup_edge_vertex (EdgeVertexMap const& edgemap,
        EdgeIndex const& edge);
    void find_twin_vertex(EdgeInfo const& edge_info,
        EdgeIndex* twin, EdgeInfo* twin_info);

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
