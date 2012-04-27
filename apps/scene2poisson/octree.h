#ifndef S2P_OCTREE_HEADER
#define S2P_OCTREE_HEADER

#include <algorithm>

#include "math/vector.h"
#include "mve/trianglemesh.h"

struct OctreeNode
{
    OctreeNode* children[8];

    math::Vec3f pos;
    math::Vec3f normal;
    math::Vec3f color;
    float weight;

    OctreeNode (void);
    ~OctreeNode (void);
};

struct Point
{
    math::Vec3f pos;
    math::Vec3f color;
    math::Vec3f normal;
    float footprint;
    float confidence;
};

class Octree
{
private:
    math::Vec3f center;
    float halfsize;
    OctreeNode* root;

public:
    Octree (void);
    ~Octree (void);

    void set_aabb (math::Vec3f const& min, math::Vec3f const& max);
    void insert (Point const& p);

    mve::TriangleMesh::Ptr get_pointset (float thres) const;
};

/* ---------------------------------------------------------------- */

inline
OctreeNode::OctreeNode (void)
{
    std::fill(this->children, this->children + 8, (OctreeNode*)0);
    this->pos = this->normal = this->color = math::Vec3f(0.0f);
    this->weight = 0.0f;
}

inline
OctreeNode::~OctreeNode (void)
{
    for (int i = 0; i < 8; ++i)
        delete this->children[i];
}

inline
Octree::Octree (void)
{
    this->root = 0;
}

inline
Octree::~Octree (void)
{
    delete this->root;
}

#endif /* S2P_OCTREE_HEADER */
