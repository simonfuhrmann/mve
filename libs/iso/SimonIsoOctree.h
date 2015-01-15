/*
 * Written by Simon Fuhrmann, 2013.
 */

#ifndef SIMON_ISO_OCTREE_H
#define SIMON_ISO_OCTREE_H

#include "math/vector.h"
#include "mve/mesh.h"
#include "fssr/iso_octree.h"

#include "IsoOctree.h"

template<class Real>
struct MyNodeData
{
    int mcIndex;
};

typedef fssr::VoxelData SimonVertexData;
typedef MyNodeData<float> SimonNodeData;
typedef OctNode<SimonNodeData, float> SimonOctNode;

class SimonIsoOctree : public IsoOctree<SimonNodeData, float, SimonVertexData>
{
public:
    void set_octree (fssr::IsoOctree const& octree);
    mve::TriangleMesh::Ptr extract_mesh (void);
    void clear (void);

private:
    void transfer_octree (fssr::IsoOctree::Iterator const& in_iter,
        SimonOctNode* out_node,
        SimonOctNode::NodeIndex out_node_index,
        int max_level);

    long long index_convert (fssr::VoxelIndex const& index) const;

private:
    math::Vec3f translate;
    float scale;
};

#include "SimonIsoOctree.inl"

#endif // SIMON_ISO_OCTREE_H
