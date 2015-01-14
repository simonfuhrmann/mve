/*
 * Written by Simon Fuhrmann, 2013.
 */

#include "math/vector.h"
#include "util/timer.h"
#include "fssr/iso_octree.h"
#include "fssr/basis_function.h"
#include "fssr/triangulation.h"
#include "SimonIsoOctree.h"

#define MAX_DEPTH 19

template<class Real>
void PolygonToTriangleMesh(
    std::vector<math::Vector<Real, 3> > const& vertices,
    std::vector<std::vector<int> > const& polygons,
    std::vector<unsigned int>* triangles)
{
    fssr::MinAreaTriangulation tri;
    for (size_t i = 0; i < polygons.size(); i++)
    {
        std::vector<math::Vector<Real, 3> > loop;
        loop.resize(polygons[i].size());
        for (size_t j=0;j<polygons[i].size();j++)
            loop[j] = vertices[polygons[i][j]];
        std::vector<unsigned int> result;
        tri.triangulate(loop, &result);
        for (std::size_t j = 0; j < result.size(); ++j)
            triangles->push_back(polygons[i][result[j]]);
    }
}

inline void
SimonIsoOctree::clear (void)
{
    this->cornerValues.clear();
    this->tree.deleteChildren();
}

inline void
SimonIsoOctree::set_octree (fssr::IsoOctree const& octree)
{
    this->maxDepth = MAX_DEPTH;

    /* Compute translation/scaling. */
    // To transform a point p in the octree:
    //   p' = (translate + p) * scale
    // To transfrom it back
    //   p = p' / scale - translate
    math::Vec3f aabb_min = octree.get_node_geom_for_root().aabb_min();
    math::Vec3f aabb_max = octree.get_node_geom_for_root().aabb_max();
    this->scale = 1.0f / (aabb_max - aabb_min).maximum();
    this->translate = math::Vec3f(0.5f) / this->scale
        - (aabb_min + aabb_max) / 2.0f;

    // Transfer the octree structure to the Kazhdan octree.
    SimonOctNode::NodeIndex out_index;
    this->transfer_octree(octree.get_iterator_for_root(),
        &this->tree, out_index, octree.get_max_level());

    // Copy the voxel data to the Kazhdan octree.
    this->copy_voxel_data(octree);
}

inline mve::TriangleMesh::Ptr
SimonIsoOctree::extract_mesh (void)
{
    int const fullCaseTable = 0;

    mve::TriangleMesh::Ptr mesh = mve::TriangleMesh::create();
    mve::TriangleMesh::VertexList& verts = mesh->get_vertices();
    mve::TriangleMesh::FaceList& faces = mesh->get_faces();
    mve::TriangleMesh::ConfidenceList& confs = mesh->get_vertex_confidences();
    mve::TriangleMesh::ColorList& colors = mesh->get_vertex_colors();
    mve::TriangleMesh::ValueList& scales = mesh->get_vertex_values();

    std::cout << "Getting ISO surface..." << std::flush;
    util::WallTimer timer;
    std::vector<std::vector<int> > polygons;
    std::vector<SimonVertexData> vertex_data;
    this->getIsoSurface(0.0f, verts, vertex_data, polygons, fullCaseTable);
    std::cout << " took " << timer.get_elapsed() << "ms." << std::endl;

    /* De-Normalize the output vertices. */
    for (std::size_t i = 0; i < verts.size(); ++i)
        verts[i] = verts[i] / this->scale - this->translate;

    /* Colorize mesh and copy confidence values. */
    confs.resize(verts.size());
    colors.resize(verts.size());
    scales.resize(verts.size());
    for (std::size_t i = 0; i < verts.size(); ++i)
    {
        confs[i] = vertex_data[i].conf;
        colors[i] = math::Vec4f(vertex_data[i].color, 1.0f);
        scales[i] = vertex_data[i].scale;
    }

    std::cout << "Converting polygons to triangles..." << std::flush;
    timer.reset();
    PolygonToTriangleMesh<float>(verts, polygons, &faces);
    std::cout << " took " << timer.get_elapsed() << "ms." << std::endl;

    return mesh;
}

inline void
SimonIsoOctree::transfer_octree (
    fssr::IsoOctree::Iterator const& in_iter,
    SimonOctNode* out_node,
    SimonOctNode::NodeIndex out_node_index,
    int max_level)
{
    /*
     * Have to build octree at one level less (?!?) for whatever reasons.
     * Otherwise code tries to locate samples at one level too deep.
     */
    if (in_iter.node_path.level >= max_level)
        return;

    /* Determine whether to descend into octree. */
    if (in_iter.node->children == NULL)
        return;

    out_node->initChildren();
    for (int i = 0; i < 8; ++i)
        this->transfer_octree(in_iter.descend(i),
            &out_node->children[i], out_node_index.child(i), max_level);
}

inline void
SimonIsoOctree::copy_voxel_data (fssr::IsoOctree const& isooctree)
{
    fssr::IsoOctree::VoxelVector const& voxels = isooctree.get_voxels();
    for (std::size_t i = 0; i < voxels.size(); i++)
    {
        this->cornerValues[voxels[i].first.index] = voxels[i].second;
    }
}
