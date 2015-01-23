/*
 * This file is part of the Floating Scale Surface Reconstruction software.
 * Written by Simon Fuhrmann.
 *
 * The code assumes an ordering of cube vertices and cube edges as follows:
 *
 *      2------3        +---3--+        +------+
 *     /|     /|       5|    11|       /|     /|        y
 *    6-+----7 |      +-+ 9--+ |      +-+----+ |        |
 *    | |    | |      | |    | |      | 4    | 1        |
 *    | 0----+-1      | +--0-+-+      | +----+-+        +------ x
 *    |/     |/       |8     |2       7/    10/        /
 *    4------5        +--6---+        +------+        z
 *   Vertex Order    Edge Order 1    Edge Order 2
 */

#include <iostream>

#include "util/timer.h"
#include "fssr/octree.h"
#include "fssr/iso_surface.h"

#define ISO_VALUE 0.0f
#define CUBE_CORNERS 8
#define CUBE_EDGES 12

FSSR_NAMESPACE_BEGIN

mve::TriangleMesh::Ptr
IsoSurface::extract_mesh (void)
{
    this->sanity_checks();
    util::WallTimer timer;

    /*
     * Assign MC index to every octree node. This can be done in two ways:
     * (1) Iterate all nodes, query corner values, and determine MC index.
     * (2) Iterate leafs only, query corner values, propagate to parents.
     * Strategy (1) is implemented, it is simpler but slightly more expensive.
     */
    std::cout << "  Computing Marching Cubes indices..." << std::flush;
    Octree::Iterator iter = this->octree->get_iterator_for_root();
    for (iter.first_node(); iter.current != NULL; iter.next_node())
        this->compute_mc_index(iter);
    std::cout << " took " << timer.get_elapsed() << " ms." << std::endl;

    /*
     * Compute isovertices on the octree edges for every leaf node.
     */
    std::cout << "  Computing isovertices..." << std::flush;
    timer.reset();
    EdgeVertexMap isovertices;
    for (iter.first_leaf(); iter.current != NULL; iter.next_leaf())
        this->compute_isovertices(iter, &isovertices);
    std::cout << " took " << timer.get_elapsed() << " ms." << std::endl;

    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    return mesh;
}

void
IsoSurface::sanity_checks (void)
{
    std::cout << "  Sanity checking..." << std::endl;
    if (this->voxels == NULL || this->octree == NULL)
        throw std::runtime_error("extract_isosurface(): NULL octree/voxels");
    for (std::size_t i = 1; i < this->voxels->size(); ++i)
        if (this->voxels->at(i).first < this->voxels->at(i - 1).first)
            throw std::runtime_error("extract_isosurface(): Voxels unsorted");
}

void
IsoSurface::compute_mc_index (Octree::Iterator const& iter)
{
    iter.current->mc_index = 0;
    for (int i = 0; i < CUBE_CORNERS; ++i)
    {
        VoxelIndex vi;
        vi.from_path_and_corner(iter.level, iter.path, i);
        VoxelData const* vd = this->get_voxel_data(vi);
        if (vd->value < ISO_VALUE)
            iter.current->mc_index |= (1 << i);
    }
}

void
IsoSurface::compute_isovertices (Octree::Iterator const& iter,
    std::map<EdgeIndex, IsoVertex>* isovertices)
{
    /* Check if cube contains an isosurface. */
    if (iter.current->mc_index == 0x00 || iter.current->mc_index == 0xff)
        return;

    for (int i = 0; i < CUBE_EDGES; ++i)
    {
        /* Check if edge contains an isovertex. */
        if (!this->is_isovertex_on_edge(iter.current->mc_index, i))
            continue;

        /* Get the finest edge that contains an isovertex. */
        EdgeIndex edge_index;
        this->get_finest_isoedge(iter, i, &edge_index);

        /* Interpolate the isovertex position and attributes. */
        IsoVertex isovertex;
        this->get_isovertex(edge_index, &isovertex);
    }
}

void
IsoSurface::get_finest_isoedge (Octree::Iterator const& iter,
    int edge_id, EdgeIndex* index)
{
    /* Locate all nodes adjacent to the edge. */
    /* Check if any of the nodes have children. */
    /* Compute iters of the two relevant childs. */
    /* Determine new edge_id of the two childs and recurse. */

    throw std::runtime_error("Error finding isoedge");
}

void
IsoSurface::get_isovertex (EdgeIndex const& edge_index, IsoVertex* iso_vertex)
{
    VoxelIndex vi1, vi2;
    vi1.index = edge_index.first;
    vi2.index = edge_index.second;
    VoxelData const* vd1 = this->get_voxel_data(vi1);
    VoxelData const* vd2 = this->get_voxel_data(vi2);
    math::Vec3d pos1 = vi1.compute_position(
        this->octree->get_root_node_center(),
        this->octree->get_root_node_size());
    math::Vec3d pos2 = vi2.compute_position(
        this->octree->get_root_node_center(),
        this->octree->get_root_node_size());
    double const w = (vd1->value - ISO_VALUE) / (vd1->value - vd2->value);
    iso_vertex->data = fssr::interpolate(*vd1, (1.0 - w), *vd2,  w);
    iso_vertex->pos = pos1 * (1.0 - w) + pos2 * w;
}

bool
IsoSurface::is_isovertex_on_edge (int mc_index, int edge_id)
{
    switch (edge_id)
    {
        case  0: return ((mc_index >> 1) & 1) ^ ((mc_index >> 0) & 1);
        case  1: return ((mc_index >> 1) & 1) ^ ((mc_index >> 3) & 1);
        case  2: return ((mc_index >> 1) & 1) ^ ((mc_index >> 5) & 1);
        case  3: return ((mc_index >> 2) & 1) ^ ((mc_index >> 3) & 1);
        case  4: return ((mc_index >> 2) & 1) ^ ((mc_index >> 0) & 1);
        case  5: return ((mc_index >> 2) & 1) ^ ((mc_index >> 6) & 1);
        case  6: return ((mc_index >> 4) & 1) ^ ((mc_index >> 5) & 1);
        case  7: return ((mc_index >> 4) & 1) ^ ((mc_index >> 6) & 1);
        case  8: return ((mc_index >> 4) & 1) ^ ((mc_index >> 0) & 1);
        case  9: return ((mc_index >> 7) & 1) ^ ((mc_index >> 6) & 1);
        case 10: return ((mc_index >> 7) & 1) ^ ((mc_index >> 5) & 1);
        case 11: return ((mc_index >> 7) & 1) ^ ((mc_index >> 3) & 1);
        default: throw std::runtime_error("Invalid edge index");
    }
}

FSSR_NAMESPACE_END
